#pragma once

#include "Red/Common.hpp"
#include "Red/EntityTemplate.hpp"
#include "Red/GarmentAssembler.hpp"

namespace Red
{
using EntityTemplate = Red::ent::EntityTemplate;
using ComponentsStorage = Red::ent::ComponentsStorage;
using AppearanceResource = Red::appearance::AppearanceResource;
using AppearanceDefinition = Red::appearance::AppearanceDefinition;

template<typename T>
using ResourceTokenPtr = Red::SharedPtr<Red::ResourceToken<T>>;

struct ItemFactoryRequest {};

struct ItemFactoryAppearanceChangeRequest {};

struct AppearanceDescriptor
{
    using Hash = uint64_t;

    inline operator bool() const noexcept
    {
        return resource && appearance;
    }

    inline operator Hash() const noexcept
    {
        return Red::FNV1a64(reinterpret_cast<const uint8_t*>(&appearance.hash), sizeof(appearance.hash), resource.hash);
    }

    ResourcePath resource; // 00
    CName appearance;      // 08
};
RED4EXT_ASSERT_SIZE(AppearanceDescriptor, 0x10);
RED4EXT_ASSERT_OFFSET(AppearanceDescriptor, resource, 0x0);
RED4EXT_ASSERT_OFFSET(AppearanceDescriptor, appearance, 0x8);

struct AppearanceChangeRequest
{
    WeakHandle<gamePuppet> puppet;      // 00
    AppearanceDescriptor oldAppearance; // 10
    AppearanceDescriptor newAppearance; // 20
    uint64_t unk30;                     // 30
};
RED4EXT_ASSERT_OFFSET(AppearanceChangeRequest, oldAppearance, 0x10);
RED4EXT_ASSERT_OFFSET(AppearanceChangeRequest, newAppearance, 0x20);
}

namespace Raw::AppearanceChanger
{
#ifdef __APPLE__
// macOS: CString(x8 out; owner& x0, ownerOverride& x1, const Handle<Item_Record>& x2, const ItemID& x3).
// GetSuffixValue does not exist on macOS: it is inlined into GetSuffixes (see RED4ext.SDK docs/re/appearance.md).
constexpr auto GetSuffixes = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChanger_GetSuffixes,
    /* type = */ Red::CString (*)(Red::Handle<Red::GameObject>& aOwner,
                                  Red::Handle<Red::GameObject>& aOwnerOverride,
                                  const Red::Handle<Red::TweakDBRecord>& aItemRecord,
                                  const Red::ItemID& aItemID)>();
#else
constexpr auto GetSuffixes = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChanger_GetSuffixes,
    /* type = */ void* (*)(Red::CString& aResult,
                           Red::Handle<Red::GameObject>& aOwner,
                           Red::Handle<Red::GameObject>& aOwnerOverride,
                           const Red::TweakDBRecord& aItemRecord,
                           const Red::ItemID& aItemID)>();

constexpr auto GetSuffixValue = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChanger_GetSuffixValue,
    /* type = */ bool (*)(const Red::ItemID& aItemID,
                          uint64_t a2, // Must be non-zero
                          Red::Handle<Red::GameObject>& aOwner,
                          Red::TweakDBID aSuffixRecordID,
                          Red::CString& aResult)>();
using GetSuffixValuePtr = decltype(GetSuffixValue)::Callable;
#endif

constexpr auto RegisterPart = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChanger_RegisterPart,
    /* type = */ void (*)(uintptr_t,
                          Red::Handle<Red::EntityTemplate>& aPart,
                          Red::Handle<Red::ComponentsStorage>& aComponents,
                          Red::Handle<Red::AppearanceDefinition>& aAppearance)>();

constexpr auto GetBaseMeshOffset = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChanger_GetBaseMeshOffset,
    /* type = */ int32_t (*)(Red::Handle<Red::IComponent>& aComponent,
                             Red::Handle<Red::EntityTemplate>& aTemplate)>();

constexpr auto ComputePlayerGarment = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChanger_ComputePlayerGarment,
    /* type = */ void (*)(uintptr_t a1,
                          Red::Handle<Red::ent::Entity>& aEntity,
                          Red::DynArray<int32_t>& aOffsets,
                          Red::SharedPtr<Red::GarmentProcessingContext>& aData,
                          uintptr_t a5,
                          uintptr_t a6,
                          uintptr_t a7,
                          bool a8)>();

#ifdef __APPLE__
// macOS: CName returned in x0, no out argument (record& x0, ItemID& x1, Handle<AppRes>& x2, a5 x3, name x4).
constexpr auto SelectAppearanceName = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChanger_SelectAppearanceName,
    /* type = */ Red::CName (*)(const Red::Handle<Red::TweakDBRecord>& aItemRecord,
                                const Red::ItemID& aItemID,
                                const Red::Handle<Red::AppearanceResource>& aAppearanceResource,
                                uint64_t a5,
                                Red::CName aAppearanceName)>();
#else
constexpr auto SelectAppearanceName = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChanger_SelectAppearanceName,
    /* type = */ void* (*)(Red::CName* aOut,
                           const Red::Handle<Red::TweakDBRecord>& aItemRecord,
                           const Red::ItemID& aItemID,
                           const Red::Handle<Red::AppearanceResource>& aAppearanceResource,
                           uint64_t a5,
                           Red::CName aAppearanceName)>();
#endif
}

