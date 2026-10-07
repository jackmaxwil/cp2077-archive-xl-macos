#pragma once

namespace Red
{
struct ResourceRequest
{
    ResourcePath path; // 00
    uint64_t unk08{};  // 08
    uint64_t unk10{};  // 10
    int32_t unk18{-1}; // 18
    int32_t unk1C{};   // 1C
    uint64_t unk20{};  // 20
};
RED4EXT_ASSERT_SIZE(ResourceRequest, 0x28);
RED4EXT_ASSERT_OFFSET(ResourceRequest, path, 0x0);
}

namespace Raw::ResourceLoader
{
#ifdef __APPLE__
// macOS: SharedPtr<Token>(x8 out; loader x0, const ResourceRequest* x1), see RED4ext.SDK docs/re/resources.md.
constexpr auto LoadAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceLoader_RequestResource,
    /* type = */ Red::SharedPtr<Red::ResourceToken<>> (*)(Red::ResourceLoader* aLoader,
                                                          Red::ResourceRequest& aRequest)>();

// macOS: no per-frame ResourceLoader function is verified, so the hot-reload trigger is disabled (ExtensionService).
#else
constexpr auto LoadAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceLoader_RequestResource,
    /* type = */ void (*)(Red::ResourceLoader* aLoader,
                          Red::SharedPtr<Red::ResourceToken<>>& aToken,
                          Red::ResourceRequest& aRequest)>();

constexpr auto OnUpdate = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceLoader_OnUpdate,
    /* type = */ void (*)(Red::ResourceLoader* aLoader)>();
#endif
}
