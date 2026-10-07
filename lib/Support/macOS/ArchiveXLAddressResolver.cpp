#include "ArchiveXLAddressResolver.hpp"
#include <iostream>
#include <cstdlib>
#include <RED4ext/Relocation.hpp>

namespace Support
{

std::unordered_map<uint32_t, int> ArchiveXLAddressResolver::s_requestedAddresses;

namespace
{
bool IsAddressTraceEnabled()
{
    static const bool enabled = []() {
        const char* value = std::getenv("ARCHIVEXL_ADDR_TRACE");
        return value && value[0] != '\0' && value[0] != '0';
    }();
    return enabled;
}
}

ArchiveXLAddressResolver::ArchiveXLAddressResolver()
{
    InitializeAddressTable();
}

void ArchiveXLAddressResolver::OnInitialize()
{
    AddressResolver::SetDefault(*this);
    if (IsAddressTraceEnabled())
    {
        std::cerr << "[ArchiveXLAddressResolver] Registered as default address resolver" << std::endl;
    }
}

uintptr_t ArchiveXLAddressResolver::GetImageBase()
{
    static const uintptr_t base = RED4ext::RelocBase::GetImageBase();
    return base;
}

void ArchiveXLAddressResolver::InitializeAddressTable()
{
    // Former hard-coded offsets now live, unverified, in the canonical address DB.
}

uintptr_t ArchiveXLAddressResolver::ResolveAddress(uint32_t aAddressID)
{
    // All addresses come from the canonical DB (red4ext/bin/x64/cyberpunk2077_addresses.json) through the SDK
    // resolver, which refuses entries not marked "verified" (fail closed): an unverified hook target resolves to 0
    // and the hook is not installed.
    s_requestedAddresses[aAddressID]++;
    return RED4ext::UniversalRelocBase::Resolve(aAddressID);
}

} // namespace Support

