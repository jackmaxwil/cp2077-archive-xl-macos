#include "HostImage.hpp"

#if defined(_WIN32) || defined(_WIN64)
#include "Core/Win.hpp"

Core::HostImage::HostImage(int aExePathDepth)
{
    const auto handle = GetModuleHandleW(nullptr);

    m_base = reinterpret_cast<uintptr_t>(handle);

    std::wstring filePath;
    wil::GetModuleFileNameW(handle, filePath);

    m_exe = filePath;
    m_root = m_exe.parent_path();

    while (--aExePathDepth >= 0)
        m_root = m_root.parent_path();

    TryResolveVersion(filePath);
}

bool Core::HostImage::TryResolveVersion(const std::wstring& filePath)
{
    auto size = GetFileVersionInfoSizeW(filePath.c_str(), nullptr);
    if (!size)
        return false;

    std::unique_ptr<uint8_t[]> data(new (std::nothrow) uint8_t[size]());
    if (!data)
        return false;

    if (!GetFileVersionInfoW(filePath.c_str(), 0, size, data.get()))
        return false;

    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT fileInfoBytes;

    if (!VerQueryValueW(data.get(), L"\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoBytes))
        return false;

    constexpr auto signature = 0xFEEF04BD;
    if (fileInfo->dwSignature != signature)
        return false;

    m_fileVer.major = (fileInfo->dwFileVersionMS >> 16) & 0xFF;
    m_fileVer.minor = fileInfo->dwFileVersionMS & 0xFFFF;
    m_fileVer.build = (fileInfo->dwFileVersionLS >> 16) & 0xFFFF;
    m_fileVer.revision = fileInfo->dwFileVersionLS & 0xFFFF;

    m_productVer.major = (fileInfo->dwProductVersionMS >> 16) & 0xFF;
    m_productVer.minor = fileInfo->dwProductVersionMS & 0xFFFF;
    m_productVer.patch = (fileInfo->dwProductVersionLS >> 16) & 0xFFFF;

    return true;
}

#else // macOS

#include "Core/macOS.hpp"

Core::HostImage::HostImage(int aExePathDepth)
{
    // Get base address from dyld
    m_base = Core::Platform::GetImageBase();

    // Get executable path
    std::string filePath = Core::Platform::GetModuleFileName(nullptr);
    m_exe = filePath;
    m_root = m_exe.parent_path();

    while (--aExePathDepth >= 0)
        m_root = m_root.parent_path();

    TryResolveVersion(filePath);
}

bool Core::HostImage::TryResolveVersion(const std::string& /*filePath*/)
{
    // On macOS, we hardcode the game version since there's no equivalent
    // to Windows GetFileVersionInfo. The game version can be determined
    // from the Info.plist or by other means if needed.

    // Default to a known Cyberpunk 2077 macOS version.
    m_fileVer.major = 2;
    m_fileVer.minor = 21;
    m_fileVer.build = 0;
    m_fileVer.revision = 0;

    m_productVer.major = 2;
    m_productVer.minor = 21;
    m_productVer.patch = 0;

    return true;
}

#endif // Platform

uintptr_t Core::HostImage::GetBase() const
{
    return m_base;
}

std::filesystem::path Core::HostImage::GetPath() const
{
    return m_exe;
}

std::string Core::HostImage::GetName() const
{
    return m_exe.stem().string();
}

std::filesystem::path Core::HostImage::GetRootDir() const
{
    return m_root;
}

const Core::FileVer& Core::HostImage::GetFileVer() const
{
    return m_fileVer;
}

const Core::SemvVer& Core::HostImage::GetProductVer() const
{
    return m_productVer;
}
