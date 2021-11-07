#pragma once

#include "Runnable.h"
#include <memory>
#include <string>

class Service
{
public:
    Service(const std::shared_ptr<Runnable>& runnable, std::string name);
    ~Service();

    void run();
    void shutdown() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
