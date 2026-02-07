#pragma once

// macOS platform definitions and helpers
// Replaces WIL and Windows-specific headers for macOS builds
// NOTE: RED4ext.SDK already provides WinCompat.hpp, so we only provide
// platform-specific implementations, not type aliases.

#include <cstdint>
#include <string>
#include <filesystem>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <unistd.h>
#include <libproc.h>
#include <sys/types.h>
#include <climits>

namespace Core::Platform
{

// Type alias for module handle (replaces HMODULE)
using ModuleHandle = void*;

// Get the path of a loaded module.
// Note: On macOS, aHandle should be either:
// 1. nullptr - returns main executable path
// 2. An address within the module (e.g., &SomeFunction) - NOT a dlopen handle!
//    Since dlopen handles are opaque on macOS, we can't use them directly with dladdr.
inline std::string GetModuleFileName(ModuleHandle aHandle)
{
    if (!aHandle)
    {
        // Get main executable path
        char path[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(getpid(), path, sizeof(path)) > 0)
        {
            return path;
        }

        // Fallback: use _NSGetExecutablePath
        char exePath[PATH_MAX];
        uint32_t size = sizeof(exePath);
        if (_NSGetExecutablePath(exePath, &size) == 0)
        {
            char realPath[PATH_MAX];
            if (realpath(exePath, realPath))
            {
                return realPath;
            }
            return exePath;
        }
        return "";
    }

    // Try to get path using dladdr with the handle as an address
    Dl_info info;
    if (dladdr(aHandle, &info) && info.dli_fname)
    {
        return info.dli_fname;
    }

    // Fallback: The handle might be a dlopen handle or some other value
    // that's not a valid address. Return empty string.
    return "";
}

// Get module handle for a loaded library (nullptr = main executable)
inline ModuleHandle GetModuleHandle(const char* aName = nullptr)
{
    if (!aName)
    {
        // Return handle to main executable
        // On macOS, passing nullptr to dlopen returns handle to main executable
        return dlopen(nullptr, RTLD_NOLOAD);
    }

    // Try to get handle to already-loaded library
    return dlopen(aName, RTLD_NOLOAD);
}

// Get base address of main executable
inline uintptr_t GetImageBase()
{
    return reinterpret_cast<uintptr_t>(_dyld_get_image_header(0));
}

// Helper to convert wide string to narrow string (for compatibility)
inline std::string WideToNarrow(const std::wstring& aWide)
{
    if (aWide.empty())
        return "";

    std::string narrow;
    narrow.reserve(aWide.size());
    for (wchar_t wc : aWide)
    {
        if (wc < 128)
            narrow.push_back(static_cast<char>(wc));
        else
            narrow.push_back('?'); // Replace non-ASCII
    }
    return narrow;
}

// Helper to convert narrow string to wide string (for compatibility)
inline std::wstring NarrowToWide(const std::string& aNarrow)
{
    return std::wstring(aNarrow.begin(), aNarrow.end());
}

} // namespace Core::Platform

// NOTE: Windows type aliases (HMODULE, DWORD, etc.) and compatibility functions
// (GetModuleHandleW, etc.) are already provided by RED4ext.SDK's WinCompat.hpp.

