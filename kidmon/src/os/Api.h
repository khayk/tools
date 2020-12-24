#pragma once

#include "Window.h"

#include <memory>

class Api
{
public:
    virtual ~Api() = default;

    virtual WindowPtr forgroundWindow() = 0;
};

using ApiPtr = std::unique_ptr<Api>;

class ApiFactory
{
public:
    static ApiPtr create();
};

