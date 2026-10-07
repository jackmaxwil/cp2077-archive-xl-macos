#pragma once

namespace Raw::ResourcePath
{
#ifdef __APPLE__
// macOS: uint64(const char* x0, uint32 w1): the StringView is passed by value and the hash comes back in x0.
constexpr auto Create = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourcePath_Create,
    /* type = */ uint64_t (*)(const char* aPathStr, uint32_t aLength)>();
#else
constexpr auto Create = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourcePath_Create,
    /* type = */ Red::ResourcePath* (*)(Red::ResourcePath* aOut, Red::StringView* aPathStr)>();
#endif
}
