#pragma once

#include "Core/Foundation/Feature.hpp"
#include "Core/Hooking/HookingDriver.hpp"
#include <RED4ext/Api/Sdk.hpp>
#include <iostream>

namespace Support
{
class MacOSHookingProvider
    : public Core::Feature
    , public Core::HookingDriver
{
public:
    MacOSHookingProvider(RED4ext::PluginHandle aPlugin, const RED4ext::Sdk* aSdk) noexcept
        : m_plugin(aPlugin)
        , m_sdk(aSdk)
    {
    }

protected:
    void OnInitialize() override
    {
        std::cerr << "[MacOSHookingProvider] Setting as default hooking driver" << std::endl;
        SetDefault(*this);
    }

    bool HookAttach(uintptr_t aAddress, void* aCallback) override
    {
        std::cerr << "[MacOSHookingProvider] HookAttach (no original): " << std::hex << aAddress << " -> " << aCallback
                  << std::dec << std::endl;

        if (!m_sdk || !m_sdk->hooking)
        {
            std::cerr << "[MacOSHookingProvider] SDK or hooking interface not available" << std::endl;
            return false;
        }

        return m_sdk->hooking->Attach(m_plugin, reinterpret_cast<void*>(aAddress), aCallback, nullptr);
    }

    bool HookAttach(uintptr_t aAddress, void* aCallback, void** aOriginal) override
    {
        std::cerr << "[MacOSHookingProvider] HookAttach (with original): " << std::hex << aAddress << " -> " << aCallback
                  << std::dec << std::endl;

        if (!m_sdk || !m_sdk->hooking)
        {
            std::cerr << "[MacOSHookingProvider] SDK or hooking interface not available" << std::endl;
            return false;
        }

        return m_sdk->hooking->Attach(m_plugin, reinterpret_cast<void*>(aAddress), aCallback, aOriginal);
    }

    bool HookDetach(uintptr_t aAddress) override
    {
        std::cerr << "[MacOSHookingProvider] HookDetach: " << std::hex << aAddress << std::dec << std::endl;

        if (!m_sdk || !m_sdk->hooking)
        {
            std::cerr << "[MacOSHookingProvider] SDK or hooking interface not available" << std::endl;
            return false;
        }

        return m_sdk->hooking->Detach(m_plugin, reinterpret_cast<void*>(aAddress));
    }

private:
    RED4ext::PluginHandle m_plugin;
    const RED4ext::Sdk* m_sdk;
};
}

