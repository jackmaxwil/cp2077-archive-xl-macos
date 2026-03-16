# ArchiveXL (macOS)

Custom archive and resource loading for Cyberpunk 2077 on macOS ARM64.

**Status:** Build validated — 130/130 addresses resolved, 6 hooks registered, all services enabled.

## What it does

ArchiveXL enables loading custom resources (archives, factories, localization, garments, animations, world streaming) without overwriting base game files. Built as a RED4ext `.dylib` plugin with 130 custom address mappings.

## Prerequisites

- RED4ext installed and functional
- CMake 3.24+, Clang 15+

## Build

```bash
mkdir build-macos && cd build-macos
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

## Install

```bash
cp build-macos/ArchiveXL.dylib "<game>/red4ext/plugins/ArchiveXL/"
```

## Runtime validation

```bash
ARCHIVEXL_ADDR_TRACE=1 ARCHIVEXL_HOOK_TRACE=1 ./launch_red4ext.sh
```

## Key files

| File | Purpose |
|------|---------|
| `lib/Support/macOS/ArchiveXLAddressResolver.cpp` | 130 address mappings |
| `src/Red/Addresses/Library.hpp` | Hash constant definitions |
| `tools/macos_discover_archivexl_offsets.py` | Address discovery tool |
| `docs/STATUS.md` | Port status |

## Related projects

| Project | Description |
|---------|-------------|
| [RED4ext](../RED4ext) | Required mod loader |
| [RED4ext.SDK](../RED4ext.SDK) | SDK dependency |
| [TweakXL](../cp2077-tweak-xl) | Companion tweak plugin |

## Attribution

Forked from [psiberx/cp2077-archive-xl](https://github.com/psiberx/cp2077-archive-xl). macOS port by memaxo.
