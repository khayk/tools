#include <duplicates/FileList.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <spdlog/spdlog.h>

#include <iterator>
#include <fstream>
#include <stdexcept>

namespace tools::dups {

FileList::FileList(fs::path file, bool saveWhenDone)
    : filePath_(std::move(file))
    , saveWhenDone_(saveWhenDone)
{
    if (fs::exists(filePath_))
    {
        load();
    }
}

FileList::~FileList()
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

bool FileList::contains(const fs::path& file) const
{
    return files_.contains(file);
}

bool FileList::empty() const noexcept
{
    return files_.empty();
}


size_t FileList::size() const noexcept
{
    return files_.size();
}

const PathsSet& FileList::files() const
{
    return files_;
}

void FileList::add(const fs::path& file)
{
    files_.insert(file);
}

void FileList::add(const PathsVec& files)
{
    std::ranges::copy(files, std::inserter(files_, files_.end()));
}

void FileList::load()
{
    spdlog::info("Loading files from: {}", filePath_);
    file::readLines(filePath_, [&](const std::string& line) {
        if (!line.empty())
        {
            files_.emplace(line);
        }
        return true;
    });
}

void FileList::save() const
{
    if (files_.empty())
    {
        return;
    }

    std::ofstream out(filePath_, std::ios::out | std::ios::binary);

    if (!out)
    {
        throw std::runtime_error(fmt::format("Unable to open file: '{}'", filePath_));
    }

    for (const auto& file : files_)
    {
        out << file::path2s(file) << '\n';
    }
}

} // namespace tools::dups
