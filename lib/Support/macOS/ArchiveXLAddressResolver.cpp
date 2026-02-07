#include "ArchiveXLAddressResolver.hpp"
#include <mach-o/dyld.h>
#include <iostream>

namespace Support
{

std::unordered_map<uint32_t, int> ArchiveXLAddressResolver::s_requestedAddresses;

ArchiveXLAddressResolver::ArchiveXLAddressResolver()
{
    InitializeAddressTable();
}

void ArchiveXLAddressResolver::OnInitialize()
{
    AddressResolver::SetDefault(*this);
    std::cerr << "[ArchiveXLAddressResolver] Registered as default address resolver" << std::endl;
}

uintptr_t ArchiveXLAddressResolver::GetImageBase()
{
    static const uintptr_t base = reinterpret_cast<uintptr_t>(_dyld_get_image_header(0));
    return base;
}

void ArchiveXLAddressResolver::InitializeAddressTable()
{
    // =========================================================================
    // ArchiveXL Required Addresses (from src/Red/Addresses/Library.hpp)
    // =========================================================================
    // =========================================================================

    // Target game build: Cyberpunk 2077 macOS v2.3.1 (arm64)
    //
    // Offsets are relative to the main image base:
    //   resolved = image_base + offset

    m_addressTable.clear();
    m_addressTable.reserve(130);

    // Main (0x0E54032B)
    m_addressTable[240386859] = 0x31E18;

    // AISpotPersistentDataArray_Reserve (0xE50F1650)
    m_addressTable[3842971216] = 0x3040DEC;

    // AIWorkspotManager_RegisterSpots (0x95ED24DC)
    m_addressTable[2515346652] = 0x3DFFF44;

    // AnimatedComponent_InitializeAnimations (0xAA331635)
    m_addressTable[2855474741] = 0x24DF14C;

    // AppearanceChanger_ComputePlayerGarment (0xC152A90F)
    m_addressTable[3243419919] = 0xACA488;

    // AppearanceChanger_GetBaseMeshOffset (0xFB832A63)
    m_addressTable[4219677283] = 0xAD043C;

    // AppearanceChanger_GetSuffixes (0x03C22EF0)
    m_addressTable[63057648] = 0x2ABCDD0;

    // AppearanceChanger_GetSuffixValue (0x3BD02F1E)
    m_addressTable[1003499294] = 0x370E9AC;

    // AppearanceChanger_RegisterPart (0xBCE53BEF)
    m_addressTable[3169139695] = 0xCD2B90;

    // AppearanceChanger_SelectAppearanceName (0xA5233D59)
    m_addressTable[2770550105] = 0xAD043C;

    // AppearanceChangeSystem_ChangeAppearance1 (0x2BD73C8A)
    m_addressTable[735526026] = 0xAD043C;

    // AppearanceChangeSystem_ChangeAppearance2 (0x170E5679)
    m_addressTable[386815609] = 0xAD043C;

    // AppearanceDefinition_ExtractPartComponents (0x02563CB3)
    m_addressTable[39206067] = 0xCD2B90;

    // AppearanceNameVisualTagsPreset_GetVisualTags (0x46BD1B44)
    m_addressTable[1186798404] = 0x2BF8BD0;

    // AppearanceResource_OnLoad (0xBB431A21)
    m_addressTable[3141736993] = 0xACE140;

    // AppearanceResource_FindAppearanceDefinition (0x20BF2893)
    m_addressTable[549398675] = 0xAD043C;

    // AttachmentSlots_InitializeSlots (0xC0371F97)
    m_addressTable[3224838039] = 0x14E6D44;

    // AttachmentSlots_IsSlotEmpty (0xFC3E16A8)
    m_addressTable[4231927464] = 0x14E6D44;

    // AttachmentSlots_IsSlotSpawning (0x4C7C1B7E)
    m_addressTable[1283201918] = 0x14E6D44;

    // BufferReader_MakeType0 (0x4D6E4A12)
    m_addressTable[1299073554] = 0xBE6FA8;

    // BufferReader_MakeType1 (0xD5456975)
    m_addressTable[3578095989] = 0xBE6FA8;

    // CNamePool_RegisterName (0x25691379)
    m_addressTable[627643257] = 0x1E3ED04;

    // CNamePool_GetStringView (0xAB921110)
    m_addressTable[2878476560] = 0x1E3F8F4;

    // CBaseEngine_InitEngine (0xC3241A08)
    m_addressTable[3273923080] = 0x3D73CF8;

    // CBaseEngine_LoadGatheredResources (0xDE501230)
    m_addressTable[3729789488] = 0x3D9F0F8;

    // CharacterCustomizationFeetController_CheckState (0xB3BA2F12)
    m_addressTable[3015323410] = 0x423DE2C;

    // CharacterCustomizationGenitalsController_OnAttach (0x6A46257A)
    m_addressTable[1782982010] = 0x2DEAC58;

    // CharacterCustomizationGenitalsController_CheckState (0x8BE730CF)
    m_addressTable[2347184335] = 0x2DEAC58;

    // CharacterCustomizationHairstyleController_OnDetach (0x861A25BB)
    m_addressTable[2249860539] = 0x2DEBB90;

    // CharacterCustomizationHairstyleController_CheckState (0x9E1F3132)
    m_addressTable[2652844338] = 0x2DEBB90;

    // CharacterCustomizationHelper_GetHairColor (0xCB882EA4)
    m_addressTable[3414699684] = 0x1441630;

    // CharacterCustomizationState_FinalizePart (0x2AB14DCF)
    m_addressTable[716262863] = 0x3D783B8;

    // CharacterCustomizationState_GetHeadAppearances1 (0xF1773D53)
    m_addressTable[4051123539] = 0x28B94DC;

    // CharacterCustomizationState_GetHeadAppearances2 (0xE07A3494)
    m_addressTable[3766105236] = 0x28B94DC;

    // CharacterCustomizationState_GetBodyAppearances1 (0xEF4334B0)
    m_addressTable[4014159024] = 0x204218;

    // CharacterCustomizationState_GetBodyAppearances2 (0x035F3D6F)
    m_addressTable[56573295] = 0x204218;

    // CharacterCustomizationState_GetArmsAppearances1 (0x069A3D74)
    m_addressTable[110771572] = 0x4357924;

    // CharacterCustomizationState_GetArmsAppearances2 (0xF1F234B5)
    m_addressTable[4059182261] = 0x4357924;

    // CharacterCustomizationSystem_Initialize (0x8B8D4700)
    m_addressTable[2341291776] = 0x3F20314;

    // CharacterCustomizationSystem_Uninitialize (0x17F91F49)
    m_addressTable[402202441] = 0x3F98478;

    // CharacterCustomizationSystem_GetResource (0xFED0370E)
    m_addressTable[4275058446] = 0x21AC670;

    // CharacterCustomizationSystem_EnsureState (0x2EEA2FC4)
    m_addressTable[787099588] = 0x2F2D360;

    // CharacterCustomizationSystem_InitializeAppOption (0x4120715D)
    m_addressTable[1092645213] = 0x1E3F8F4;

    // CharacterCustomizationSystem_InitializeMorphOption (0xE11C6365)
    m_addressTable[3776734053] = 0x1E3F8F4;

    // CharacterCustomizationSystem_InitializeSwitcherOption (0x84DD7B39)
    m_addressTable[2229107513] = 0x1E3F8F4;

    // CharacterCustomizationSystem_InitializeOptionsFromState (0x45A51FC2)
    m_addressTable[1168449474] = 0x453C2D4;

    // CMesh_PostLoad (0x87741069)
    m_addressTable[2272530537] = 0x1DF1EC;

    // CMesh_GetAppearance (0x2E1A1ACD)
    m_addressTable[773462733] = 0xACAE08;

    // CMesh_FindAppearance (0xB33D1C7B)
    m_addressTable[3007126651] = 0xACAE08;

    // CMesh_LoadMaterialsAsync (0x29D24DC6)
    m_addressTable[701648326] = 0x23AC3C;

    // CMesh_AddStubAppearance (0x21190DEB)
    m_addressTable[555290091] = 0xACAE08;

    // CMesh_ShouldPreloadAppearances (0x1E0A17D7)
    m_addressTable[503977943] = 0xC27788;

    // MeshMaterialBuffer_LoadMaterialAsync (0x5FEF4FDC)
    m_addressTable[1609519068] = 0x3D73CF8;

    // Entity_Attach (0xFD3D12D9)
    m_addressTable[4248638169] = 0x180C9D8;

    // Entity_Detach (0x86ED095F)
    m_addressTable[2263681375] = 0x15EDE88;

    // Entity_Dispose (0x95EC09FD)
    m_addressTable[2515274237] = 0xCF5E98;

    // Entity_Initialize (0xD00D1A41)
    m_addressTable[3490519617] = 0x3641A50;

    // Entity_Assemble (0x82171553)
    m_addressTable[2182550867] = 0xCCEDEC;

    // Entity_Reassemble (0x5D0640A9)
    m_addressTable[1560690857] = 0xC95744;

    // Entity_Uninitialize (0xD65C0C1B)
    m_addressTable[3596356635] = 0xCC45EC;

    // Entity_RequestComponents (0x88DE290A)
    m_addressTable[2296260874] = 0xD4DC20;

    // EntityBuilder_ExtractComponentsJob (0x1D2D1648)
    m_addressTable[489494088] = 0xD4DC20;

    // EntityBuilder_ScheduleExtractComponentsJob (0x1A182B6A)
    m_addressTable[437791594] = 0xD4DC20;

    // EntitySpawner_SpawnFromTemplate (0x959224DE)
    m_addressTable[2509382878] = 0x33E4040;

    // EntityTemplate_OnLoad (0xA36615D9)
    m_addressTable[2741376473] = 0xAE4070;

    // EntityTemplate_FindAppearance (0x02321AA8)
    m_addressTable[36838056] = 0xC7493C;

    // FactoryIndex_LoadFactoryAsync (0x70771C5A)
    m_addressTable[1886854234] = 0x417704C;

    // FactoryIndex_ResolveResource (0xB53B19B5)
    m_addressTable[3040549301] = 0x7B4670;

    // GameApplication_InitResourceDepot (0xAE3B1D7B)
    m_addressTable[2923109755] = 0x1704194;

    // GarmentAssembler_FindState (0x9AA62D78)
    m_addressTable[2594581880] = 0x36FE44C;

    // GarmentAssembler_RemoveItem (0x6F162906)
    m_addressTable[1863723270] = 0x36FE44C;

    // GarmentAssembler_ProcessGarment (0x01BB5218)
    m_addressTable[29053464] = 0xAFA378;

    // GarmentAssembler_ExtractComponentsJob (0x303B168E)
    m_addressTable[809178766] = 0xAE6348;

    // GarmentAssembler_ProcessSkinnedMesh (0x6328586F)
    m_addressTable[1663588463] = 0xAFA378;

    // GarmentAssembler_ProcessMorphedMesh (0x5D755CDC)
    m_addressTable[1567972572] = 0xAFA378;

    // GarmentAssembler_OnGameDetach (0x2A471EE7)
    m_addressTable[709304039] = 0x23A74F0;

    // GarmentAssemblerState_AddItem (0x88C21F65)
    m_addressTable[2294423397] = 0x36FE44C;

    // GarmentAssemblerState_AddCustomItem (0xBA7F2EF9)
    m_addressTable[3128897273] = 0x36FE44C;

    // GarmentAssemblerState_ChangeItem (0xDEED2089)
    m_addressTable[3740082313] = 0x36FE44C;

    // GarmentAssemblerState_ChangeCustomItem (0x3905301D)
    m_addressTable[956641309] = 0x36FE44C;

    // ImpostorComponent_OnAttach (0xEE8B1B13)
    m_addressTable[4002093843] = 0x37DCB7C;

    // InkSpawner_FinishAsyncSpawn (0xA0DF3EEB)
    m_addressTable[2698985195] = 0xEECB90;

    // InkWidgetLibrary_AsyncSpawnFromExternal (0x53363DE7)
    m_addressTable[1396063719] = 0xEECB90;

    // InkWidgetLibrary_AsyncSpawnFromLocal (0x0713336F)
    m_addressTable[118698863] = 0xEECB90;

    // InkWidgetLibrary_SpawnFromExternal (0x1E2D3123)
    m_addressTable[506278179] = 0x596DBC;

    // InkWidgetLibrary_SpawnFromLocal (0x450E26AB)
    m_addressTable[1158555307] = 0x228794;

    // InkWorldLayer_UpdateComponents (0x64EC1777)
    m_addressTable[1693194103] = 0x3DA0574;

    // ItemFactoryAppearanceChangeRequest_LoadTemplate (0x4CFA1F9B)
    m_addressTable[1291460507] = 0xCD2B90;

    // ItemFactoryAppearanceChangeRequest_LoadAppearance (0xCA37210E)
    m_addressTable[3392610574] = 0xAD043C;

    // ItemFactoryRequest_LoadAppearance (0xDA241AD8)
    m_addressTable[3659799256] = 0xACA488;

    // IPlacedComponent_SetTransform (0x6D02190A)
    m_addressTable[1828854026] = 0x1443324;

    // JobHandle_Wait (0x5DF10EF9)
    m_addressTable[1576079097] = 0x22575C0;

    // JournalManager_LoadJournal (0x751814E9)
    m_addressTable[1964512489] = 0x378B8;

    // JournalManager_TrackQuest (0xC32B1D01)
    m_addressTable[3274382593] = 0x2D3AD64;

    // JournalRootFolderEntry_Initialize (0x73F24381)
    m_addressTable[1945256833] = 0x1E7528C;

    // JournalTree_ProcessJournalIndex (0x31E616A8)
    m_addressTable[837162664] = 0x1E9FE34;

    // Localization_LoadOnScreens (0xD39A337B)
    m_addressTable[3550098299] = 0x13F68E0;

    // Localization_LoadSubtitles (0x2E0B2E25)
    m_addressTable[772484645] = 0x13F68E0;

    // Localization_LoadVoiceOvers (0xFBC0159B)
    m_addressTable[4223669659] = 0x4293E48;

    // Localization_LoadLipsyncs (0x58BB1C62)
    m_addressTable[1488657506] = 0x1405BC4;

    // MappinSystem_GetMappinData (0xC4AB2879)
    m_addressTable[3299551353] = 0x1D83A68;

    // MappinSystem_GetPoiData (0x25031E71)
    m_addressTable[620961393] = 0x2D3D108;

    // MappinSystem_OnStreamingWorldLoaded (0x085E2668)
    m_addressTable[140387944] = 0x3314AE4;

    // MeshAppearance_LoadMaterialSetupAsync (0x549A2744)
    m_addressTable[1419388740] = 0xB30B3C;

    // MorphTargetMesh_PostLoad (0x5AC91493)
    m_addressTable[1523127443] = 0x1DF1EC;

    // MorphTargetManager_ApplyMorphTarget (0x25C81E78)
    m_addressTable[633871992] = 0x1C5226C;

    // PersistencySystem_SetPersistentStateData (0x208D26A1)
    m_addressTable[546121377] = 0x144EEFC;

    // QuestLoader_ProcessPhaseResource (0x2F1F26CC)
    m_addressTable[790570700] = 0x2ED27D0;

    // QuestsSystem_OnGameRestored (0x7A20106E)
    m_addressTable[2048921710] = 0x2F03328;

    // QuestRootInstance_Start (0x2F8E2179)
    m_addressTable[797843833] = 0x2EFD700;

    // ResourceDepot_InitializeArchives (0xABFC114D)
    m_addressTable[2885423437] = 0x3ED96B0;

    // ResourceDepot_LoadArchives (0x960C410E)
    m_addressTable[2517385486] = 0x3EDA488;

    // ResourceDepot_RequestResource (0x92164ADF)
    m_addressTable[2450934495] = 0x3ED9B94;

    // ResourceDepot_CheckResource (0x02931751)
    m_addressTable[43194193] = 0x3ED9E9C;

    // ResourceLoader_RequestResource (0x8CF73CC3)
    m_addressTable[2365013187] = 0x1569CD0;

    // ResourceLoader_OnUpdate (0x4DAB0F21)
    m_addressTable[1303056161] = 0x2D9766C;

    // ResourcePath_Create (0xEE521259)
    m_addressTable[3998356057] = 0x34094E4;

    // ResourceSerializer_Load (0x99A65476)
    m_addressTable[2577814646] = 0x21AC670;

    // ResourceSerializer_Deserialize (0xACF439FA)
    m_addressTable[2901686778] = 0x21AC670;

    // ResourceSerializer_PostLoad (0x50851ED4)
    m_addressTable[1350901460] = 0x21AC670;

    // ResourceSerializer_OnDependenciesReady (0x46A31827)
    m_addressTable[1185093671] = 0x20AB510;

    // ResourceSerializer_OnResourceReady (0x44601C1A)
    m_addressTable[1147149338] = 0x20AB510;

    // StreamingSector_PostLoad (0xECC9170B)
    m_addressTable[3972601611] = 0x3421748;

    // StreamingWorld_Serialize (0x187B12F3)
    m_addressTable[410718963] = 0x3340510;

    // TPPRepresentationComponent_OnAttach (0xF61E1E7D)
    m_addressTable[4129169021] = 0x35AD5D0;

    // TPPRepresentationComponent_OnItemEquipped (0xEF101D7B)
    m_addressTable[4010810747] = 0x37DF568;

    // TPPRepresentationComponent_OnItemUnequipped (0x733C1BEA)
    m_addressTable[1933319146] = 0x37DF568;

    // TPPRepresentationComponent_RegisterAffectedItem (0xB50A2F8A)
    m_addressTable[3037343626] = 0x37DF568;

    // TPPRepresentationComponent_IsAffectedSlot (0x28771ABA)
    m_addressTable[678894266] = 0x23D9DF4;

    // TweakDB_Load (0xD6BB165A)
    m_addressTable[3602585178] = 0x2B76A50;

    std::cerr << "[ArchiveXLAddressResolver] Initialized with " << m_addressTable.size()
              << " address mappings" << std::endl;
}

uintptr_t ArchiveXLAddressResolver::ResolveAddress(uint32_t aAddressID)
{
    s_requestedAddresses[aAddressID]++;

    auto it = m_addressTable.find(aAddressID);
    if (it != m_addressTable.end())
    {
        const uintptr_t offset = it->second;
        if (offset == 0)
        {
            std::cerr << "[ArchiveXLAddressResolver] WARNING: Address 0x" << std::hex << aAddressID << std::dec
                      << " is a placeholder (offset 0x0)" << std::endl;
            return 0;
        }

        const uintptr_t resolved = GetImageBase() + offset;
        std::cerr << "[ArchiveXLAddressResolver] Resolved 0x" << std::hex << aAddressID << " -> 0x" << resolved
                  << std::dec << std::endl;
        return resolved;
    }

    std::cerr << "[ArchiveXLAddressResolver] ERROR: Unknown address hash 0x" << std::hex << aAddressID << std::dec
              << " (" << aAddressID << ")" << std::endl;
    return 0;
}

} // namespace Support

