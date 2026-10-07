#pragma once

namespace Red
{
template<typename T>
using ResourceTokenPtr = Red::SharedPtr<Red::ResourceToken<T>>;

template<typename T>
struct Range
{
    constexpr operator bool() const noexcept
    {
        return beginPtr != endPtr;
    }

    [[nodiscard]] inline T* begin() const
    {
        return beginPtr;
    }

    [[nodiscard]] inline T* end() const
    {
        return endPtr;
    }

    [[nodiscard]] auto GetSize() const
    {
        return endPtr - beginPtr;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return !beginPtr;
    }

    T* beginPtr{}; // 00
    T* endPtr{};   // 08
};
RED4EXT_ASSERT_SIZE(Range<int32_t>, 0x10);
RED4EXT_ASSERT_OFFSET(Range<int32_t>, beginPtr, 0x0);
RED4EXT_ASSERT_OFFSET(Range<int32_t>, endPtr, 0x8);

#ifdef __APPLE__
/**
 * macOS: an opaque N-byte value that a game function returns through x8 (arm64 indirect result register).
 *
 * The user-provided destructor makes the type non-trivially copyable, so clang also passes the result address in x8
 * when ArchiveXL declares, calls or hooks such a function. A trivially copyable struct of 16 bytes or less would be
 * returned in x0/x1 instead, which is wrong for these functions. Use it only to forward a result unchanged, or to read
 * a field the evidence documents.
 */
template<size_t N>
struct SretValue
{
    ~SretValue() {} // NOLINT: must stay user-provided, see above

    alignas(8) uint8_t data[N];
};
static_assert(!std::is_trivially_copyable_v<SretValue<8>>);
#endif

/**
 * Fail-closed layout check: true only when the RTTI property aName of T is at aOffset, the offset ArchiveXL was compiled
 * with. Use it before writing a field of a class whose SDK layout is known to differ on macOS, so a stale SDK header
 * disables the feature instead of corrupting memory.
 */
template<typename T>
inline bool HasPropertyAt(CName aName, size_t aOffset)
{
    auto* type = GetClass<T>();
    if (!type)
        return false;

    auto* prop = type->GetProperty(aName);
    return prop && prop->valueOffset == aOffset;
}
}

// True when _type::_field is where the game's RTTI says it is (see Red::HasPropertyAt).
#define RED_FIELD_MATCHES_RTTI(_type, _field) Red::HasPropertyAt<_type>(#_field, __builtin_offsetof(_type, _field))
