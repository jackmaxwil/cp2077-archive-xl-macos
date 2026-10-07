#pragma once

namespace Raw::TPPRepresentationComponent
{
using SlotListener = Core::OffsetPtr<0x148, Red::Handle<Red::game::IAttachmentSlotsListener>>;

constexpr auto OnAttach = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_OnAttach,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent, uintptr_t a2)>();

constexpr auto RegisterAffectedItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_RegisterAffectedItem,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent,
                          Red::TweakDBID aItemID,
                          const Red::Handle<Red::ItemObject>& aItemObject)>();

#ifdef __APPLE__
// macOS: IsAffectedSlot is inlined into the slot listener handlers below (a Head/Eyes check on the slot ID), see
// RED4ext.SDK docs/re/appearance.md. The listener thunks call them as (component, item TweakDBID, slot TweakDBID).
constexpr auto OnItemEquipped = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_OnItemEquipped,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent,
                          Red::TweakDBID aItemID,
                          Red::TweakDBID aSlotID)>();

constexpr auto OnItemUnequipped = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_OnItemUnequipped,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent,
                          Red::TweakDBID aItemID,
                          Red::TweakDBID aSlotID)>();

constexpr auto UnregisterAffectedItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_UnregisterAffectedItem,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent, Red::TweakDBID aItemID)>();
#else
constexpr auto IsAffectedSlot = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_IsAffectedSlot,
    /* type = */ bool (*)(Red::TweakDBID aSlotID)>();
#endif
}
