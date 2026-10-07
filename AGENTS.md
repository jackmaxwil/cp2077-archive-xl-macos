# ArchiveXL macOS Port - Agent Guidelines

## Project Context

ArchiveXL is a Cyberpunk 2077 mod (psiberx) that enables loading and extending resources without overwriting base game files (archive/resource workflows, factories, localization, customization, garment/appearance helpers). This repository is the **macOS ARM64 port** targeting the native macOS game build.

## Current Status (Canonical)

See `docs/STATUS.md`. ArchiveXL is not loadable yet: RED4ext refuses it until every address hash it needs is verified. The authoritative progress table is §0 of `~/Development/cyberpunk/RESUME_PLAN.md`. For the address-discovery deep dive, see `docs/MACOS_ADDRESS_DISCOVERY.md`.

## Development Practices

### Platform Awareness

1. **macOS-first.** All code must compile and run on macOS ARM64.
2. **No Windows APIs.** Avoid `<windows.h>`, Win32 types, MinHook, and Windows-only pathing in shared code paths.
3. **Conditional compilation.** Use `#ifdef __APPLE__` / `#ifdef _WIN32` where needed.

### Hooking & Runtime

1. **Native hooking.** Hook attachment goes through the macOS provider (`MacOSHookingProvider`) to RED4ext's native hook engine instead of Detours/MinHook.
2. **Fail loudly on required hooks.** Keep `.OrThrow()` on required hooks so missing addresses surface immediately.
3. **No exceptions in hooks.** Hook callbacks must not throw; handle/log failures internally.

## Address Resolution (Critical)

ArchiveXL has **130 custom hash IDs** (distinct from RED4ext.SDK hashes). Their offsets live in the SDK's canonical `cyberpunk2077_addresses.json`; only entries marked verified resolve.

- **Hash list**: `src/Red/Addresses/Library.hpp`
- **Resolver**: `lib/Support/macOS/ArchiveXLAddressResolver.cpp` (forwards to the SDK resolver)
- **Remaining work**: `RED4ext.SDK/scripts/plugin_requirements.py ArchiveXL.dylib` lists every unverified hash
- **Discovery tool**: `tools/macos_discover_archivexl_offsets.py`

### Working rule

- RED4ext loads ArchiveXL only once every hash it needs is verified, with evidence in `RED4ext.SDK/docs/ADDRESS_AUDIT.md`.

### Common pitfalls

1. **CName builder false positives.** “Name-like” strings often map to registration code, not real targets.
2. **Assert/trap stubs.** Many strong anchors lead to assertion helpers (`brk`) and require caller-chasing.
3. **Virtual-heavy systems.** Some targets (notably ResourceDepot) are best recovered via vtable extraction in `__DATA_CONST`.

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(sysctl -n hw.ncpu)"
```

## Runtime sanity checks

1. Install the built plugin into `<game>/red4ext/plugins/ArchiveXL/`.
2. Launch via RED4ext (`launch_red4ext.sh`).
3. Check logs for:
   - successful address resolution
   - successful hook attachment (no NULL parameters)

