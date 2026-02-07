# ArchiveXL macOS Porting Audit

> **Date:** December 31, 2025  
> **Auditor:** Based on RED4ext, RED4ext.SDK, and TweakXL macOS port experience

For the current implementation status, see [STATUS.md](STATUS.md).

## Executive Summary

**Feasibility: ✅ HIGHLY FEASIBLE - Very similar to TweakXL**

ArchiveXL is an excellent candidate for macOS porting. It shares the same architecture as TweakXL (by the same author, psiberx), uses the same Core library, and has **NO graphics dependencies**. The porting effort should be comparable to TweakXL.

| Aspect | TweakXL | ArchiveXL | Delta |
|--------|---------|-----------|-------|
| Porting Effort | ~2 weeks | **~2-3 weeks** | +50% hooks |
| Graphics Dependency | None ✅ | **None ✅** | Same |
| Core Library | psiberx/Core | **psiberx/Core** | Nearly identical |
| Hooking Complexity | Low | **Medium** | More hooks |
| Address Hashes Needed | ~15 | **~130** | 8x more |
| Source Files | 62 cpp + 146 hpp | **62 cpp + 146 hpp** | Same |

---

## Architecture Analysis

### Code Structure

ArchiveXL follows the exact same architecture as TweakXL:

```
cp2077-archive-xl-macos/
├── src/
│   ├── main.cpp              # RED4ext entry point (identical pattern)
│   ├── App/
│   │   ├── Application.cpp   # Service registration
│   │   ├── Archives/         # Archive loading
│   │   ├── Extensions/       # Feature modules (18 extensions)
│   │   ├── Patches/          # Game patches
│   │   └── Shared/           # Shared utilities
│   └── Red/
│       ├── Addresses/        # ~130 address hashes
│       └── *.hpp             # Game structure definitions
├── lib/
│   ├── Core/                 # ✅ Same as TweakXL's Core
│   │   ├── Facades/
│   │   ├── Foundation/
│   │   ├── Hooking/
│   │   ├── Logging/
│   │   └── Runtime/
│   └── Support/
│       ├── MinHook/          # → Replace with Frida
│       ├── RED4ext/          # → Already ported
│       ├── RedLib/           # Windows-only
│       └── Spdlog/           # → Already ported
└── vendor/
    └── RED4ext.SDK/          # → Use macOS fork
```

### Key Similarity to TweakXL

The `lib/Core/` directory is **nearly identical** to TweakXL's. This means:

1. ✅ All TweakXL platform fixes apply directly
2. ✅ Same hooking abstraction (`Core::HookingDriver`)
3. ✅ Same logging infrastructure (`SpdlogProvider`)
4. ✅ Same module/runtime handling patterns

---

## Windows Dependencies Analysis

### 🟢 No Critical Blockers

| Dependency | Usage | macOS Solution |
|------------|-------|----------------|
| MinHook | Hooking | Frida Gadget (done in RED4ext) |
| WIL | 3 files | Platform abstraction (done in TweakXL) |
| HMODULE | Entry points | `void*` + `dlopen` |
| wil::GetModuleFileNameW | Path resolution | `Core::Platform::GetModuleFileName` |

### No Graphics Dependencies ✅

```bash
$ grep -r "D3D12\|DXGI\|DirectX" src/
# No matches!
```

This is a crucial difference from CET - ArchiveXL has **zero graphics code**.

---

## Comparison with TweakXL Port

### Already Solved (Reusable)

| Component | TweakXL Solution | ArchiveXL Effort |
|-----------|------------------|------------------|
| `lib/Core/macOS.hpp` | Created | Copy directly |
| `lib/Core/Win.hpp` | Platform guarded | Copy pattern |
| `lib/Core/Stl.hpp` | TiltedCore abstraction | Already compatible |
| `CMakeLists.txt` | xmake → CMake | Copy & adapt |
| Frida hooking | `MacOSHookingProvider` | Copy directly |
| Address resolver | `TweakXLAddressResolver` | Create `ArchiveXLAddressResolver` |
| RED4ext.SDK | macOS fork | Already available |

### New Work Required

