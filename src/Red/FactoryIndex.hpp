#pragma once

namespace Raw::FactoryIndex
{
constexpr auto LoadFactoryAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::FactoryIndex_LoadFactoryAsync,
    /* type = */ void (*)(uintptr_t aIndex, Red::ResourcePath aPath, uintptr_t aContext)>();

#ifdef __APPLE__
// macOS: ResourcePath(index x0, CName x1), the path comes back in x0.
constexpr auto ResolveResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::FactoryIndex_ResolveResource,
    /* type = */ Red::ResourcePath (*)(uintptr_t aIndex, Red::CName aName)>();
#else
constexpr auto ResolveResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::FactoryIndex_ResolveResource,
    /* type = */ void (*)(uintptr_t aIndex, Red::ResourcePath& aPath, Red::CName aName)>();
#endif
}
