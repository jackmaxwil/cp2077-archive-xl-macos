# ArchiveXL macOS Address Discovery (v2.3.1)

> **Last updated:** 2026-02-01  
> **Target game build:** Cyberpunk 2077 macOS **v2.3.1** (arm64)  

## Goal

ArchiveXL uses a set of **130 custom 32-bit IDs** (not the RED4ext.SDK hashes) to resolve game function addresses at runtime.

On macOS we must provide a **complete** hash→offset map so that:

- all `Core::RawFunc<Red::AddressLib::..., ...>` resolve to non-null
- all hooks attach successfully (no “required parameter is NULL” failures)
- features like archive loading, resource interception, customization, garment, localization, etc. work end-to-end

## Where the 130 IDs come from

- `src/Red/Addresses/Library.hpp` is the **canonical ordered list** of the 130 IDs.
- `lib/Support/macOS/ArchiveXLAddressResolver.cpp` is the **target** where `m_addressTable[...] = 0x...;` is hardcoded.

## Tooling used

### Primary automated tool (repo-local)

- `tools/macos_discover_archivexl_offsets.py`
  - parses `Library.hpp`
  - extracts `__TEXT.__cstring` strings via `rabin2 -z -j`
  - decodes `LC_FUNCTION_STARTS` to get accurate function boundaries
  - bulk-scans `__TEXT.__text` for ARM64 `ADRP + ADD` sequences that materialize string addresses
  - maps xrefs → function start offsets → “best” candidate per hash
  - writes a JSON evidence report:
    - `tools/archivexl_offsets_report_tmp.json` (latest)
    - `tools/archivexl_offsets_report.json` (older baseline)

### Supporting tools

- **radare2** (`r2`, `rabin2`) for strings/sections and quick disassembly.
- **RED4ext** helper scripts/libraries (reference):
  - `RED4ext/scripts/lib/address_discovery.py` (string → xref → function start style workflows)

## Current automated status

As of the latest heuristic run (see `tools/archivexl_offsets_report_tmp.json`):

- **Resolved:** 113 / 130
- **Unresolved:** 17 / 130
- **Duplicate offsets among resolved:** 19 offsets shared by multiple hashes (some may be real wrappers/overloads; several look suspicious and need validation)

### Unresolved (17)

- `AnimatedComponent_InitializeAnimations`
- `BufferReader_MakeType0`
- `BufferReader_MakeType1`
- `CharacterCustomizationFeetController_CheckState`
- `CharacterCustomizationGenitalsController_OnAttach`
- `CharacterCustomizationGenitalsController_CheckState`
- `CharacterCustomizationHairstyleController_OnDetach`
- `CharacterCustomizationHairstyleController_CheckState`
- `CharacterCustomizationSystem_Uninitialize`
- `Entity_Uninitialize`
- `FactoryIndex_LoadFactoryAsync`
- `IPlacedComponent_SetTransform`
- `Localization_LoadLipsyncs`
- `PersistencySystem_SetPersistentStateData`
- `ResourceDepot_InitializeArchives`
- `ResourceDepot_LoadArchives`
- `ResourceSerializer_Deserialize`

### Biggest duplicate-offset clusters (needs validation)

- `0x36FE44C` (6): `GarmentAssembler_*` / `GarmentAssemblerState_*`
- `0xAD043C` (6): several `Appearance*` + `AppearanceChangeSystem_*` + `ItemFactory*`
- `0x14E6D44` (3): `AttachmentSlots_*`
- `0x21AC670` (3): `CharacterCustomizationSystem_GetResource` + `ResourceSerializer_{Load,PostLoad}`
- `0xACAE08` (3): `CMesh_{Get,Find,AddStub}Appearance`
- `0x1704A5C` (2): `ResourceDepot_{RequestResource,CheckResource}` (**very likely wrong; see ResourceDepot section**)

## Key learnings (high signal)

### 1) “String hit” ≠ “target function” (CName builder false positives)

Many attractive markers (especially name-like strings such as `LoadResourceAsync`) are referenced by generic **CName registration** / builder code. This leads to:

- multiple unrelated hashes mapping to the same “registration hub”
- high duplicate rates
- plausible-looking but wrong mappings

Mitigations already implemented in `tools/macos_discover_archivexl_offsets.py`:

- filtering out **pure alphanumeric** “name-like” strings when better markers exist
- preferring **message-like / path-like** strings (spaces, `%`, extensions, file paths, etc.)
- penalizing “string-heavy” functions referenced by many candidate strings

### 2) Assertion/trap stubs look like perfect anchors (but are not real logic)

Some very strong anchors (e.g. file paths like `.../resourceGameDepot.cpp` or strings like `!m_archives.Empty()`) can resolve to tiny functions that:

- load file/line/assert text
- call a common assert/log routine
- `brk` / abort

Those stubs are **not** the real implementation we need to hook/call. In many cases the correct target is a **caller** of the stub.

**Action item:** improve automation to detect these stubs and map to caller functions instead of the stub itself.

### 3) Current xref scan is narrow (ADRP+ADD only)

The bulk scanner currently matches only the common literal materialization pattern:

- `ADRP reg, page`
- `ADD reg, reg, #imm12`

This likely misses valid references built via:

- `ADRP + LDR` (GOT/indirect)
- `ADR` (pc-relative)
- literal `LDR` forms

**Action item:** expand xref decoding to raise coverage for the remaining unresolved hashes.

## ResourceDepot: concrete new evidence + likely incorrect current mapping

ArchiveXL depends heavily on ResourceDepot behavior (archives, resource existence, request paths).

### Problem

The latest heuristic report currently maps:

- `ResourceDepot_RequestResource`
- `ResourceDepot_CheckResource`

to the same offset `0x1704A5C`, which corresponds to an **assert/trap stub** (loads assert strings and executes `brk`).

### Evidence (vtable extraction)

By inspecting `__DATA_CONST.__const` we can locate a depot-like vtable at:

- **vtable address:** `0x10728A630` (in `__DATA_CONST.__const`)

The vtable contains method pointers into `__TEXT` including (among others):

- `0x103ED9B94` (offset `0x3ED9B94`) — large method-shaped implementation
- `0x103ED9E9C` (offset `0x3ED9E9C`) — smaller wrapper/lookup implementation

These are far more plausible candidates for `RequestResource(...)` and `ResourceExists(...)` than the assert stub.

### Implication

For ResourceDepot in particular, we should treat **vtable-based mapping** as a higher-confidence escape hatch than string heuristics.

## Recommended next steps

1. **Add assert-stub detection + caller chasing** to `tools/macos_discover_archivexl_offsets.py`.
2. **Add vtable-based overrides** for ResourceDepot request/exists (and potentially other virtual-heavy systems).
3. **Expand xref scanning** beyond ADRP+ADD to include ADR and ADRP+LDR patterns.
4. Re-run discovery and regenerate `tools/archivexl_offsets_report_tmp.json` until:
   - resolved = 130
   - unresolved = 0
   - duplicates are explainable or eliminated