| Task | Effort | Notes |
|------|--------|-------|
| Map 130 address hashes | 2-3 weeks | Main effort |
| Update `.gitmodules` | 10 min | Point to SDK fork |
| Create `CMakeLists.txt` | 2-3 hours | Copy from TweakXL |
| Platform guards | 1-2 days | Same patterns |
| Testing | 1 week | Integration with game |

---

## Address Hashes Analysis

ArchiveXL requires **130 address hashes** in `src/Red/Addresses/Library.hpp`:

### Categories

| Category | Count | Criticality |
|----------|-------|-------------|
| ResourceDepot | 4 | 🔴 Critical |
| Entity functions | 11 | 🔴 Critical |
| AppearanceChanger | 7 | 🔴 Critical |
| GarmentAssembler | 11 | 🔴 Critical |
| CharacterCustomization | 27 | 🟡 Medium |
| Mesh/MorphTarget | 9 | 🟡 Medium |
| Journal/Quest | 8 | 🟢 Low |
| Localization | 4 | 🟢 Low |
| Other systems | ~49 | Varies |

### Address Mapping Strategy

1. **Shared with TweakXL:** Some hashes (like `TweakDB_Load`) are already mapped
2. **Shared with RED4ext:** Basic engine functions may be shared
3. **New mappings needed:** Most are ArchiveXL-specific

```cpp
// Example from Library.hpp - needs ARM64 offsets
constexpr uint32_t Entity_Attach = 4248638169;
constexpr uint32_t Entity_Detach = 2263681375;
constexpr uint32_t Entity_Dispose = 2515274237;
constexpr uint32_t Entity_Initialize = 3490519617;
constexpr uint32_t Entity_Assemble = 2182550867;
constexpr uint32_t Entity_Reassemble = 1560690857;
```

---

## Hooking Analysis

ArchiveXL uses **74 hooks** across 18 extension files:

```bash
$ grep -r "HookOnce\|HookAfter\|HookBefore\|HookWrap" src/ | wc -l
74
```

### Hook Distribution by Extension

| Extension | Hooks | Complexity |
|-----------|-------|------------|
| Garment | 16 | High |
| Customization | 14 | High |
| ResourcePatch | 10 | Medium |
| Attachment | 7 | Medium |
| ExtensionService | 4 | Low |
| Localization | 4 | Low |
| Journal | 3 | Low |
| QuestPhase | 3 | Low |
| Others | 13 | Low |

### Hooking Pattern (Already Solved)

ArchiveXL uses the same `Core::Hook` abstraction as TweakXL:

```cpp
// src/App/Extensions/Garment/Extension.cpp
HookBefore<Raw::ItemFactoryRequest::LoadAppearance>(&OnLoadAppearanceResource).OrThrow();
HookAfter<Raw::AppearanceResource::FindAppearance>(&OnResolveDefinition).OrThrow();
Hook<Raw::EntityTemplate::FindAppearance>(&OnResolveAppearance).OrThrow();
HookWrap<Raw::GarmentAssembler::ProcessSkinnedMesh>(&OnProcessGarmentMesh).OrThrow();
```

This will work automatically once `MacOSHookingProvider` is registered (same as TweakXL).

---

## Files Requiring Modification

### Direct Copies from TweakXL

```
lib/Core/macOS.hpp                           # Copy entirely
lib/Support/macOS/MacOSHookingProvider.hpp   # Copy entirely  
lib/Support/macOS/MacOSHookingProvider.cpp   # Copy entirely
lib/Support/macOS/ArchiveXLAddressResolver.hpp  # Adapt from TweakXL
lib/Support/macOS/ArchiveXLAddressResolver.cpp  # Adapt from TweakXL
```

### Modifications Needed

| File | Changes |
|------|---------|
| `src/main.cpp` | Add macOS entry point (`__attribute__((constructor))`) |
| `src/App/Application.hpp` | `HMODULE` → `void*` |
| `src/App/Application.cpp` | Register macOS providers |
| `lib/Core/Win.hpp` | Platform guards |
| `lib/Core/Runtime/ModuleImage.cpp` | Use `Core::Platform::GetModuleFileName` |
| `.gitmodules` | Point to `memaxo/RED4ext.SDK-macos` |

### New Files

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Build configuration |
| `lib/Support/macOS/*.hpp/.cpp` | macOS providers |

---

## Build System

### Current: xmake

