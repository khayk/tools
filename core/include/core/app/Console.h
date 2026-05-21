#pragma once

#include <core/app/Runnable.h>
#include <memory>

namespace core {

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

} // namespace core
