#pragma once

#include "Red/Common.hpp"

#ifdef __APPLE__
// The sync spawn hooks return Handle<WidgetLibraryItemInstance> by value, which needs its members' types complete.
#include <RED4ext/Scripting/Natives/Generated/ink/IEffect.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/Layer.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/PropertyManager.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/StyleResourceWrapper.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/UserData.hpp>
#endif

namespace Red
{
struct InkSpawningRequest
{
    uint8_t unk00[0x48];                             // 00
    CName itemName;                                  // 48
    WeakHandle<ink::Widget> parentWidget;            // 50
    Handle<ink::Widget> rootWidget;                  // 60
    Handle<ink::IWidgetController> gameController;   // 70
    Handle<ink::WidgetLibraryResource> library;      // 80
    Handle<ink::WidgetLibraryItemInstance> instance; // 90
    ResourcePath externalLibrary;                    // A0
    // bool flag;                                    // F8
    // uint8_t status;                               // 184 (Windows), 1AC (macOS, set to 3 by FinishAsyncSpawn)
};
RED4EXT_ASSERT_OFFSET(InkSpawningRequest, itemName, 0x48);
RED4EXT_ASSERT_OFFSET(InkSpawningRequest, rootWidget, 0x60);

struct InkSpawningContext
{
    virtual void sub_00() = 0;
    virtual void sub_08() = 0;
    virtual void sub_10() = 0;
    virtual void sub_18() = 0;
    virtual void sub_20() = 0;
    virtual void sub_28() = 0;

    SharedPtr<void>               unk08;   // 08
    SharedPtr<InkSpawningRequest> request; // 18
};
RED4EXT_ASSERT_SIZE(InkSpawningContext, 0x28);
RED4EXT_ASSERT_OFFSET(InkSpawningContext, request, 0x18);

#ifdef __APPLE__
// macOS: the context pointer is at +0x18 (see RED4ext.SDK docs/re/world.md). The size is not known; ArchiveXL only
// ever receives this struct by reference.
struct InkSpawningInfo
{
    uint8_t unk00[0x18];         // 00
    InkSpawningContext* context; // 18
};
RED4EXT_ASSERT_OFFSET(InkSpawningInfo, context, 0x18);
#else
struct InkSpawningInfo
{
    uint8_t unk00[0x38];         // 00
    InkSpawningContext* context; // 38
};
RED4EXT_ASSERT_SIZE(InkSpawningInfo, 0x40);
RED4EXT_ASSERT_OFFSET(InkSpawningInfo, context, 0x38);
#endif
}

#ifdef __APPLE__
// macOS signatures (RED4ext.SDK docs/re/world.md, pass2.md): the async variants take an extra bool, the sync variants
// return the item instance Handle through x8, and FinishAsyncSpawn returns void.
namespace Raw::InkWidgetLibrary
{
constexpr auto AsyncSpawnFromExternal = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkWidgetLibrary_AsyncSpawnFromExternal,
    /* type = */ bool (*)(
        Red::ink::WidgetLibraryResource& aLibrary,
        Red::InkSpawningInfo& aSpawningInfo,
        Red::ResourcePath aExternalPath,
        Red::CName aItemName,
        bool a5)>();

constexpr auto AsyncSpawnFromLocal = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkWidgetLibrary_AsyncSpawnFromLocal,
    /* type = */ bool (*)(
        Red::ink::WidgetLibraryResource& aLibrary,
        Red::InkSpawningInfo& aSpawningInfo,
        Red::CName aItemName,
        bool a4)>();

constexpr auto SpawnFromExternal = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkWidgetLibrary_SpawnFromExternal,
    /* type = */ Red::Handle<Red::ink::WidgetLibraryItemInstance> (*)(
        Red::ink::WidgetLibraryResource& aLibrary,
        Red::ResourcePath aExternalPath,
        Red::CName aItemName)>();

constexpr auto SpawnFromLocal = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkWidgetLibrary_SpawnFromLocal,
    /* type = */ Red::Handle<Red::ink::WidgetLibraryItemInstance> (*)(
        Red::ink::WidgetLibraryResource& aLibrary,
        Red::CName aItemName)>();
}

namespace Raw::InkSpawner
{
constexpr auto FinishAsyncSpawn = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkSpawner_FinishAsyncSpawn,
    /* type = */ void (*)(
        Red::InkSpawningContext& aContext,
        Red::Handle<Red::ink::WidgetLibraryItemInstance>& aInstance)>();
}
#else
namespace Raw::InkWidgetLibrary
{
constexpr auto AsyncSpawnFromExternal = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkWidgetLibrary_AsyncSpawnFromExternal,
    /* type = */ bool (*)(
        Red::ink::WidgetLibraryResource& aLibrary,
        Red::InkSpawningInfo& aSpawningInfo,
        Red::ResourcePath aExternalPath,
        Red::CName aItemName)>();

constexpr auto AsyncSpawnFromLocal = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkWidgetLibrary_AsyncSpawnFromLocal,
    /* type = */ bool (*)(
        Red::ink::WidgetLibraryResource& aLibrary,
        Red::InkSpawningInfo& aSpawningInfo,
        Red::CName aItemName)>();

constexpr auto SpawnFromExternal = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkWidgetLibrary_SpawnFromExternal,
    /* type = */ uintptr_t (*)(
        Red::ink::WidgetLibraryResource& aLibrary,
        Red::Handle<Red::ink::WidgetLibraryItemInstance>& aInstance,
        Red::ResourcePath aExternalPath,
        Red::CName aItemName)>();

constexpr auto SpawnFromLocal = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkWidgetLibrary_SpawnFromLocal,
    /* type = */ uintptr_t (*)(
        Red::ink::WidgetLibraryResource& aLibrary,
        Red::Handle<Red::ink::WidgetLibraryItemInstance>& aInstance,
        Red::CName aItemName)>();
}

namespace Raw::InkSpawner
{
constexpr auto FinishAsyncSpawn = Core::RawFunc<
    /* addr = */ Red::AddressLib::InkSpawner_FinishAsyncSpawn,
    /* type = */ bool (*)(
        Red::InkSpawningContext& aContext,
        Red::Handle<Red::ink::WidgetLibraryItemInstance>& aInstance)>();
}
#endif
