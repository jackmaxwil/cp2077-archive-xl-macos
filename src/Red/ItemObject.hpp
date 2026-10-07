#pragma once

namespace Raw::ItemObject
{
using ItemID = Core::OffsetPtr<0x288, Red::ItemID>;

#ifdef __APPLE__
// macOS: vtable 0x1071FA570 +0x288 -> 0x10373A494 (`ldr x0, [x0, #0x50]; ret`), the CName comes back in x0.
constexpr auto GetAppearanceName = Core::RawVFunc<
    /* offset = */ 0x280,
    /* type = */ Red::CName (Red::ItemObject::*)()>();
#else
constexpr auto GetAppearanceName = Core::RawVFunc<
    /* offset = */ 0x280,
    /* type = */ Red::CName* (Red::ItemObject::*)(Red::CName& aOut)>();
#endif

inline Red::CName GetItemAppearanceName(Red::ItemObject* aItemObject)
{
#ifdef __APPLE__
    return GetAppearanceName(aItemObject);
#else
    Red::CName appearanceName;
    GetAppearanceName(aItemObject, appearanceName);
    return appearanceName;
#endif
}
}
