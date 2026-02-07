# ArchiveXL macOS Port - Agent Guidelines

## Project Context

ArchiveXL is a Cyberpunk 2077 mod (psiberx) that enables loading and extending resources without overwriting base game files (archive/resource workflows, factories, localization, customization, garment/appearance helpers). This repository is the **macOS ARM64 port** targeting the native macOS game build.

## Current Status (Canonical)

See `docs/STATUS.md` for the current port snapshot. For the address-discovery deep dive, see `docs/MACOS_ADDRESS_DISCOVERY.md`.

## Development Practices

### Platform Awareness

1. **macOS-first.** All code must compile and run on macOS ARM64.
2. **No Windows APIs.** Avoid `<windows.h>`, Win32 types, MinHook, and Windows-only pathing in shared code paths.
3. **Conditional compilation.** Use `#ifdef __APPLE__` / `#ifdef _WIN32` where needed.

### Hooking & Runtime

1. **Frida-based hooking.** Hook attachment is performed via the macOS provider (Frida Gadget) instead of Detours/MinHook.
2. **Fail loudly on required hooks.** Keep `.OrThrow()` on required hooks so missing addresses surface immediately.
3. **No exceptions in hooks.** Hook callbacks must not throw; handle/log failures internally.

## Address Resolution (Critical)

ArchiveXL has **130 custom hash IDs** (distinct from RED4ext.SDK hashes). On macOS we must provide a complete mapping:

- **Hash list**: `src/Red/Addresses/Library.hpp`
- **Resolver table target**: `lib/Support/macOS/ArchiveXLAddressResolver.cpp`
- **Discovery tool**: `tools/macos_discover_archivexl_offsets.py`

### Working rule

- A port is not “functionally ready” until **all 130** resolve to non-zero and hooks attach without NULL/0 addresses.

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

