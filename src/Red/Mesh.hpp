#pragma once

namespace Red
{
struct MeshMaterialsData
{
    using AllocatorType = Memory::MeshAllocator;

    DynArray<Handle<IMaterial>> materials;
};

struct MeshMaterialsToken
{
    JobHandle job;
    SharedPtr<MeshMaterialsData> data;
};
}

namespace Raw::CMesh
{
using MaterialLock = Core::OffsetPtr<0x218, Red::SharedSpinLock>;

constexpr auto PostLoad = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_PostLoad,
    /* type = */ void (*)(Red::CMesh* aResource, Red::PostLoadParams* a2)>();

constexpr auto GetAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_GetAppearance,
    /* type = */ Red::Handle<Red::mesh::MeshAppearance>& (*)(Red::CMesh* aMesh, Red::CName aAppearance)>();

constexpr auto FindAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_FindAppearance,
    /* type = */ Red::Handle<Red::mesh::MeshAppearance>& (*)(Red::CMesh* aMesh, Red::CName aAppearance)>();

#ifdef __APPLE__
// macOS: MeshMaterialsToken(x8 out; mesh x0, const DynArray<CName>& x1, u8 w2).
constexpr auto LoadMaterialsAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_LoadMaterialsAsync,
    /* type = */ Red::MeshMaterialsToken (*)(Red::CMesh* aMesh,
                                             const Red::DynArray<Red::CName>& aMaterialNames,
                                             uint8_t a4)>();
#else
constexpr auto LoadMaterialsAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_LoadMaterialsAsync,
    /* type = */ void* (*)(Red::CMesh* aMesh,
                           Red::MeshMaterialsToken& aOut,
                           const Red::DynArray<Red::CName>& aMaterialNames,
                           uint8_t a4)>();
#endif

constexpr auto AddStubAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_AddStubAppearance,
    /* type = */ void (*)(Red::CMesh* aMesh)>();

constexpr auto ShouldPreloadAppearances = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_ShouldPreloadAppearances,
    /* type = */ bool (*)(Red::CMesh* aMesh)>();
}

namespace Raw::MeshMaterialBuffer
{
#ifdef __APPLE__
// macOS: SharedPtr<ResourceToken<IMaterial>>(x8 out; buffer x0, const Handle<CMesh>& x1, u16 w2, u64 x3, u8 w4).
constexpr auto LoadMaterialAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::MeshMaterialBuffer_LoadMaterialAsync,
    /* type = */ Red::SharedPtr<Red::ResourceToken<Red::IMaterial>> (*)(Red::meshMeshMaterialBuffer* aBuffer,
                                                                        const Red::Handle<Red::CMesh>& aMesh,
                                                                        uint16_t aMaterialIndex,
                                                                        uint64_t a5,
                                                                        uint8_t a6)>();
#else
constexpr auto LoadMaterialAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::MeshMaterialBuffer_LoadMaterialAsync,
    /* type = */ void* (*)(Red::meshMeshMaterialBuffer* aBuffer,
                           Red::SharedPtr<Red::ResourceToken<Red::IMaterial>>& aOut,
                           const Red::Handle<Red::CMesh>& aMesh,
                           uint16_t aMaterialIndex,
                           uint64_t a5,
                           uint8_t a6)>();
#endif
}

namespace Raw::MeshAppearance
{
using Owner = Core::OffsetPtr<0x50, Red::CMesh*>;

#ifdef __APPLE__
// macOS: Handle<MeshAppearance>(x8 out; appearance x0, u8 w1).
constexpr auto LoadMaterialSetupAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::MeshAppearance_LoadMaterialSetupAsync,
    /* type = */ Red::Handle<Red::mesh::MeshAppearance> (*)(Red::mesh::MeshAppearance& aAppearance, uint8_t a3)>();
#else
constexpr auto LoadMaterialSetupAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::MeshAppearance_LoadMaterialSetupAsync,
    /* type = */ void (*)(Red::mesh::MeshAppearance& aAppearance, Red::Handle<Red::mesh::MeshAppearance>& aOut,
                          uint8_t a3)>();
#endif
}

namespace Raw::MeshComponent
{
constexpr auto LoadResource = Core::RawVFunc<
        /* offset = */ 0x260,
        /* type = */ uint64_t(Red::IComponent::*)(Red::JobQueue& aQueue)>();

constexpr auto RefreshAppearance = Core::RawVFunc<
        /* offset = */ 0x280,
        /* type = */ void(Red::IComponent::*)()>();
}
