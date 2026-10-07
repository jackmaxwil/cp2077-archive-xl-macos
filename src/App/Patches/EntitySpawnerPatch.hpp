#pragma once

#include "Core/Foundation/Feature.hpp"
#include "Core/Hooking/HookingAgent.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/EntitySpawner.hpp"

namespace App
{
class EntitySpawnerPatch
    : public Core::Feature
    , public Core::HookingAgent
    , public Core::LoggingAgent
{
protected:
    void OnBootstrap() override;

#ifdef __APPLE__
    static void OnSpawnEntity(void* aSpawner, Red::EntitySpawnerRequest* aRequest, Red::ResourcePath aTemplate);
#else
    static void OnSpawnEntity(void* aSpawner, void* aOut, Red::EntitySpawnerRequest* aRequest,
                              Red::ResourcePath aTemplate);
#endif
};
}
