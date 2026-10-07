#pragma once

#include "Red/Common.hpp"

namespace Red
{
#ifdef __APPLE__
// macOS: each field is 0x20 earlier than on Windows (SpawnFromRecord 0x101408860 stores the record ID at +0xC0, and
// the spawn params take the appearance from +0xA0; see RED4ext.SDK docs/re/pass2.md).
struct EntitySpawnerRequest
{
    uint8_t unk00[0xA0];        // 00
    CName appearanceName;       // A0
    uint8_t unkA8[0xC0 - 0xA8]; // A8
    TweakDBID recordID;         // C0
};
RED4EXT_ASSERT_OFFSET(EntitySpawnerRequest, appearanceName, 0xA0);
RED4EXT_ASSERT_OFFSET(EntitySpawnerRequest, recordID, 0xC0);
#else
struct EntitySpawnerRequest
{
    uint8_t unk00[0xC0];        // 00
    CName appearanceName;       // C0
    uint8_t unkC8[0xE0 - 0xC8]; // C8
    TweakDBID recordID;         // E0
};
RED4EXT_ASSERT_OFFSET(EntitySpawnerRequest, appearanceName, 0xC0);
RED4EXT_ASSERT_OFFSET(EntitySpawnerRequest, recordID, 0xE0);
#endif
}

namespace Raw::EntitySpawner
{
#ifdef __APPLE__
// macOS: Ticket(x8 out, 8 bytes; spawner x0, request* x1, ResourcePath x2). Only [x8] (one pointer) is written.
using SpawnTicket = Red::SretValue<8>;

constexpr auto SpawnFromTemplate = Core::RawFunc<
    /* addr = */ Red::AddressLib::EntitySpawner_SpawnFromTemplate,
    /* type = */ SpawnTicket (*)(void* aSpawner, Red::EntitySpawnerRequest* aRequest, Red::ResourcePath aTemplate)>();
#else
constexpr auto SpawnFromTemplate = Core::RawFunc<
    /* addr = */ Red::AddressLib::EntitySpawner_SpawnFromTemplate,
    /* type = */ void* (*)(void* aSpawner, void* aOut, Red::EntitySpawnerRequest* aRequest,
                           Red::ResourcePath aTemplate)>();
#endif
}
