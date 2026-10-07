#pragma once

#ifdef __APPLE__
#include <dlfcn.h>

namespace Raw::AISpotPersistentDataArray
{
// macOS: no standalone reserve exists, every call site inlines it (RED4ext.SDK docs/re/world.md). Rebuilt the way
// AIWorkspotManager::RegisterSpots does it: the verified DynArray_Realloc with element size 0x20, alignment 8, and the
// game's exported move callback when the array is not empty. Fails closed (does nothing) if either is unavailable.
inline bool Reserve(Red::SortedArray<Red::AISpotPersistentData>* aArray, uint32_t aCapacity)
{
    using Move_t = void (*)(void* aDst, void* aSrc, uint32_t aCount, const void* aSrcArray);
    using Realloc_t = void (*)(void* aArray, uint32_t aCapacity, uint32_t aElementSize, uint32_t aAlignment,
                               Move_t aMoveFunc);

    static_assert(sizeof(Red::AISpotPersistentData) == 0x20);

    static const auto s_move = reinterpret_cast<Move_t>(
        dlsym(RTLD_MAIN_ONLY, "_ZN3red8DynArrayIN2AI18SpotPersistentDataEE21MoveAfterReallocationEPvS4_jPKv"));
    static const Red::UniversalRelocFunc<Realloc_t> s_realloc(Red::Detail::AddressHashes::DynArray_Realloc);

    if (!aArray || !s_move || !s_realloc.IsValid())
        return false;

    s_realloc(aArray, aCapacity, 0x20, 8, aArray->size ? s_move : nullptr);
    return true;
}
}
#else
namespace Raw::AISpotPersistentDataArray
{
constexpr auto Reserve = Core::RawFunc<
    /* addr = */ Red::AddressLib::AISpotPersistentDataArray_Reserve,
    /* type = */ void (*)(Red::SortedArray<Red::AISpotPersistentData>* aArray, uint32_t aCapacity)>();
}
#endif

namespace Raw::AIWorkspotManager
{
using Spots = Core::OffsetPtr<0x48, Red::SortedArray<Red::AISpotPersistentData>>;

constexpr auto RegisterSpots = Core::RawFunc<
    /* addr = */ Red::AddressLib::AIWorkspotManager_RegisterSpots,
    /* type = */ void (*)(Red::AIWorkspotManager* aManager, const Red::DynArray<Red::AISpotPersistentData>& aSpots)>();
}
