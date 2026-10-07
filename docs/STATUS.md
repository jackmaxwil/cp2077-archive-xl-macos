# ArchiveXL macOS Port: Status

Target: Cyberpunk 2077 macOS 2.3.1 (arm64).

**Not loadable yet.** ArchiveXL builds, but most of the game addresses it needs are still unverified in the SDK's canonical address DB. RED4ext refuses to load a plugin unless every address hash compiled into it is verified, so ArchiveXL is not loaded in game. Earlier claims that all 130 addresses were resolved and that hooks were registered through Frida Gadget were wrong: the offsets came from heuristic discovery and were never verified, and the Frida backend never ran plugin detours.

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
