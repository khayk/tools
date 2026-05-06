#pragma once

class Runnable
{
public:
    virtual ~Runnable() = default;

    virtual void run() = 0;
    virtual void shutdown() noexcept = 0;
};
