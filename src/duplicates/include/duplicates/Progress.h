#pragma once

#include <core/utils/StopWatch.h>

#include <iostream>
#include <functional>

namespace tools::dups {

class Progress
{
public:
    using DisplayCb = std::function<void(std::ostream&)>;

    explicit Progress(std::chrono::milliseconds freq = std::chrono::milliseconds(100))
        : sw_(true)
        , freq_ {freq}
    {
    }

    void update(const DisplayCb& cb)
    {
        if (sw_.elapsed() > freq_)
        {
            cb(std::cout);
            std::cout << '\r';
            std::cout.flush();
            sw_.restart();
        }
    }

private:
    StopWatch sw_;
    std::chrono::milliseconds freq_ {};
};

} // namespace tools::dups
