#pragma once

#include <ostream>
#include <streambuf>

namespace tools::strm {

class NullBuffer : public std::streambuf
{
protected:
    int overflow(int c) override
    {
        return c; // Discard character
    }
};

// Can be used to suppress output to the console or any other stream
class NullOStream : public std::ostream
{
public:
    NullOStream()
        : std::ostream(&nullBuffer_)
    {
    }

private:
    NullBuffer nullBuffer_;
};

} // namespace tools::strm