```lua
-- xmake.lua
add_requires("hopscotch-map", "minhook", "spdlog", "tiltedcore", "yaml-cpp")

target("ArchiveXL")
    set_kind("shared")
    set_filename("ArchiveXL.dll")
    add_packages("hopscotch-map", "minhook", "spdlog", "tiltedcore", "yaml-cpp")
    add_syslinks("Version", "User32")
```

### Target: CMake (macOS)

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(ArchiveXL VERSION 1.26.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if(APPLE)
    add_compile_definitions(RED4EXT_PLATFORM_MACOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(OUTPUT_NAME "ArchiveXL")
endif()

# Homebrew dependencies
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)

# Submodule
add_library(RED4ext.SDK INTERFACE)
target_include_directories(RED4ext.SDK INTERFACE 
    "${CMAKE_SOURCE_DIR}/vendor/RED4ext.SDK/include")

# Sources
file(GLOB_RECURSE ARCHIVEXL_SOURCES "src/*.cpp" "lib/*.cpp")
add_library(ArchiveXL SHARED ${ARCHIVEXL_SOURCES})

target_link_libraries(ArchiveXL PRIVATE
    RED4ext.SDK spdlog::spdlog yaml-cpp)
```

---

## Estimated Effort

### Phase 1: Build System & Platform Abstraction (2-3 days)

1. Create `CMakeLists.txt`
2. Copy macOS platform files from TweakXL
3. Update `.gitmodules`
4. Get code compiling

### Phase 2: Address Resolution (2-3 weeks)

1. Create `ArchiveXLAddressResolver`
2. Map critical addresses (~40 for basic functionality)
3. Map remaining addresses (~90 for full features)

### Phase 3: Integration Testing (1 week)

1. Test archive loading
2. Test garment system
3. Test customization
4. Test all extensions

### Total: ~3-4 weeks

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Address mapping complexity | Medium | High | Prioritize critical functions |
| Game structure differences | Low | Medium | Validate ARM64 struct layouts |
| Extension conflicts | Low | Low | Test each extension individually |

---

## Recommendations

### 1. Start Immediately ✅

ArchiveXL is the ideal next port:
- Same architecture as TweakXL
- No graphics dependencies
- Critical for many mods (item additions, localization)

### 2. Reuse TweakXL Infrastructure

Copy directly:
- `lib/Core/macOS.hpp`
- `lib/Support/macOS/MacOSHookingProvider.*`
- CMakeLists.txt patterns
- Platform abstraction patterns

### 3. Prioritize Address Mapping

Focus on these first (enables basic functionality):
1. `ResourceDepot_*` - Archive loading
2. `Entity_*` - Entity spawning
3. `AppearanceResource_*` - Appearances
4. `GarmentAssembler_*` - Garment system

### 4. Create Combined Address Database

Both TweakXL and ArchiveXL need addresses. Consider:
- Shared address file between projects
- Community-maintained address database
- Tools for automated discovery

---

## Files to Copy from TweakXL

```bash
# From cp2077-tweak-xl to cp2077-archive-xl-macos
cp lib/Core/macOS.hpp → lib/Core/macOS.hpp
cp lib/Support/macOS/MacOSHookingProvider.hpp → lib/Support/macOS/
cp lib/Support/macOS/MacOSHookingProvider.cpp → lib/Support/macOS/

# Adapt these
lib/Support/macOS/TweakXLAddressResolver.hpp → ArchiveXLAddressResolver.hpp
lib/Support/macOS/TweakXLAddressResolver.cpp → ArchiveXLAddressResolver.cpp
```

---

## Conclusion

**ArchiveXL is highly portable and should be the next priority after TweakXL.**

The main effort is address mapping (~130 hashes), but the infrastructure work is minimal thanks to code sharing with TweakXL. The same architecture means lessons learned transfer directly.

**Recommended Timeline:**
- Week 1-2: Build system + platform abstraction + critical addresses
- Week 3: Remaining addresses + testing
- Week 4: Polish + documentation + release

---

## References

- [RED4ext-macos](https://github.com/memaxo/RED4ext-macos)
- [RED4ext.SDK-macos](https://github.com/memaxo/RED4ext.SDK-macos)
- [TweakXL-macos](https://github.com/memaxo/cp2077-tweak-xl-macos)
- [Original ArchiveXL](https://github.com/psiberx/cp2077-archive-xl)
