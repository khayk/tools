#include <duplicates/PathList.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <spdlog/spdlog.h>

#include <iterator>
#include <fstream>
#include <stdexcept>

namespace tools::dups {

PathList::PathList(fs::path file, bool saveWhenDone)
    : filePath_(std::move(file))
    , saveWhenDone_(saveWhenDone)
{
    if (fs::exists(filePath_))
    {
        load();
    }
}

PathList::~PathList()
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

bool PathList::contains(const fs::path& path) const
{
    return paths_.contains(path);
}

bool PathList::empty() const noexcept
{
    return paths_.empty();
}


size_t PathList::size() const noexcept
{
    return paths_.size();
}

const PathsSet& PathList::files() const
{
    return paths_;
}

void PathList::add(const fs::path& path)
{
    paths_.insert(path);
}

void PathList::add(const PathsVec& paths)
{
    std::ranges::copy(paths, std::inserter(paths_, paths_.end()));
}

void PathList::load()
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

void PathList::save() const
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
