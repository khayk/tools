#include <kidmon/repo/MultiRepository.h>

#include <spdlog/spdlog.h>

#include <exception>
#include <stdexcept>

namespace km {

MultiRepository::MultiRepository(
    std::vector<std::reference_wrapper<IRepository>> repos)
    : repos_(std::move(repos))
{
    if (repos_.empty())
    {
        throw std::invalid_argument(
            "MultiRepository requires at least one repository");
    }
}

MultiRepository::MultiRepository(IRepository& primary, IRepository& secondary)
    : MultiRepository(
          std::vector<std::reference_wrapper<IRepository>> {primary, secondary})
{
}

template <typename Write>
void MultiRepository::fanOut(const Write& write)
{
    // Every repository gets a write attempt; only the primary's failure propagates.
    std::exception_ptr primaryFailure;

    for (std::size_t i = 0; i < repos_.size(); ++i)
    {
        try
        {
            write(repos_[i].get());
        }
        catch (const std::exception& ex)
        {
            if (i == 0)
            {
                primaryFailure = std::current_exception();
            }
            else
            {
                spdlog::error("MultiRepository: secondary repository #{} failed to "
                              "persist: {}",
                              i,
                              ex.what());
            }
        }
    }

    if (primaryFailure)
    {
        std::rethrow_exception(primaryFailure);
    }
}

void MultiRepository::add(const Entry& entry)
{
    fanOut([&](IRepository& repo) {
        repo.add(entry);
    });
}

void MultiRepository::addAll(std::span<const Entry> entries)
{
    fanOut([&](IRepository& repo) {
        repo.addAll(entries);
    });
}

void MultiRepository::queryUsers(const UserCb& cb) const
{
    repos_.front().get().queryUsers(cb);
}

void MultiRepository::queryEntries(const Filter& filter, const EntryCb& cb) const
{
    repos_.front().get().queryEntries(filter, cb);
}

} // namespace km
