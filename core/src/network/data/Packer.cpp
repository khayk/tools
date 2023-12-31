#include <core/network/data/Packer.h>

namespace data {

Packer::Packer(ISource& source)
    : source_(source)
{
}


size_t Packer::get(std::string& buf, size_t maxSize)
{
    if (writeSize_)
    {
        const size_t size = source_.size();
        buf.append(reinterpret_cast<const char*>(&size), sizeof(size));
        writeSize_ = false;
    }

    return source_.get(buf, maxSize);
}


} // namespace data
