#pragma once

#include "Red/EntityTemplate.hpp"

namespace Red
{
using AppearanceResource = appearance::AppearanceResource;
using AppearanceDefinition = appearance::AppearanceDefinition;
}

namespace Raw::AppearanceDefinition
{
using Mutex = Core::OffsetPtr<0xE6, Red::SharedSpinLock>;
using CompilationFlag = Core::OffsetPtr<0xE7, bool>;
using CompilationJob = Core::OffsetPtr<0xE8, Red::JobHandle>;

#ifdef __APPLE__
// macOS: DynArray<Handle<ISerializable>>(x8 out; const SharedPtr<Token>& x0).
constexpr auto ExtractPartComponents = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceDefinition_ExtractPartComponents,
    /* type = */ Red::DynArray<Red::Handle<Red::ISerializable>> (*)(
        const Red::SharedPtr<Red::ResourceToken<Red::EntityTemplate>>& aPartToken)>();
#else
constexpr auto ExtractPartComponents = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceDefinition_ExtractPartComponents,
    /* type = */ void* (*)(Red::DynArray<Red::Handle<Red::ISerializable>>& aOut,
                           const Red::SharedPtr<Red::ResourceToken<Red::EntityTemplate>>& aPartToken)>();
#endif
}

namespace Raw::AppearanceResource
{
using Mutex = Core::OffsetPtr<0xF0, Red::SharedSpinLock>;

constexpr auto PostLoad = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceResource_PostLoad,
    /* type = */ void (*)(Red::AppearanceResource* aResource, Red::PostLoadParams* a2)>();

#ifdef __APPLE__
// macOS: Handle<AppearanceDefinition>(x8 out; resource x0, CName x1, u32 w2, u8 w3).
constexpr auto FindAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceResource_FindAppearanceDefinition,
    /* type = */ Red::Handle<Red::AppearanceDefinition> (*)(Red::AppearanceResource* aResource,
                                                            Red::CName aName,
                                                            uint32_t,
                                                            uint8_t)>();
#else
constexpr auto FindAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceResource_FindAppearanceDefinition,
    /* type = */ uintptr_t (*)(Red::AppearanceResource* aResource,
                               Red::Handle<Red::AppearanceDefinition>* aDefinition,
                               Red::CName aName,
                               uint32_t,
                               uint8_t)>();
#endif
}
