#pragma once

#include "Core/Foundation/Application.hpp"
#include "Core/Logging/LoggingAgent.hpp"

namespace App
{
class Application
    : public Core::Application
    , public Core::LoggingAgent
{
public:
#if defined(_WIN32) || defined(_WIN64)
    explicit Application(HMODULE aHandle, const RED4ext::Sdk* aSdk = nullptr);
#else
    explicit Application(void* aHandle, const RED4ext::Sdk* aSdk = nullptr);
#endif

protected:
    void OnStarting() override;
};
}
