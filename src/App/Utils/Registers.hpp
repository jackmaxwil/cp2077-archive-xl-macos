#pragma once

#if defined(_WIN32) || defined(_WIN64)
extern "C"
{
    uint64_t get_r8();
    uint64_t get_r9();
    void set_r8(uint64_t);
    void set_r9(uint64_t);
}
#else
// macOS/ARM64: x64 register helpers are not applicable.
// The Journal extension uses these to preserve scratch registers around hook logic on Windows.
// For now, provide no-op stubs so the port compiles.
inline uint64_t get_r8()
{
    return 0;
}
inline uint64_t get_r9()
{
    return 0;
}
inline void set_r8(uint64_t)
{
}
inline void set_r9(uint64_t)
{
}
#endif
