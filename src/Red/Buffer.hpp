#pragma once

#include "Red/Common.hpp"

namespace Red
{
struct BufferReader
{
    virtual ~BufferReader() = 0;
    virtual uint32_t GetSize() = 0;
    virtual void sub_10(uint64_t a1) = 0;
    virtual void sub_18(uint64_t a1) = 0;
    virtual uint8_t GetType() = 0;

#ifdef __APPLE__
    // macOS: UniquePtr<BufferReader>(x8 out; const void* payload x0). The result is the single 8-byte owner pointer.
    inline static void Clone(void*& aDst, void* aSrc)
    {
        using MakeResult = Red::SretValue<8>;

        constexpr auto MakeBufferReaderType0 = Core::RawFunc<
            /* addr = */ Red::AddressLib::BufferReader_MakeType0,
            /* type = */ MakeResult (*)(uintptr_t aPayload)>();

        constexpr auto MakeBufferReaderType1 = Core::RawFunc<
            /* addr = */ Red::AddressLib::BufferReader_MakeType1,
            /* type = */ MakeResult (*)(uintptr_t aPayload)>();

        const auto payload = reinterpret_cast<uintptr_t>(aSrc) + 8;

        switch (reinterpret_cast<BufferReader*>(aSrc)->GetType())
        {
        case 0:
        {
            auto result = MakeBufferReaderType0(payload);
            std::memcpy(&aDst, result.data, sizeof(void*));
            break;
        }
        case 1:
        {
            auto result = MakeBufferReaderType1(payload);
            std::memcpy(&aDst, result.data, sizeof(void*));
            break;
        }
        }
    }
#else
    inline static void Clone(void*& aDst, void* aSrc)
    {
        constexpr auto MakeBufferReaderType0 = Core::RawFunc<
            /* addr = */ Red::AddressLib::BufferReader_MakeType0,
            /* type = */ void (*)(void** aOut, uintptr_t a2)>();

        constexpr auto MakeBufferReaderType1 = Core::RawFunc<
            /* addr = */ Red::AddressLib::BufferReader_MakeType1,
            /* type = */ void (*)(void** aOut, uintptr_t a2)>();

        switch (reinterpret_cast<BufferReader*>(aSrc)->GetType())
        {
        case 0:
        {
            MakeBufferReaderType0(&aDst, reinterpret_cast<uintptr_t>(aSrc) + 8);
            break;
        }
        case 1:
        {
            MakeBufferReaderType1(&aDst, reinterpret_cast<uintptr_t>(aSrc) + 8);
            break;
        }
        }
    }
#endif
};
}

namespace Red
{
inline void CopyBuffer(DataBuffer& aDst, const DataBuffer& aSrc)
{
    if (aSrc.buffer.data)
    {
        aDst.buffer.Initialize(aSrc.buffer.GetAllocator(), aSrc.buffer.size, aSrc.buffer.alignment);
        std::memcpy(aDst.buffer.data, aSrc.buffer.data, aSrc.buffer.size);
    }
}

inline void CopyBuffer(DeferredDataBuffer& aDst, const DeferredDataBuffer& aSrc)
{
    if (aSrc.unk30)
    {
        Red::BufferReader::Clone(aDst.unk30, aSrc.unk30);
    }
}
}
