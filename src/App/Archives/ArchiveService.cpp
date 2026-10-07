#include "ArchiveService.hpp"
#include "Core/Facades/Runtime.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace
{
bool IsEnvFlagEnabled(const char* aName)
{
    const char* value = std::getenv(aName);
    return value && *value && !(value[0] == '0' && value[1] == '\0');
}
}

App::ArchiveService::ArchiveService(std::filesystem::path aGameDir, std::filesystem::path aBundleDir)
    : m_gameDir(std::move(aGameDir))
    , m_bundleDir(std::move(aBundleDir))
    , m_loaded(false)
{
    if (!m_bundleDir.empty())
    {
        RegisterDirectory(m_bundleDir);
    }

#ifdef __APPLE__
    // The macOS game builds no mod archive group, so archive/pc/mod (where mod managers and mod instructions put
    // mods) is never read. Load it here like the bundle folder; the group it gets is a mod group, so the .xl files
    // next to the archives are found too. ARCHIVEXL_MOD_DIR replaces the folder (used by tests).
    const char* modDirOverride = std::getenv("ARCHIVEXL_MOD_DIR");
    auto modDir = modDirOverride && *modDirOverride ? std::filesystem::path(modDirOverride)
                                                    : m_gameDir / "archive" / "pc" / "mod";
    std::error_code error;
    if (std::filesystem::is_directory(modDir, error))
    {
        RegisterDirectory(modDir);
    }
#endif
}

void App::ArchiveService::OnBootstrap()
{
    if (IsEnvFlagEnabled("ARCHIVEXL_DISABLE_ARCHIVE_SERVICE_HOOK"))
    {
        LogWarning("[ArchiveService] HookAfter(ResourceDepot::InitializeArchives) disabled via ARCHIVEXL_DISABLE_ARCHIVE_SERVICE_HOOK=1");
        return;
    }

    HookAfter<Raw::ResourceDepot::InitializeArchives>(&ArchiveService::OnInitializeArchives).OrThrow();
}

void App::ArchiveService::OnShutdown()
{
    Unhook<Raw::ResourceDepot::InitializeArchives>();
}

namespace
{
void LoadArchives(Red::ArchiveGroup& aGroup, const Red::DynArray<Red::CString>& aArchivePaths,
                  Red::DynArray<Red::ResourcePath>& aLoadedResources)
{
#ifdef __APPLE__
    Raw::ResourceDepot::LoadArchives(nullptr, aGroup, aArchivePaths, aLoadedResources, false,
                                     Raw::ResourceDepot::LoadArchivesExitCode);
#else
    Raw::ResourceDepot::LoadArchives(nullptr, aGroup, aArchivePaths, aLoadedResources, false);
#endif
}
}

void App::ArchiveService::OnInitializeArchives(Red::ResourceDepot* aDepot)
{
    Red::RegisterPendingTypes();

    m_loaded = true;

    LogInfo("Loading extra archives...");

    if (m_dirs.empty() && m_archives.empty())
        return;

    Core::Vector<std::filesystem::path> loadedArchives;
    Red::DynArray<Red::ResourcePath> loadedResources;

    for (const auto& archiveDir : m_dirs)
    {
        std::error_code error;
        auto dirIt = std::filesystem::directory_iterator(archiveDir, error);

        if (error)
        {
            LogError("Can't load archive directory \"{}\": {}",
                     std::filesystem::relative(archiveDir, m_gameDir).string(),
                     error.message());
            continue;
        }

        // Sorted by name: mods rely on the alphabetical load order (prefixes such as "!" and "#").
        Core::Vector<std::filesystem::path> dirArchives;

        for (const auto& entry : dirIt)
        {
            if (entry.is_regular_file() && entry.path().extension() == L".archive")
            {
                dirArchives.push_back(entry.path());
            }
        }

        // Windows lists a folder in case-insensitive order (names compared upper-cased), and when two archives contain
        // the same file the first one wins; the macOS depot keeps the first one too (tested), so sort the same way.
        std::sort(dirArchives.begin(), dirArchives.end(), [](const auto& aLeft, const auto& aRight) {
            auto left = aLeft.filename().string();
            auto right = aRight.filename().string();
            auto upper = [](unsigned char aChar) { return static_cast<char>(std::toupper(aChar)); };
            std::transform(left.begin(), left.end(), left.begin(), upper);
            std::transform(right.begin(), right.end(), right.begin(), upper);
            return left != right ? left < right : aLeft < aRight;
        });

        Red::DynArray<Red::CString> archivePaths;

        for (const auto& archivePath : dirArchives)
        {
            archivePaths.PushBack(archivePath.string());

            if (archiveDir != m_bundleDir)
            {
                loadedArchives.push_back(archivePath);
            }
        }

        auto& group = ResolveArchiveGroup(aDepot, archiveDir.string());

        if (!archivePaths.IsEmpty())
        {
            LoadArchives(group, archivePaths, loadedResources);
        }
    }

    if (!m_archives.empty())
    {
        Red::DynArray<Red::CString> archivePaths;

        for (const auto& archivePath : m_archives)
        {
            archivePaths.PushBack(archivePath.string());
            loadedArchives.push_back(archivePath);
        }

        auto& group = ResolveArchiveGroup(aDepot, "");
        LoadArchives(group, archivePaths, loadedResources);
    }

    for (const auto& archivePath : loadedArchives)
    {
        LogInfo("Archive \"{}\" loaded.", std::filesystem::relative(archivePath, m_gameDir).string());
    }
}

Red::ArchiveGroup& App::ArchiveService::ResolveArchiveGroup(Red::ResourceDepot* aDepot, const Red::CString& aBasePath)
{
    auto existingGroup = std::find_if(aDepot->groups.begin(), aDepot->groups.end(),
                                     [&aBasePath](const Red::ArchiveGroup& aGroup) {
                                         return aGroup.basePath == aBasePath;
                                     });

    if (existingGroup != aDepot->groups.end())
    {
        return *existingGroup;
    }

    auto firstNonModGroup = std::find_if(aDepot->groups.begin(), aDepot->groups.end(),
                                         [](const Red::ArchiveGroup& aGroup) {
                                             return aGroup.scope != Red::ArchiveScope::Mod;
                                         });
    auto firstNonModGroupIndex = firstNonModGroup - aDepot->groups.begin();

    aDepot->groups.Emplace(firstNonModGroup);

    auto& group = aDepot->groups[firstNonModGroupIndex];
    group.basePath = aBasePath;
    group.scope = Red::ArchiveScope::Mod;

    return group;
}

bool App::ArchiveService::RegisterArchive(std::filesystem::path aPath)
{
    std::error_code error;

    if (aPath.is_relative())
    {
        aPath = m_gameDir / aPath;
    }

    if (!std::filesystem::exists(aPath, error) || !std::filesystem::is_regular_file(aPath, error))
    {
        LogError("Can't register archive \"{}\": path doesn't exist.",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    if (m_loaded)
    {
        LogError("Can't register archive \"{}\": depot is already initialized.",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    m_archives.emplace_back(std::move(aPath));
    return true;
}

bool App::ArchiveService::RegisterDirectory(std::filesystem::path aPath)
{
    std::error_code error;

    if (aPath.is_relative())
    {
        aPath = m_gameDir / aPath;
    }

    if (!std::filesystem::exists(aPath, error) || !std::filesystem::is_directory(aPath, error))
    {
        LogError("Can't register archive directory \"{}\": path doesn't exist.",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    if (m_loaded)
    {
        LogError("Can't register archive directory \"{}\": depot is already initialized.",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    m_dirs.emplace_back(std::move(aPath));
    return true;
}
