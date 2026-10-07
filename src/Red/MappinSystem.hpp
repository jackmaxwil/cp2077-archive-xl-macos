#pragma once

namespace Raw::MappinSystem
{
using CookedMappinResource = Core::OffsetPtr<0x58, Red::Handle<Red::gameMappinResource>>;
using CookedPoiResource = Core::OffsetPtr<0x68, Red::Handle<Red::gamePointOfInterestMappinResource>>;

#ifdef __APPLE__
// macOS: the system-level getters are 16-byte stubs (ldr, cbz, b, ret) with no room for a safe patch. Hook the
// resource-level lookups they tail-call instead; the first argument is then the cooked resource, not the system.
// Both lookups also serve GetMappinPosition (system vtable +0x260), see RED4ext.SDK docs/re/world.md.
constexpr auto GetMappinData = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinResource_GetMappinData,
    /* type = */ void* (*)(void* aResource, uint32_t aHash)>();

constexpr auto GetPoiData = Core::RawFunc<
    /* addr = */ Red::AddressLib::PointOfInterestMappinResource_GetMappinData,
    /* type = */ void* (*)(void* aResource, uint32_t aHash)>();
#else
constexpr auto GetMappinData = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_GetMappinData,
    /* type = */ void* (*)(void* aSystem, uint32_t aHash)>();

constexpr auto GetPoiData = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_GetPoiData,
    /* type = */ void* (*)(void* aSystem, uint32_t aHash)>();
#endif

#ifdef __APPLE__
// macOS: (this, RuntimeScene*, a2, JobGroup&); all four are forwarded to the original.
constexpr auto OnStreamingWorldLoaded = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_OnStreamingWorldLoaded,
    /* type = */ void (*)(void* aSystem, Red::worldRuntimeScene*, uintptr_t a3, uintptr_t a4)>();
#else
constexpr auto OnStreamingWorldLoaded = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_OnStreamingWorldLoaded,
    /* type = */ void (*)(void* aSystem, Red::worldRuntimeScene*)>();
#endif
}
