#pragma once

#include "Core/Foundation/Feature.hpp"
#include "Core/Memory/AddressResolver.hpp"
#include <cstdint>
#include <unordered_map>

namespace Support
{
/**
 * Custom address resolver for ArchiveXL on macOS.
 *
 * ArchiveXL uses its own ~130 address hashes (see src/Red/Addresses/Library.hpp).
 * On macOS, we resolve these hashes to ARM64 offsets relative to the main image base:
 *   resolved = image_base + offset
 */
class ArchiveXLAddressResolver : public Core::Feature, public Core::AddressResolver
{
public:
    ArchiveXLAddressResolver();

    uintptr_t ResolveAddress(uint32_t aAddressID) override;

    /**
     * Get the main executable image base (cached).
     */
    static uintptr_t GetImageBase();

protected:
    /**
     * Called when the feature is initialized - sets this as the default resolver.
     */
    void OnInitialize() override;

private:
    void InitializeAddressTable();

    std::unordered_map<uint32_t, uintptr_t> m_addressTable;

    static std::unordered_map<uint32_t, int> s_requestedAddresses;
};
}

