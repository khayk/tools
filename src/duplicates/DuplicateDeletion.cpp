#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DeletionStrategy.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Number.h>

#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace tools::dups {

namespace {

bool isSafeToDelete(const PathsVec& delDirs, const fs::path& path)
{
    const auto& pathStr = path.native();

    return std::ranges::any_of(delDirs, [&pathStr](const auto& deleteDir) {
        return pathStr.find(deleteDir.native(), 0) != std::string::npos;
    });
};

} // namespace

IgnoredFiles::IgnoredFiles(fs::path file, bool saveWhenDone)
    : filePath_(std::move(file))
    , saveWhenDone_(saveWhenDone)
{
    if (fs::exists(filePath_))
    {
        load();
    }
}

IgnoredFiles::~IgnoredFiles()
{
    try
    {
        if (saveWhenDone_ || fs::exists(filePath_))
        {
            save();
        }
    }
    catch (const std::exception& ex)
    {
        spdlog::error("Failed to save ignored files to: {}, exception: ", filePath_, ex.what());
    }
}

bool IgnoredFiles::contains(const fs::path& file) const
{
    return files_.contains(file);
}

bool IgnoredFiles::empty() const noexcept
{
    return files_.empty();
}


size_t IgnoredFiles::size() const noexcept
{
    return files_.size();
}

const PathsSet& IgnoredFiles::files() const
{
    return files_;
}

void IgnoredFiles::add(const fs::path& file)
{
    files_.insert(file);
}

void IgnoredFiles::add(const PathsVec& files)
{
    std::ranges::copy(files, std::inserter(files_, files_.end()));
}

void IgnoredFiles::load()
{
    spdlog::info("Loading ignored files from: {}", filePath_);
    file::readLines(filePath_, [&](const std::string& line) {
        if (!line.empty())
        {
            files_.emplace(line);
        }
        return true;
    });
}

void IgnoredFiles::save() const
{
    if (files_.empty())
    {
        return;
    }

    std::ofstream out(filePath_, std::ios::out | std::ios::binary);
    for (const auto& file : files_)
    {
        out << file::path2s(file) << '\n';
    }
}

void deleteFiles(const IDeletionStrategy& strategy, PathsVec& files)
{
    for (const auto& file : files)
    {
        try
        {
            strategy.apply(file);
        }
        catch (const std::exception& e)
        {
            spdlog::error("Error '{}' while deleting file '{}'", e.what(), file);
        }
    }

    files.clear();
};

bool deleteInteractively(const IDeletionStrategy& strategy,
                         PathsVec& files,
                         IgnoredFiles& ignoredFiles,
                         std::ostream& out,
                         std::istream& in)
{
    // Display files to be deleted
    size_t i = 0;
    for (const auto& file : files)
    {
        const auto width = static_cast<int>(num::digits(files.size()));
        out << std::setw(width + 1) << ++i << ": " << file << '\n';
    }

    static std::string lastInput;
    std::string input;

    while (input.empty())
    {
        out << "Enter number to KEEP (q - quit, i - ignore) > ";
        std::getline(in, input);

        if (input.empty())
        {
            input = lastInput;
        }
    }

    if (!input.empty())
    {
        lastInput = input;
    }

    if (tolower(input[0]) == 'q')
    {
        spdlog::info("User requested to stop deletion...");
        return false;
    }

    if (tolower(input[0]) == 'i')
    {
        spdlog::info("User requested to ignore this group...");
        ignoredFiles.add(files);
        return true;
    }

    const auto choice = num::s2num<size_t>(input);
    if (choice > 0 && choice <= files.size())
    {
        std::swap(files[choice - 1], files.back());
        files.pop_back();
        deleteFiles(strategy, files);
    }
    else
    {
        out << "Invalid choice, no file is deleted.\n";
    }

    return true;
};


void deleteDuplicates(const IDeletionStrategy& strategy,
                      const DuplicateDetector& detector,
                      const PathsVec& safeToDeleteDirs,
                      IgnoredFiles& ignoredFiles,
                      std::ostream& out,
                      std::istream& in)
{
    PathsVec deleteWithoutAsking;
    PathsVec deleteSelectively;
    bool resumeEnumeration = true;

    detector.enumDuplicates([&](const DupGroup& group) {
        deleteWithoutAsking.clear();
        deleteSelectively.clear();

        // Categorize files into 2 groups
        for (const auto& e : group.entires)
        {
            if (ignoredFiles.contains(e.file))
            {
                spdlog::warn("File is ignored: {}", e.file);
                return true;
            }

            if (isSafeToDelete(safeToDeleteDirs, e.file.parent_path()))
            {
                deleteWithoutAsking.emplace_back(e.file);
            }
            else
            {
                deleteSelectively.emplace_back(e.file);
            }
        }

        // Not all files fall under the safe to delete category
        if (!deleteSelectively.empty())
        {
            // So after deleting the files below, we know that at least one
            // file will be left in the system
            deleteFiles(strategy, deleteWithoutAsking);
        }
        else
        {
            deleteSelectively.insert(
                deleteSelectively.end(),
                std::make_move_iterator(deleteWithoutAsking.begin()),
                std::make_move_iterator(deleteWithoutAsking.end()));
        }

        if (deleteSelectively.size() > 1)
        {
            out << "file size: " << group.entires.front().size
                << ", sha256: " << group.entires.front().sha256 << '\n';
            resumeEnumeration = deleteInteractively(strategy,
                                                    deleteSelectively,
                                                    ignoredFiles,
                                                    out,
                                                    in);
        }

        // We left with one file, which is now unique in the system
        return resumeEnumeration;
    });
}

} // namespace tools::dups
