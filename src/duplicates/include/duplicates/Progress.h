#pragma once

#include <core/utils/StopWatch.h>

#include <iostream>
#include <functional>

namespace tools::dups {

class Progress
{
public:
    using DisplayCb = std::function<void(std::ostream&)>;

    explicit Progress(std::chrono::milliseconds freq = std::chrono::milliseconds(100),
                      std::ostream* os = &std::cout)
        : sw_(true)
        , freq_ {freq}
        , os_(os)
    {
    }

    void setFrequency(std::chrono::milliseconds freq)
    {
        freq_ = freq;
    }

    void update(const DisplayCb& cb)
    {
        if (sw_.elapsed() >= freq_)
        {
            cb(*os_);
            *os_ << '\r';
            os_->flush();
            sw_.restart();
        }
    }

private:
    StopWatch sw_;
    std::chrono::milliseconds freq_ {};
    std::ostream* os_ {&std::cout};
};

} // namespace tools::dups
