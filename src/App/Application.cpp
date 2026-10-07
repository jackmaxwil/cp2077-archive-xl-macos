#include "Application.hpp"
#include "App/Archives/ArchiveService.hpp"
#include "App/Environment.hpp"
#include "App/Extensions/ExtensionService.hpp"
#include "App/Migration.hpp"
#include "App/Patches/EntitySpawnerPatch.hpp"
#include "App/Patches/WorldWidgetLimitPatch.hpp"
#include "App/Project.hpp"
#include "App/Shared/ResourcePathRegistry.hpp"
#include "Red/Common.hpp"
#include "Core/Foundation/RuntimeProvider.hpp"
#include "Support/RED4ext/RED4extProvider.hpp"
#include "Support/RedLib/RedLibProvider.hpp"
#include "Support/Spdlog/SpdlogProvider.hpp"

#if defined(_WIN32) || defined(_WIN64)
#include "Support/MinHook/MinHookProvider.hpp"
#else
#include "Support/macOS/ArchiveXLAddressResolver.hpp"
#include "Support/macOS/MacOSHookingProvider.hpp"
#endif

#ifdef __APPLE__
namespace
{
// Itanium C++ ABI: a class is returned in registers only when it is "trivial for the purposes of calls" (trivial copy
// and move constructors and a trivial destructor) and at most 16 bytes; otherwise the result goes through x8.
template<typename T>
constexpr bool ReturnedInRegisters = std::is_trivially_copy_constructible_v<T> && std::is_trivially_destructible_v<T> &&
                                     sizeof(T) <= 16;
}

// Game functions that return these types write through x8, so ArchiveXL's declarations must use x8 too.
static_assert(!ReturnedInRegisters<Red::JobHandle>);
static_assert(!ReturnedInRegisters<Red::JobHandleResult>);
static_assert(!ReturnedInRegisters<Red::Handle<Red::ISerializable>>);
static_assert(!ReturnedInRegisters<Red::SharedPtr<Red::ResourceToken<>>>);
static_assert(!ReturnedInRegisters<Red::CString>);
static_assert(!ReturnedInRegisters<Red::DynArray<Red::Handle<Red::ISerializable>>>);
static_assert(!ReturnedInRegisters<Red::SretValue<8>>);
static_assert(!ReturnedInRegisters<Red::SretValue<0x10>>);
// ResourcePath and CName results come back in x0 as plain 8-byte values.
static_assert(ReturnedInRegisters<Red::ResourcePath> && sizeof(Red::ResourcePath) == 8);
static_assert(ReturnedInRegisters<Red::CName> && sizeof(Red::CName) == 8);
#endif

#if defined(_WIN32) || defined(_WIN64)
App::Application::Application(HMODULE aHandle, const RED4ext::Sdk* aSdk)
#else
App::Application::Application(void* aHandle, const RED4ext::Sdk* aSdk)
#endif
{
    // Game root from the executable: bin/x64/Cyberpunk2077.exe on Windows,
    // Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077 on macOS.
    Register<Core::RuntimeProvider>(aHandle)
#if defined(_WIN32) || defined(_WIN64)
        ->SetBaseImagePathDepth(2);
#else
        ->SetBaseImagePathDepth(3);
#endif

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

    // Registers ArchiveXL's native script classes (ArchiveXL, PuppetStateSystem, ...). Without them the game's script
    // loader cannot bind the declarations in ArchiveXL's scripts.
    Register<Support::RedLibProvider>();
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
