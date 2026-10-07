# ArchiveXL macOS Port: Status

Target: Cyberpunk 2077 macOS 2.3.1 (arm64).

Every address hash compiled into the macOS build is verified in the SDK's canonical address DB
(`plugin_requirements.py build-dev/ArchiveXL.dylib` reports 0 unverified), so RED4ext can load it. It has not been
confirmed in game yet. Earlier claims that all 130 addresses were resolved and that hooks were registered through Frida
Gadget were wrong: the offsets came from heuristic discovery and were never verified.

## macOS differences

- Functions the macOS game inlines are hooked at their callers or rebuilt in code: garment ChangeItem/ChangeCustomItem
  (aggregator wrappers), TPP IsAffectedSlot (equip/unequip handlers), GetSuffixValue (folded into the GetSuffixes hook
  and evaluated in code for dynamic appearances), GetHairColor (reimplemented), the AI spot reserve (DynArray_Realloc),
  the mappin getters (resource-level lookups) and the resource token Unk38 release (in the SDK).
- Struct results returned through x8 on arm64 are declared as by-value returns of non-trivially-copyable types.
- `Core::RawVFunc` offsets stay MSVC offsets in the source; on macOS each is shifted by 8 (Itanium destructor pair).
- Disabled on macOS (fail closed): `ArchiveXL.Reload()` hot reload (no verified per-frame loader hook), the Transmog
  factory template override (the factory handle at `+0x120` is unverified), and native registration of the two
  PuppetState enums (the SDK's CEnum overrides are not macOS-verified).
- Writes to classes whose SDK layout differs on macOS (inkWidgetLibraryResource, some world nodes) are checked against
  the game's RTTI offsets first and skipped on mismatch.

## Old crash at 0x1D50D24 (0x101D50CE8)

The fault was in `game::input` context loading (ContextLoaderXML, a loop over `this+0x108`). ArchiveXL does not hook
anything input-related. The cause was most likely the old, unverified DB value for `MappinSystem_GetMappinData`,
`1:0x1D83A68`: that function is the input XML mapping loader (it references "AddActionPreset() failed…",
"AddMapping failed for %hs…", "Loading mappingPresets…"), in the same module as the fault. A two-argument mappin detour
patched over it corrupts the loader state. The macOS build now hooks the verified mappin resource lookup
`1:0x429EED0` instead, and fail-closed resolution prevents any unverified address from being patched.

- Authoritative progress: §0 of `~/Development/cyberpunk/RESUME_PLAN.md` (a workspace file outside this repo).
- Address evidence: `RED4ext.SDK/docs/ADDRESS_AUDIT.md`.
- Remaining work: `python3 RED4ext.SDK/scripts/plugin_requirements.py ArchiveXL.dylib` lists every required hash and whether it is verified.

## Key files

- `src/Red/Addresses/Library.hpp`: ArchiveXL's hash constants.
- `lib/Support/macOS/ArchiveXLAddressResolver.cpp`: forwards every hash to the SDK resolver (canonical DB, verified entries only).
- `tools/macos_discover_archivexl_offsets.py`: heuristic offset discovery. Its output is a candidate list, not evidence.
- `docs/MACOS_ADDRESS_DISCOVERY.md`: discovery pipeline notes.

## Environment variables

| Variable | Purpose |
|----------|---------|
| `ARCHIVEXL_ADDR_TRACE` | Log every address resolution |
| `ARCHIVEXL_HOOK_TRACE` | Log hook attach/detach operations |
| `ARCHIVEXL_DISABLE_BOOTSTRAP` | Skip full bootstrap (returns early) |
