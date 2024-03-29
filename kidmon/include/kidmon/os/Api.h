#pragma once

#include <kidmon/os/Window.h>
#include <kidmon/os/ProcessLauncher.h>

#include <memory>

class Api
{
public:
    virtual ~Api() = default;

    virtual WindowPtr foregroundWindow() = 0;
    virtual ProcessLauncherPtr createProcessLauncher() = 0;
};

using ApiPtr = std::unique_ptr<Api>;

class ApiFactory
{
public:
    static ApiPtr create();
};
