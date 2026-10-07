#include "ResourcePathRegistry.hpp"
#include "App/Project.hpp"
#include "Red/SharedStorage.hpp"

#include <cstdlib>

namespace
{
constexpr auto SharedName = Red::CName("ResourcePathRegistryV3" BUILD_SUFFIX);

bool IsEnvFlagEnabled(const char* aName)
{
    const char* value = std::getenv(aName);
    return value && *value && !(value[0] == '0' && value[1] == '\0');
}
}

App::ResourcePathRegistry::ResourcePathRegistry(const std::filesystem::path& aPreloadPath)
{
#if defined(__APPLE__)
    // macOS: CRTTISystem/RTTI shared storage isn't reliably safe during plugin load.
    // Keep the registry instance local to avoid early RTTI access (prevents startup crashes).
    static SharedInstance s_localInstance;
    s_instance = &s_localInstance;
#else
    s_instance = Red::AcquireSharedInstance<SharedName, SharedInstance>();
#endif
    s_preloadPath = aPreloadPath;
}

void App::ResourcePathRegistry::OnBootstrap()
{
    std::unique_lock lock(s_instance->m_lock);

    if (!s_instance->m_initialized)
    {
        s_instance->m_initialized = true;
        s_instance->m_map.reserve(400000);

        if (IsEnvFlagEnabled("ARCHIVEXL_DISABLE_RESOURCE_PATH_REGISTRY_HOOK"))
        {
            LogWarning("[ResourcePathRegistry] HookAfter(ResourcePath::Create) disabled via ARCHIVEXL_DISABLE_RESOURCE_PATH_REGISTRY_HOOK=1");
        }
        else
        {
            HookAfter<Raw::ResourcePath::Create>(&OnCreatePath);
        }
    }

    if (!s_instance->m_preloaded && !s_preloadPath.empty() && std::filesystem::exists(s_preloadPath))
    {
        s_instance->m_preloaded = true;

        std::thread([lock = std::move(lock)]() {
            LogInfo("[ResourcePathRegistry] Loading metadata...");

            std::ifstream f(s_preloadPath);
            std::string s;
            while (std::getline(f, s))
            {
                s_instance->m_map[Red::ResourcePath::HashSanitized(s.data())] = std::move(s);
            }

            LogInfo("[ResourcePathRegistry] Loaded {} predefined hashes.", s_instance->m_map.size());
        }).detach();
    }
}

#ifdef __APPLE__
void App::ResourcePathRegistry::OnCreatePath(uint64_t& aHash, const char* aPathStr, uint32_t aLength)
{
    if (aPathStr && aLength)
    {
        std::scoped_lock _(s_instance->m_lock);
        s_instance->m_map[Red::ResourcePath(aHash)] = {aPathStr, aLength};
    }
}
#else
void App::ResourcePathRegistry::OnCreatePath(Red::ResourcePath* aPath, Red::StringView* aPathStr)
{
    if (aPathStr && *aPathStr)
    {
        std::scoped_lock _(s_instance->m_lock);
        s_instance->m_map[*aPath] = {aPathStr->Data(), aPathStr->Length()};
    }
}
#endif

std::string App::ResourcePathRegistry::ResolvePath(Red::ResourcePath aPath)
{
    if (!aPath)
        return {};

    std::shared_lock _(s_instance->m_lock);
    const auto& it = s_instance->m_map.find(aPath);

    if (it == s_instance->m_map.end())
        return {};

    return it->second;
}

std::string App::ResourcePathRegistry::ResolvePathOrHash(Red::ResourcePath aPath)
{
    auto str = ResolvePath(aPath);

    if (str.empty())
    {
        str = std::to_string(aPath.hash);
    }

    return str;
}

Red::ResourcePath App::ResourcePathRegistry::RegisterPath(const std::string& aPathStr)
{
    if (aPathStr.empty())
        return {};

    auto path = Red::ResourcePath(aPathStr.data());

    RegisterPath(path, aPathStr);

    return path;
}

void App::ResourcePathRegistry::RegisterPath(Red::ResourcePath aPath, const std::string& aPathStr)
{
    if (!aPath)
        return;

    {
        std::shared_lock _(s_instance->m_lock);
        if (s_instance->m_map.contains(aPath))
            return;
    }

    {
        std::scoped_lock _(s_instance->m_lock);
        s_instance->m_map[aPath] = aPathStr;
    }
}
