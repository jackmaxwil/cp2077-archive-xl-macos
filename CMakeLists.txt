cmake_minimum_required(VERSION 3.20)
project(ArchiveXL VERSION 1.27.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(APPLE)
    message(STATUS "Building ArchiveXL for macOS")
    add_compile_definitions(RED4EXT_PLATFORM_MACOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(OUTPUT_NAME "ArchiveXL")

    # macOS linker flags for dylib
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
else()
    message(STATUS "Building ArchiveXL for Windows")
    add_compile_definitions(WINVER=0x0601)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
    set(OUTPUT_NAME "ArchiveXL")
endif()

# Generate Project.hpp from template
set(NAME "ArchiveXL")
set(AUTHOR "psiberx")
set(VERSION "${PROJECT_VERSION}")
set(VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(MODE "release")
set(DEBUG "")
configure_file(
    "${CMAKE_SOURCE_DIR}/config/Project.hpp.in"
    "${CMAKE_SOURCE_DIR}/src/App/Project.hpp"
    @ONLY
)

# Vendor dependencies (header-only)
add_library(nameof INTERFACE)
target_include_directories(nameof INTERFACE "${CMAKE_SOURCE_DIR}/vendor/nameof/include")

add_library(semver INTERFACE)
target_include_directories(semver INTERFACE "${CMAKE_SOURCE_DIR}/vendor/semver/include")

# RED4ext.SDK (header-only)
add_library(RED4ext.SDK INTERFACE)

# RED4ext.SDK headers. RED4EXT_SDK_DIR is the SDK repository root; it defaults to the pinned vendor/RED4ext.SDK
# submodule. RED4ext's tools/cp-dev and scripts/create_release.sh pass the workspace checkout explicitly.
set(RED4EXT_SDK_DIR "${CMAKE_SOURCE_DIR}/vendor/RED4ext.SDK" CACHE PATH "RED4ext.SDK repository root")
if(NOT EXISTS "${RED4EXT_SDK_DIR}/include/RED4ext/RED4ext.hpp")
    message(FATAL_ERROR "RED4ext.SDK not found at ${RED4EXT_SDK_DIR}. "
                        "Run `git submodule update --init --recursive`, or pass -DRED4EXT_SDK_DIR=<path>.")
endif()
message(STATUS "RED4ext.SDK: ${RED4EXT_SDK_DIR}")
target_include_directories(RED4ext.SDK INTERFACE "${RED4EXT_SDK_DIR}/include")

# Platform-specific dependencies
if(NOT APPLE)
    # WIL (Windows only)
    add_library(wil INTERFACE)
    target_include_directories(wil INTERFACE "${CMAKE_SOURCE_DIR}/vendor/wil/include")
endif()

# Find external packages (Homebrew/system)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)

# Source files
set(ARCHIVEXL_SOURCES
    src/main.cpp
    src/pch.cpp
    src/rtti.cpp
    src/App/Application.cpp
    src/App/Facade.cpp
    src/App/Archives/ArchiveService.cpp
    src/App/Extensions/Animation/Config.cpp
    src/App/Extensions/Animation/Extension.cpp
    src/App/Extensions/Attachment/Extension.cpp
    src/App/Extensions/Customization/Config.cpp
    src/App/Extensions/Customization/Extension.cpp
    src/App/Extensions/ExtensionLoader.cpp
    src/App/Extensions/ExtensionService.cpp
    src/App/Extensions/FactoryIndex/Config.cpp
    src/App/Extensions/FactoryIndex/Extension.cpp
    src/App/Extensions/Garment/Config.cpp
    src/App/Extensions/Garment/Dynamic.cpp
    src/App/Extensions/Garment/Extension.cpp
    src/App/Extensions/Garment/Prefix.cpp
    src/App/Extensions/Garment/States.cpp
    src/App/Extensions/Garment/Tags.cpp
    src/App/Extensions/Garment/Wrapper.cpp
    src/App/Extensions/InkSpawner/Extension.cpp
    src/App/Extensions/Journal/Config.cpp
    src/App/Extensions/Journal/Extension.cpp
    src/App/Extensions/Localization/Config.cpp
    src/App/Extensions/Localization/Extension.cpp
    src/App/Extensions/Localization/Language.cpp
    src/App/Extensions/Mesh/Extension.cpp
    src/App/Extensions/PuppetState/Config.cpp
    src/App/Extensions/PuppetState/Extension.cpp
    src/App/Extensions/PuppetState/Handler.cpp
    src/App/Extensions/PuppetState/System.cpp
    src/App/Extensions/QuestPhase/Config.cpp
    src/App/Extensions/QuestPhase/Extension.cpp
    src/App/Extensions/ResourceLink/Config.cpp
    src/App/Extensions/ResourceLink/Extension.cpp
    src/App/Extensions/ResourceMeta/Config.cpp
    src/App/Extensions/ResourceMeta/Extension.cpp
    src/App/Extensions/ResourcePatch/Config.cpp
    src/App/Extensions/ResourcePatch/Extension.cpp
    src/App/Extensions/Transmog/Extension.cpp
    src/App/Extensions/WorldStreaming/Config.cpp
    src/App/Extensions/WorldStreaming/Extension.cpp
    src/App/Patches/EntitySpawnerPatch.cpp
    src/App/Patches/WorldWidgetLimitPatch.cpp
    src/App/Shared/ResourcePathRegistry.cpp

    lib/Core/Facades/Runtime.cpp
    lib/Core/Foundation/Application.cpp
    lib/Core/Foundation/RuntimeProvider.cpp
    lib/Core/Hooking/HookingAgent.cpp
    lib/Core/Hooking/HookingDriver.cpp
    lib/Core/Logging/LoggingAgent.cpp
    lib/Core/Logging/LoggingDriver.cpp
    lib/Core/Memory/AddressResolver.cpp
    lib/Core/Runtime/HostImage.cpp
    lib/Core/Runtime/ModuleImage.cpp
    lib/Core/Runtime/OwnerMutex.cpp
    lib/Support/RED4ext/RED4extProvider.cpp
    lib/Support/Spdlog/SpdlogProvider.cpp
    lib/Support/RedLib/RedLibProvider.cpp
)

# Platform-specific sources
if(APPLE)
    list(APPEND ARCHIVEXL_SOURCES
        lib/Support/macOS/ArchiveXLAddressResolver.cpp
    )
else()
    list(APPEND ARCHIVEXL_SOURCES
        lib/Support/MinHook/MinHookProvider.cpp
        src/App/Utils/Registers.asm
    )
endif()

# Create shared library
add_library(ArchiveXL SHARED ${ARCHIVEXL_SOURCES})

# Set output name
set_target_properties(ArchiveXL PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})

if(APPLE)
    set_target_properties(ArchiveXL PROPERTIES
        SUFFIX ".dylib"
        PREFIX ""
    )
else()
    set_target_properties(ArchiveXL PROPERTIES
        SUFFIX ".dll"
        PREFIX ""
    )
endif()

# Include directories
target_include_directories(ArchiveXL PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
)

# Link libraries
target_link_libraries(ArchiveXL PRIVATE
    RED4ext.SDK
    nameof
    semver
    spdlog::spdlog
    yaml-cpp::yaml-cpp
)

if(NOT APPLE)
    target_link_libraries(ArchiveXL PRIVATE
        wil
        Version
        User32
    )
endif()

# Precompiled header
target_precompile_headers(ArchiveXL PRIVATE src/pch.hpp)

# Compiler options
if(APPLE)
    target_compile_options(ArchiveXL PRIVATE
        -fno-rtti
        -fexceptions
        -Wno-deprecated-declarations
        -fvisibility=hidden
        -fvisibility-inlines-hidden
    )
else()
    target_compile_options(ArchiveXL PRIVATE
        /MP /GR- /EHsc
    )
endif()

message(STATUS "ArchiveXL ${PROJECT_VERSION} configured")
