#pragma once

#include "Red/Common.hpp"

namespace Raw::ResourceDepot
{
constexpr auto InitializeArchives = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_InitializeArchives,
    /* type = */ void (*)(Red::ResourceDepot* aDepot)>{};

#ifdef __APPLE__
// macOS: w5 is the exit code used on the fatal-error path; the game's own callers pass 2.
constexpr uint32_t LoadArchivesExitCode = 2;

constexpr auto LoadArchives = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_LoadArchives,
    /* type = */ void (*)(Red::ResourceDepot* aDepot,
                          Red::ArchiveGroup& aGroup,
                          const Red::DynArray<Red::CString>& aArchivePaths,
                          Red::DynArray<Red::ResourcePath>& aLoadedResourcePaths,
                          bool aMemoryResident,
                          uint32_t aExitCode)>{};

// macOS: a 16-byte {ptr, refcount} handle returned through x8 (depot x0, path x1, archive handle x2).
using ResourceHandle = Red::SretValue<0x10>;

constexpr auto RequestResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_RequestResource,
    /* type = */ ResourceHandle (*)(Red::ResourceDepot* aDepot,
                                    Red::ResourcePath aPath,
                                    const int32_t* aArchiveHandle)>{};
#else
constexpr auto LoadArchives = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_LoadArchives,
    /* type = */ void (*)(Red::ResourceDepot* aDepot,
                          Red::ArchiveGroup& aGroup,
                          const Red::DynArray<Red::CString>& aArchivePaths,
                          Red::DynArray<Red::ResourcePath>& aLoadedResourcePaths,
                          bool aMemoryResident)>{};

constexpr auto RequestResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_RequestResource,
    /* type = */ uintptr_t* (*)(Red::ResourceDepot* aDepot,
                                const uintptr_t* aOutResourceHandle,
                                Red::ResourcePath aPath,
                                const int32_t* aArchiveHandle)>{};
#endif

constexpr auto CheckResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_CheckResource,
    /* type = */ bool (*)(Red::ResourceDepot* aDepot, Red::ResourcePath aPath)>{};
}
