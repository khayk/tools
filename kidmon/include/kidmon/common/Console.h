#pragma once

#include <kidmon/common/Runnable.h>
#include <memory>

class Console
{
public:
    explicit Console(const std::shared_ptr<Runnable>& runnable);
    ~Console();

    void run();
    void shutdown();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
