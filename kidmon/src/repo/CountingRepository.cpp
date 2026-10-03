#include <kidmon/repo/CountingRepository.h>

namespace km {

CountingRepository::CountingRepository(IRepository& inner)
    : inner_(inner)
{
}

void CountingRepository::add(const Entry& entry)
{
    try
    {
        inner_.add(entry);
        ++stored_;
    }
    catch (...)
    {
        ++failed_;
        throw;
    }
}

void CountingRepository::addAll(std::span<const Entry> entries)
{
    // A failed batch may be partially stored; count it all as failed.
    try
    {
        inner_.addAll(entries);
        stored_ += entries.size();
    }
    catch (...)
    {
        failed_ += entries.size();
        throw;
    }
}

void CountingRepository::queryUsers(const UserCb& cb) const
{
    inner_.queryUsers(cb);
}

void CountingRepository::queryEntries(const Filter& filter, const EntryCb& cb) const
{
    inner_.queryEntries(filter, cb);
}

CountingRepository::Counts CountingRepository::take() noexcept
{
    return {.stored = stored_.exchange(0), .failed = failed_.exchange(0)};
}

} // namespace km
