#include "ModuleImage.hpp"
#include <iostream>
#if !defined(_WIN32) && !defined(_WIN64)
#include <dlfcn.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
Core::ModuleImage::ModuleImage(HMODULE aHandle)
{
    std::wstring filePath;
    wil::GetModuleFileNameW(aHandle, filePath);

    m_path = filePath;
}
#else
// Helper function to get the path of this library
static std::string GetThisLibraryPath()
{
    Dl_info info;
    // Use the address of this function to find which library contains it
    if (dladdr(reinterpret_cast<void*>(&GetThisLibraryPath), &info) && info.dli_fname)
    {
        return info.dli_fname;
    }
    return "";
}

Core::ModuleImage::ModuleImage(void* aHandle)
{
    // On macOS, aHandle might be a PluginHandle from RED4ext
    // which could be a dlopen handle or some opaque value.
    std::string filePath = Core::Platform::GetModuleFileName(aHandle);

    if (filePath.empty())
    {
        // Fallback: use dladdr on a function in this library
        filePath = GetThisLibraryPath();
    }

    if (!filePath.empty())
    {
        m_path = filePath;
    }
    else
    {
        std::cerr << "[ModuleImage] Warning: Could not determine module path" << std::endl;
        m_path = "";
    }
}
#endif

std::filesystem::path Core::ModuleImage::GetPath() const
{
    return m_path;
}

std::filesystem::path Core::ModuleImage::GetDir() const
{
    return m_path.parent_path();
}

std::string Core::ModuleImage::GetName() const
{
    return m_path.stem().string();
}

bool Core::ModuleImage::IsASI() const
{
#if defined(_WIN32) || defined(_WIN64)
    return m_path.extension() == L".asi";
#else
    return m_path.extension() == ".asi";
#endif
}
