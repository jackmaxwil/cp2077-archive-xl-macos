#pragma once

#include "Red/Common.hpp"
#include "Red/Entity.hpp"

namespace Red
{
using EntityTemplate = ent::EntityTemplate;
using TemplateAppearance = ent::TemplateAppearance;
using AppearanceDefinition = appearance::AppearanceDefinition;
using AppearancePartOverrides = appearance::AppearancePartOverrides;

struct GarmentAssembler {};

struct GarmentAssemblerState
{
    uintptr_t unk00; // 00
    uintptr_t unk08; // 08
    uintptr_t unk10; // 10
};
RED4EXT_ASSERT_SIZE(GarmentAssemblerState, 0x18);

struct GarmentItemAddRequest
{
    Handle<AppearanceDefinition> apperance; // 00
    uint64_t hash;                          // 10
    int32_t offset;                         // 18
};
RED4EXT_ASSERT_SIZE(GarmentItemAddRequest, 0x20);
RED4EXT_ASSERT_OFFSET(GarmentItemAddRequest, apperance, 0x00);
RED4EXT_ASSERT_OFFSET(GarmentItemAddRequest, hash, 0x10);
RED4EXT_ASSERT_OFFSET(GarmentItemAddRequest, offset, 0x18);

struct GarmentItemAddCustomRequest
{
    Handle<AppearanceDefinition> apperance;      // 00
    DynArray<AppearancePartOverrides> overrides; // 10
    uint64_t hash;                               // 20
    int32_t offset;                              // 28
};
RED4EXT_ASSERT_SIZE(GarmentItemAddCustomRequest, 0x30);
RED4EXT_ASSERT_OFFSET(GarmentItemAddCustomRequest, apperance, 0x00);
RED4EXT_ASSERT_OFFSET(GarmentItemAddCustomRequest, overrides, 0x10);
RED4EXT_ASSERT_OFFSET(GarmentItemAddCustomRequest, hash, 0x20);
RED4EXT_ASSERT_OFFSET(GarmentItemAddCustomRequest, offset, 0x28);

struct GarmentItemChangeRequest
{
    Handle<AppearanceDefinition> apperance; // 00
    uint64_t hash;                          // 10
};
RED4EXT_ASSERT_SIZE(GarmentItemChangeRequest, 0x18);
RED4EXT_ASSERT_OFFSET(GarmentItemChangeRequest, apperance, 0x00);
RED4EXT_ASSERT_OFFSET(GarmentItemChangeRequest, hash, 0x10);

struct GarmentItemChangeCustomRequest
{
    Handle<AppearanceDefinition> apperance;      // 00
    DynArray<AppearancePartOverrides> overrides; // 10
    uint64_t hash;                               // 20
};
RED4EXT_ASSERT_SIZE(GarmentItemChangeCustomRequest, 0x28);
RED4EXT_ASSERT_OFFSET(GarmentItemChangeCustomRequest, apperance, 0x00);
RED4EXT_ASSERT_OFFSET(GarmentItemChangeCustomRequest, overrides, 0x10);
RED4EXT_ASSERT_OFFSET(GarmentItemChangeCustomRequest, hash, 0x20);

struct GarmentItemRemoveRequest
{
    uint64_t hash; // 00
};
RED4EXT_ASSERT_SIZE(GarmentItemRemoveRequest, 0x8);
RED4EXT_ASSERT_OFFSET(GarmentItemRemoveRequest, hash, 0x0);

struct GarmentProcessingContext
{
    SharedPtr<void> unk00;                               // 00
    DynArray<Handle<ent::IComponent>> components;        // 10
    DynArray<Handle<CMesh>> meshes;                      // 20
    DynArray<Handle<ent::EntityTemplate>> templates;     // 30
    DynArray<ResourcePath> resources;                    // 40
    uint64_t unk50;                                      // 50
    Handle<appearance::AppearanceDefinition> definition; // 58
    EntityInitializeRequest request;                     // 68
    JobHandle job;                                       // 138
    int32_t unk140;                                      // 140
};

struct GarmentLoadingParams
{
    Handle<Entity> entity; // 00
};

struct GarmentExtractionParams
{
    Handle<entEntityTemplate> partTemplate;      // 00
    SharedPtr<GarmentProcessingContext> context; // 10
};
}

