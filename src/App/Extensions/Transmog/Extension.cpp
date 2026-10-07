#include "Extension.hpp"
#include "App/Extensions/Garment/Dynamic.hpp"
#include "Red/FactoryIndex.hpp"
#include "Red/TweakDB.hpp"

namespace
{
constexpr auto ExtensionName = "Transmog";

constexpr auto ItemEntityOffset = 0xD0; // todo: use OffsetPtr
constexpr auto EntityPathOffset = 0x60;
constexpr auto EntityNameFlat = ".entityName";
constexpr auto RecordOffset = 0x100;
constexpr auto FactoryOffset = 0x120;
}

std::string_view App::TransmogExtension::GetName()
{
    return ExtensionName;
}

bool App::TransmogExtension::Load()
{
    Hook<Raw::ItemFactoryAppearanceChangeRequest::LoadTemplate>(&OnLoadTemplate).OrThrow();
    Hook<Raw::AppearanceChanger::SelectAppearanceName>(&OnSelectAppearance).OrThrow();

    return true;
}

bool App::TransmogExtension::Unload()
{
    Unhook<Raw::ItemFactoryAppearanceChangeRequest::LoadTemplate>();
    Unhook<Raw::AppearanceChanger::SelectAppearanceName>();

    return true;
}

bool App::TransmogExtension::OnLoadTemplate(Red::ItemFactoryAppearanceChangeRequest* aRequest)
{
    Red::ResourcePath originalPath;

#ifdef __APPLE__
    // macOS: +0x120 is a WeakHandle whose target type is not verified (LoadTemplate 0x1036FCA88 only checks that it
    // locks), so it is not used as a factory index here; the template override is skipped (fail closed).
    return Raw::ItemFactoryAppearanceChangeRequest::LoadTemplate(aRequest);
#endif

    auto targetEntity = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(aRequest) + ItemEntityOffset);
    auto visualRecord = *reinterpret_cast<Red::gamedataTweakDBRecord**>(aRequest + RecordOffset);
    auto entityFactory = *reinterpret_cast<uintptr_t*>(aRequest + FactoryOffset);

    if (entityFactory && targetEntity && visualRecord)
    {
        auto entityFlat = Red::GetFlatPtr<Red::CName>({visualRecord->recordID, EntityNameFlat});
        if (entityFlat)
        {
#ifndef NDEBUG
            auto recordName = Red::ToStringDebug(visualRecord->recordID);
            auto factoryName = entityFlat->ToString();
#endif

#ifdef __APPLE__
            const auto overridePath = Raw::FactoryIndex::ResolveResource(entityFactory, *entityFlat);
#else
            Red::ResourcePath overridePath;
            Raw::FactoryIndex::ResolveResource(entityFactory, overridePath, *entityFlat);
#endif

            if (overridePath)
            {
                originalPath = *reinterpret_cast<Red::ResourcePath*>(targetEntity + EntityPathOffset);
                *reinterpret_cast<Red::ResourcePath*>(targetEntity + EntityPathOffset) = overridePath;
            }
        }
    }

    bool result = Raw::ItemFactoryAppearanceChangeRequest::LoadTemplate(aRequest);

    if (originalPath)
    {
        *reinterpret_cast<Red::ResourcePath*>(targetEntity + EntityPathOffset) = originalPath;
    }

    return result;
}

#ifdef __APPLE__
// macOS: the CName comes back in x0 and there is no out argument (see RED4ext.SDK docs/re/appearance.md).
Red::CName App::TransmogExtension::OnSelectAppearance(const Red::Handle<Red::TweakDBRecord>& aItemRecord,
                                                      const Red::ItemID& aItemID,
                                                      const Red::Handle<Red::AppearanceResource>& aAppearanceResource,
                                                      uint64_t a5, Red::CName aAppearanceName)
{
    if (!aAppearanceName && aItemRecord && aAppearanceResource)
    {
        if (auto name = FindRecordAppearance(aItemRecord, aAppearanceResource))
            return name;
    }

    return Raw::AppearanceChanger::SelectAppearanceName(aItemRecord, aItemID, aAppearanceResource, a5,
                                                        aAppearanceName);
}
#endif

Red::CName App::TransmogExtension::FindRecordAppearance(const Red::Handle<Red::TweakDBRecord>& aItemRecord,
                                                        const Red::Handle<Red::AppearanceResource>& aAppearanceResource)
{
    {
        auto appearanceName = Red::GetFlatValue<Red::CName>({aItemRecord->recordID, ".appearanceName"});
        if (appearanceName)
        {
            for (const auto& appearanceDefinition : aAppearanceResource->appearances)
            {
                if (appearanceDefinition->name == appearanceName)
                    return appearanceName;
            }
        }
    }
    {
        auto visualTags = Red::GetFlatPtr<Red::DynArray<Red::CName>>({aItemRecord->recordID, ".visualTags"});
        if (visualTags && visualTags->size == 1)
        {
            auto& appearanceName = visualTags->entries[0];
            for (const auto& appearanceDefinition : aAppearanceResource->appearances)
            {
                if (appearanceDefinition->name == appearanceName)
                    return appearanceName;
            }
        }
    }

    return {};
}

#ifndef __APPLE__
void* App::TransmogExtension::OnSelectAppearance(Red::CName* aOut,
                                                    const Red::Handle<Red::TweakDBRecord>& aItemRecord,
                                                    const Red::ItemID& aItemID,
                                                    const Red::Handle<Red::AppearanceResource>& aAppearanceResource,
                                                    uint64_t a5, Red::CName aAppearanceName)
{
    if (!aAppearanceName)
    {
        {
            auto appearanceName = Red::GetFlatValue<Red::CName>({aItemRecord->recordID, ".appearanceName"});
            if (appearanceName)
            {
                for (const auto& appearanceDefinition : aAppearanceResource->appearances)
                {
                    if (appearanceDefinition->name == appearanceName)
                    {
                        *aOut = appearanceName;
                        return aOut;
                    }
                }
            }
        }
        {
            auto visualTags = Red::GetFlatPtr<Red::DynArray<Red::CName>>({aItemRecord->recordID, ".visualTags"});
            if (visualTags && visualTags->size == 1)
            {
                auto& appearanceName = visualTags->entries[0];
                for (const auto& appearanceDefinition : aAppearanceResource->appearances)
                {
                    if (appearanceDefinition->name == appearanceName)
                    {
                        *aOut = appearanceName;
                        return aOut;
                    }
                }
            }
        }
    }

    return Raw::AppearanceChanger::SelectAppearanceName(aOut, aItemRecord, aItemID, aAppearanceResource,
                                                        a5, aAppearanceName);
}
#endif
