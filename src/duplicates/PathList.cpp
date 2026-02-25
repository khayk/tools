#include <duplicates/PathList.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <spdlog/spdlog.h>

#include <iterator>
#include <fstream>
#include <stdexcept>

namespace tools::dups {

PathsImpl::PathsImpl(const PathsVec& paths)
{
    add(paths);
}

PathsImpl::PathsImpl(PathsSet paths) noexcept
    : paths_(std::move(paths))
{
}

bool PathsImpl::contains(const fs::path& path) const
{
    return paths_.contains(path);
}

bool PathsImpl::empty() const noexcept
{
    return paths_.empty();
}

size_t PathsImpl::size() const noexcept
{
    return paths_.size();
}

PathsSet& PathsImpl::paths()
{
    return paths_;
}

const PathsSet& PathsImpl::paths() const
{
    return paths_;
}

void PathsImpl::add(const fs::path& path)
{
    paths_.insert(path);
}

void PathsImpl::add(const PathsVec& paths)
{
    std::ranges::copy(paths, std::inserter(paths_, paths_.end()));
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

PathsPersister::PathsPersister(PathsSet& paths, fs::path filePath, bool saveWhenDone)
    : paths_(paths)
    , filePath_(std::move(filePath))
    , saveWhenDone_(saveWhenDone)
{
    if (fs::exists(filePath_))
    {
        load();
    }
}

PathsPersister::~PathsPersister()
{
    try
    {
        if (saveWhenDone_ && !filePath_.empty())
        {
            save();
        }
    }
    catch (const std::exception& ex)
    {
        spdlog::error("Failed to save files to: {}, exception: {}",
                      filePath_,
                      ex.what());
    }
}

void PathsPersister::load()
{
    spdlog::info("Loading files from: {}", filePath_);
    file::readLines(filePath_, [&](const std::string& line) {
        if (!line.empty())
        {
            paths_.emplace(line);
        }
        return true;
    });
}

void PathsPersister::save() const
{
    if (paths_.empty())
    {
        return;
    }

    std::ofstream out(filePath_, std::ios::out | std::ios::binary);

    if (!out)
    {
        throw std::runtime_error(fmt::format("Unable to open file: '{}'", filePath_));
    }

    for (const auto& file : paths_)
    {
        out << file::path2s(file) << '\n';
    }
}

} // namespace tools::dups