namespace Raw::GarmentAssembler
{
#ifdef __APPLE__
// macOS: GarmentAssemblerState(x8 out 0x18; aggregator x0, WeakHandle<Entity>& x1).
constexpr auto FindState = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_FindState,
    /* type = */ Red::GarmentAssemblerState (*)(uintptr_t, Red::WeakHandle<Red::ent::Entity>&)>();
#else
constexpr auto FindState = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_FindState,
    /* type = */ uintptr_t (*)(uintptr_t, Red::GarmentAssemblerState* aOut, Red::WeakHandle<Red::ent::Entity>&)>();
#endif

constexpr auto RemoveItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_RemoveItem,
    /* type = */ bool (*)(uintptr_t, Red::WeakHandle<Red::ent::Entity>&, Red::GarmentItemRemoveRequest&)>();

#ifdef __APPLE__
// macOS: SharedPtr<GarmentProcessingContext>(x8 out; const Handle<AppearanceDefinition>& x0, opts x1, params* x2).
// The Windows aProcessor is the MSVC hidden return. The result is forwarded as an opaque {instance, refcount} pair:
// ArchiveXL never owns the context, so it must not be able to destroy it (its allocator is the game's).
using GarmentProcessorPtr = Red::SretValue<0x10>;

inline Red::GarmentProcessingContext* GetProcessor(const GarmentProcessorPtr& aPtr)
{
    Red::GarmentProcessingContext* instance;
    std::memcpy(&instance, aPtr.data, sizeof(instance));
    return instance;
}

constexpr auto ProcessGarment = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_ProcessGarment,
    /* type = */ GarmentProcessorPtr (*)(const Red::Handle<Red::AppearanceDefinition>& aDefinition, uintptr_t a2,
                                         Red::GarmentLoadingParams* aParams)>();

// macOS: the state-level ChangeItem/ChangeCustomItem are inlined into these aggregator-level wrappers
// (0x1036F87E4 / 0x1036F890C), which take (aggregator, WeakHandle<Entity>&, request&).
constexpr auto ChangeItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_ChangeItem,
    /* type = */ bool (*)(uintptr_t, Red::WeakHandle<Red::ent::Entity>&, Red::GarmentItemChangeRequest&)>();

constexpr auto ChangeCustomItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_ChangeCustomItem,
    /* type = */ bool (*)(uintptr_t, Red::WeakHandle<Red::ent::Entity>&, Red::GarmentItemChangeCustomRequest&)>();
#else
constexpr auto ProcessGarment = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_ProcessGarment,
    /* type = */ uintptr_t (*)(Red::SharedPtr<Red::GarmentProcessingContext>& aProcessor, uintptr_t a2, uintptr_t a3,
                               Red::GarmentLoadingParams* aParams)>();
#endif

constexpr auto ExtractComponentsJob = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_ExtractComponentsJob,
    /* type = */ void (*)(Red::GarmentExtractionParams* aParams, const Red::JobGroup& aJobGroup)>();

using ProcessMesh = void (*)(Red::GarmentProcessingContext* aProcessor,
                             uint32_t aIndex,
                             const Red::Handle<Red::EntityTemplate>& aPartTemplate,
                             const Red::SharedPtr<Red::ResourceToken<>>& aMeshToken,
                             const Red::Handle<Red::IComponent>& aComponent,
                             const Red::JobGroup& aJobGroup);

constexpr auto ProcessSkinnedMesh = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_ProcessSkinnedMesh,
    /* type = */ ProcessMesh>();

constexpr auto ProcessMorphedMesh = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_ProcessMorphedMesh,
    /* type = */ ProcessMesh>();

constexpr auto OnGameDetach = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssembler_OnGameDetach,
    /* type = */ void (*)(uintptr_t)>();
}

namespace Raw::GarmentAssemblerState
{
constexpr auto AddItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssemblerState_AddItem,
    /* type = */ bool (*)(Red::GarmentAssemblerState* aState, Red::GarmentItemAddRequest&)>();

constexpr auto AddCustomItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssemblerState_AddCustomItem,
    /* type = */ bool (*)(Red::GarmentAssemblerState* aState, Red::GarmentItemAddCustomRequest&)>();

#ifndef __APPLE__
constexpr auto ChangeItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssemblerState_ChangeItem,
    /* type = */ bool (*)(Red::GarmentAssemblerState* aState, Red::GarmentItemChangeRequest&)>();

constexpr auto ChangeCustomItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::GarmentAssemblerState_ChangeCustomItem,
    /* type = */ bool (*)(Red::GarmentAssemblerState* aState, Red::GarmentItemChangeCustomRequest&)>();
#endif
}
