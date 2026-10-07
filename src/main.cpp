#include "App/Application.hpp"
#include "App/Project.hpp"
#include "Core/Facades/Hook.hpp"
#include "Core/Facades/Runtime.hpp"

#include <cstdlib>
#include <iostream>

namespace
{
Core::UniquePtr<App::Application> g_app;
bool g_initialized = false;

bool IsEnvFlagEnabled(const char* aName)
{
    const char* value = std::getenv(aName);
    return value && *value && !(value[0] == '0' && value[1] == '\0');
}
}

// RED4ext

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
    {
        if (g_initialized)
            return true;
        g_initialized = true;

        if (IsEnvFlagEnabled("ARCHIVEXL_DISABLE_BOOTSTRAP"))
        {
            std::cerr << "[ArchiveXL] Bootstrap disabled via ARCHIVEXL_DISABLE_BOOTSTRAP=1" << std::endl;
            return true;
        }

        g_app = Core::MakeUnique<App::Application>(aHandle, aSdk);
        g_app->Bootstrap();
        break;
    }
    case RED4ext::v1::EMainReason::Unload:
    {
        if (!g_initialized || !g_app)
            return true;
        g_initialized = false;

        g_app->Shutdown();
        g_app = nullptr;
        break;
    }
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name = App::Project::NameW;
    aInfo->author = App::Project::AuthorW;
    aInfo->version = RED4EXT_V1_SEMVER(App::Project::Version.major,
                                       App::Project::Version.minor,
                                       App::Project::Version.patch);

    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_INDEPENDENT;
    aInfo->sdk = RED4EXT_V1_SDK_VERSION_1_0_0_COMPAT_0_5_0;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1_COMPAT_0;
}

// ASI

#if defined(_WIN32) || defined(_WIN64)
BOOL APIENTRY DllMain(HMODULE aHandle, DWORD aReason, LPVOID)
{
    using GameMain = Core::RawFunc<Red::AddressLib::Main, int32_t (*)(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                                                      PWSTR pCmdLine, int nCmdShow)>;

    static const bool s_isGame = Core::Runtime::IsEXE(L"Cyberpunk2077.exe");
    static const bool s_isASI = Core::Runtime::IsASI(aHandle);

    switch (aReason) // NOLINT(hicpp-multiway-paths-covered)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(aHandle);

        if (s_isGame && s_isASI)
        {
            g_app = Core::MakeUnique<App::Application>(aHandle);

            Core::Hook::Before<GameMain>(+[]() {
                g_app->Bootstrap();
            });
        }
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        if (s_isGame && s_isASI)
        {
            g_app->Shutdown();
            g_app = nullptr;
        }
        break;
    }
    }

    return TRUE;
}
#else
// macOS: No ASI loader support - RED4ext handles plugin lifecycle.
#endif
