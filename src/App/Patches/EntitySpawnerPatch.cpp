#include "EntitySpawnerPatch.hpp"
#include "Red/TweakDB.hpp"

namespace
{
constexpr auto DefaultAppearanceName = Red::CName("default");
}

void App::EntitySpawnerPatch::OnBootstrap()
{
    HookBefore<Raw::EntitySpawner::SpawnFromTemplate>(&OnSpawnEntity).OrThrow();
}

#ifdef __APPLE__
void App::EntitySpawnerPatch::OnSpawnEntity(void* aSpawner, Red::EntitySpawnerRequest* aRequest,
                                            Red::ResourcePath aTemplate)
#else
void App::EntitySpawnerPatch::OnSpawnEntity(void* aSpawner, void* aOut, Red::EntitySpawnerRequest* aRequest,
                                            Red::ResourcePath aTemplate)
#endif
{
    if (!aRequest)
        return;

    if (aRequest->recordID && (!aRequest->appearanceName || aRequest->appearanceName == DefaultAppearanceName))
    {
        auto recordAppearanceName = Red::GetFlatValue<Red::CName>({aRequest->recordID, ".appearanceName"});
        if (recordAppearanceName)
        {
            aRequest->appearanceName = recordAppearanceName;
        }
    }
}
