#include "Application.hpp"
#include "App/Archives/ArchiveService.hpp"
#include "App/Environment.hpp"
#include "App/Extensions/ExtensionService.hpp"
#include "App/Migration.hpp"
#include "App/Patches/EntitySpawnerPatch.hpp"
#include "App/Patches/WorldWidgetLimitPatch.hpp"
#include "App/Project.hpp"
#include "App/Shared/ResourcePathRegistry.hpp"
#include "Core/Foundation/RuntimeProvider.hpp"
#include "Support/RED4ext/RED4extProvider.hpp"
#include "Support/Spdlog/SpdlogProvider.hpp"

#if defined(_WIN32) || defined(_WIN64)
#include "Support/MinHook/MinHookProvider.hpp"
#include "Support/RedLib/RedLibProvider.hpp"
#else
#include "Support/macOS/ArchiveXLAddressResolver.hpp"
#include "Support/macOS/MacOSHookingProvider.hpp"
#endif

#if defined(_WIN32) || defined(_WIN64)
App::Application::Application(HMODULE aHandle, const RED4ext::Sdk* aSdk)
#else
App::Application::Application(void* aHandle, const RED4ext::Sdk* aSdk)
#endif
{
    Register<Core::RuntimeProvider>(aHandle)
        ->SetBaseImagePathDepth(2);

    Register<Support::SpdlogProvider>()
        ->AppendTimestampToLogName()
        ->CreateRecentLogSymlink();

#if defined(_WIN32) || defined(_WIN64)
    Register<Support::MinHookProvider>();

    Register<Support::RED4extProvider>(aHandle, aSdk)
        ->EnableAddressLibrary()
        ->RegisterScripts(Env::ScriptsDir());

    Register<Support::RedLibProvider>();
#else
    // macOS: Custom address resolver + hook provider that forwards to RED4ext's SDK
    Register<Support::ArchiveXLAddressResolver>();
    Register<Support::MacOSHookingProvider>(aHandle, aSdk);

    Register<Support::RED4extProvider>(aHandle, aSdk)
        ->RegisterScripts(Env::ScriptsDir());
#endif

    Register<App::ResourcePathRegistry>();
    Register<App::ArchiveService>(Env::GameDir(), Env::BundleDir());
    Register<App::ExtensionService>(Env::BundleDir());
    Register<App::EntitySpawnerPatch>();
    Register<App::WorldWidgetLimitPatch>();
}

void App::Application::OnStarting()
{
    LogInfo("{} {} is starting...", Project::Name, Project::Version.to_string());

    Migration::CleanUp(Env::LegacyBundleDir());
    Migration::CleanUp(Env::LegacyScriptsDir());
}
