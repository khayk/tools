#pragma once

#include "common/Runnable.h"

#include <memory>

class KidMon : public Runnable
{
public:
    KidMon();
    ~KidMon();

    void run() override;
    void shutdown() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
