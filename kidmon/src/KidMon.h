#pragma once

#include "common/Runnable.h"

class KidMon : public Runnable
{
public:
    void run() override;
    void shutdown() noexcept override;

private:
    bool stopRequested_{ false };
};
