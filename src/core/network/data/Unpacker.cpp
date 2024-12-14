#include <core/network/data/Unpacker.h>

namespace data {

void Unpacker::put(std::string_view bytes)
{
    buffer_ += bytes;
    readSize();

    if (!bytes.empty())
    {
        status_ = Status::HasMore;
    }
}


size_t Unpacker::size() const noexcept
{
    return size_;
}


Unpacker::Status Unpacker::status() const noexcept
{
    return status_;
}


Unpacker::Status Unpacker::get(std::string& buf, size_t maxSize)
{
    status_ = Status::NeedMore;
    const auto bytes = std::min({rem_, maxSize, buffer_.size() - off_});

    if (bytes == 0)
    {
        return status_;
    }

    buf.append(buffer_.data() + off_, bytes);
    off_ += bytes;
    rem_ -= bytes;
    status_ = Status::HasMore;

    if (rem_ == 0)
    {
        buffer_.erase(0, off_);
        off_ = 0;
        status_ = Status::Ready;
    }
    else if (off_ > maxSize)
    {
        buffer_.erase(0, off_);
        off_ = 0;
    }

    readSize();
    return status_;
}


void Unpacker::readSize() noexcept
{
    if (rem_ == 0 && buffer_.size() >= sizeof(size_t))
    {
        rem_ = *reinterpret_cast<size_t*>(buffer_.data());
        off_ += sizeof(size_t);
        size_ = rem_;
    }
}

} // namespace data
