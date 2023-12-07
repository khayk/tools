#include "StringSource.h"

namespace data {

StringSource::StringSource(std::string_view src)
    : src_(src)
    , sv_(src_)
{
}


size_t StringSource::size() const noexcept
{
    return src_.size();
}


size_t StringSource::get(std::string& buf, size_t maxSize)
{
    const auto bytes = std::min(sv_.size(), maxSize);
    buf.append(sv_.data(), bytes);
    sv_.remove_prefix(bytes);

    return bytes;
}
} // namespace data
