# ArchiveXL macOS Port — Status

> **Last updated:** 2026-02-21
> **Target game build:** Cyberpunk 2077 macOS **v2.3.1**
> **Target arch:** Apple Silicon (arm64)
> **Status:** Build validated, runtime validation pending

## What works

- **Address resolution**: 130 / 130 hashes mapped (0 unresolved)
- **Build**: Clean Release build produces `ArchiveXL.dylib` (v1.26.1)
- **Plugin loading**: RED4ext loads ArchiveXL successfully, 6 hooks registered via Frida Gadget
- **Services enabled**: ExtensionService, EntitySpawnerPatch, WorldWidgetLimitPatch, ArchiveService, ResourcePathRegistry — all unconditionally registered

## Validation results

### Load-time (from RED4ext log 2026-02-08)

- ArchiveXL loaded as plugin #5 of 6
- Hooks #12–#17 registered via Frida Gadget backend
- No NULL address errors, no crashes
- WorldWidgetLimitPatch correctly skipped (Windows-only)

### Runtime validation checklist

To complete runtime validation, launch with trace flags:

```bash
ARCHIVEXL_ADDR_TRACE=1 ARCHIVEXL_HOOK_TRACE=1 ./launch_red4ext.sh
```

Verify:
- [ ] All address resolutions logged with non-zero offsets
- [ ] All hook attach/detach operations succeed
- [ ] ExtensionService initializes (loads custom archive resources)
- [ ] 60+ seconds stable gameplay with archive mods installed
- [ ] No crashes or error-level log entries

### Known risks: duplicate-offset clusters

Six groups of hashes share the same offset. These are expected (stubs, shared entry points) but should be validated:

| Offset | Count | Functions |
|--------|-------|-----------|
| `0x36FE44C` | 6 | GarmentAssembler_*, GarmentAssemblerState_* |
| `0xAD043C` | 6 | Appearance*, AppearanceChangeSystem_*, ItemFactory* |
| `0x14E6D44` | 3 | AttachmentSlots_* |
| `0x21AC670` | 3 | CharacterCustomizationSystem_GetResource, ResourceSerializer_* |
| `0xACAE08` | 3 | CMesh_{Get,Find,AddStub}Appearance |

## Source-of-truth files

- **Hash list (must match 1:1)**: `src/Red/Addresses/Library.hpp` (130 `constexpr uint32_t` IDs)
- **macOS resolver table (target output)**: `lib/Support/macOS/ArchiveXLAddressResolver.cpp`
- **Discovery tool**: `tools/macos_discover_archivexl_offsets.py`
- **Reports**
  - `tools/archivexl_offsets_report_tmp.json` (latest heuristic run)
  - `tools/archivexl_offsets_report.json` (older baseline run)

## Environment variables

| Variable | Purpose |
|----------|---------|
| `ARCHIVEXL_ADDR_TRACE` | Log every address resolution |
| `ARCHIVEXL_HOOK_TRACE` | Log hook attach/detach operations |
| `ARCHIVEXL_DISABLE_BOOTSTRAP` | Skip full bootstrap (returns early) |

## How to reproduce the address report

```bash
python3 tools/macos_discover_archivexl_offsets.py \
  --binary "$HOME/Library/Application Support/Steam/steamapps/common/Cyberpunk 2077/Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077" \
  --max-candidates 48 \
  --out tools/archivexl_offsets_report_tmp.json
```

## Deep-dive docs

- `docs/MACOS_ADDRESS_DISCOVERY.md` (pipeline + learnings + remaining gaps)
- `docs/MACOS_PORTING_AUDIT.md` (original feasibility/port audit)