namespace Raw::RuntimeSystemEntityAppearanceChanger
{
#ifdef __APPLE__
// macOS: (system, request* x1, callback* x2, x3); every argument is forwarded to the original.
constexpr auto ChangeAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChangeSystem_ChangeAppearance1,
    /* type = */ void (*)(Red::world::RuntimeSystemEntityAppearanceChanger& aSystem,
                          Red::AppearanceChangeRequest* aRequest,
                          uintptr_t a3,
                          uintptr_t a4)>();

// macOS: both ranges arrive as raw begin/end pointers in x2..x5, a5 in x6 and a6 in w7.
constexpr auto ChangeAppearances = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChangeSystem_ChangeAppearance2,
    /* type = */ void (*)(Red::world::RuntimeSystemEntityAppearanceChanger& aSystem,
                          Red::WeakHandle<Red::game::Puppet>& aTarget,
                          Red::AppearanceDescriptor* aOldBegin,
                          Red::AppearanceDescriptor* aOldEnd,
                          Red::AppearanceDescriptor* aNewBegin,
                          Red::AppearanceDescriptor* aNewEnd,
                          uintptr_t a5,
                          uint8_t a6)>();
#else
constexpr auto ChangeAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChangeSystem_ChangeAppearance1,
    /* type = */ void (*)(Red::world::RuntimeSystemEntityAppearanceChanger& aSystem,
                          Red::AppearanceChangeRequest* aRequest,
                          uintptr_t a3)>();

constexpr auto ChangeAppearances = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceChangeSystem_ChangeAppearance2,
    /* type = */ void (*)(Red::world::RuntimeSystemEntityAppearanceChanger& aSystem,
                          Red::Handle<Red::game::Puppet>& aTarget,
                          Red::Range<Red::AppearanceDescriptor>& aOldApp,
                          Red::Range<Red::AppearanceDescriptor>& aNewApp,
                          uintptr_t a5,
                          uint8_t a6)>();
#endif
}


namespace Raw::ItemFactoryRequest
{
using Entity = Core::OffsetPtr<0x28, Red::WeakHandle<Red::Entity>>;
using EntityTemplate = Core::OffsetPtr<0x138, Red::ResourceTokenPtr<Red::EntityTemplate>>;
using AppearanceToken = Core::OffsetPtr<0x148, Red::ResourceTokenPtr<Red::AppearanceResource>>;
using AppearanceName = Core::OffsetPtr<0x158, Red::CName>;
using ItemRecord = Core::OffsetPtr<0x160, Red::gamedataTweakDBRecord*>;

constexpr auto LoadAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::ItemFactoryRequest_LoadAppearance,
    /* type = */ bool (*)(Red::ItemFactoryRequest* aRequest)>();

// constexpr auto ApplyAppearance = Core::RawFunc<
//     /* addr = */ Red::AddressLib::ItemFactoryRequest_ApplyAppearance,
//     /* type = */ bool (*)(Red::ItemFactoryRequest* aRequest)>();
};

namespace Raw::ItemFactoryAppearanceChangeRequest
{
using Entity = Core::OffsetPtr<0x80, Red::WeakHandle<Red::Entity>>;
using EntityTemplate = Core::OffsetPtr<0xA8, Red::ResourceTokenPtr<Red::EntityTemplate>>;
using AppearanceToken = Core::OffsetPtr<0xB8, Red::ResourceTokenPtr<Red::AppearanceResource>>;
using AppearanceName = Core::OffsetPtr<0x48, Red::CName>;
using ItemRecord = Core::OffsetPtr<0x100, Red::gamedataTweakDBRecord*>;

constexpr auto LoadTemplate = Core::RawFunc<
    /* addr = */ Red::AddressLib::ItemFactoryAppearanceChangeRequest_LoadTemplate,
    /* type = */ bool (*)(Red::ItemFactoryAppearanceChangeRequest* aRequest)>();

constexpr auto LoadAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::ItemFactoryAppearanceChangeRequest_LoadAppearance,
    /* type = */ bool (*)(Red::ItemFactoryAppearanceChangeRequest* aRequest)>();

// constexpr auto ApplyAppearance = Core::RawFunc<
//     /* addr = */ Red::AddressLib::ItemFactoryAppearanceChangeRequest_ApplyAppearance,
//     /* type = */ bool (*)(Red::ItemFactoryAppearanceChangeRequest* aRequest)>();
}
