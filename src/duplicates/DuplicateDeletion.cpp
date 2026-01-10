#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Progress.h>
#include <core/utils/Number.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <iostream>
#include <algorithm>
#include <iterator>
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

std::string& lastInput()
{
    static std::string s_lastInput;
    return s_lastInput;
}

std::string promptUser(std::ostream& out, std::istream& in)
{
    std::string input;

    while (in && input.empty())
    {
        out << "Enter a number to KEEP (q - quit, i - ignore, o - open dirs) > ";
        std::getline(in, input);

        if (input.empty())
        {
            input = lastInput();
        }
    }

    if (!input.empty())
    {
        lastInput() = input;
    }
    else
    {
        // The input is corrupted, treating it as a quit signal
        spdlog::error("Input stream is in bad state, quitting.");
        return "";
    }

    return input;
}

void openDirectories(const PathsVec& files)
{
    std::unordered_set<fs::path> uniqueDirs;

    for (const auto& file : files)
    {
        uniqueDirs.insert(file.parent_path());
    }

    for (const auto& dir : uniqueDirs)
    {
        file::openDirectory(dir);
    }
}

} // namespace

void deleteFiles(const IDeletionStrategy& strategy, PathsVec& files)
{
    for (const auto& file : files)
    {
        try
        {
            strategy.remove(file);
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
    size_t maxDisplayGroupSize = 10;
    const auto width = static_cast<int>(num::digits(files.size()));

    for (const auto& file : files)
    {
        ++i;
        if (i < maxDisplayGroupSize || i == files.size())
        {
            out << std::setw(width + 1) << i << ": " << file << '\n';
        }
        else if (i == maxDisplayGroupSize)
        {
            out << std::setw(width + 1) << ' ' << ": ..." << '\n';
        }
    }

    auto input = promptUser(out, in);

    while (!input.empty() && tolower(input[0]) == 'o')
    {
        openDirectories(files);
        input = promptUser(out, in);
    }

    if (input.empty())
    {
        return false;
    }

    if (tolower(input[0]) == 'q')
    {
        spdlog::info("User requested to stop deletion...");
        lastInput().clear();
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
        lastInput().clear();
    }

    return true;
};


void deleteDuplicates(const IDeletionStrategy& strategy,
                      const IDuplicateGroups& duplicates,
                      const PathsVec& dirsToDeleteFrom,
                      IgnoredFiles& ignoredFiles,
                      Progress& progress,
                      std::ostream& out,
                      std::istream& in)
{
    PathsVec deleteWithoutAsking;
    PathsVec deleteSelectively;

    size_t numDuplicates = duplicates.numGroups();
    bool resumeEnumeration = true;
    bool sensitiveToExternalEvents = false;

    duplicates.enumGroups(
        [&, numDuplicates, numGroup = 0](const DupGroup& group) mutable {
            ++numGroup;
            deleteWithoutAsking.clear();
            deleteSelectively.clear();

            progress.update([&numGroup, &numDuplicates](std::ostream& os) {
                os << "Deleting duplicate group " << numGroup << " out of "
                   << numDuplicates << '\n';
            });

            // Categorize files into 2 groups
            for (const auto& e : group.entires)
            {
                // In case if the file is deleted manually by the user while the
                // interactive file deletion was running
                if (sensitiveToExternalEvents && !fs::exists(e.file))
                {
                    continue;
                }

                if (ignoredFiles.contains(e.file))
                {
                    spdlog::warn("File is ignored: {}", e.file);
                    return true;
                }

                if (isSafeToDelete(dirsToDeleteFrom, e.file.parent_path()))
                {
                    deleteWithoutAsking.emplace_back(e.file);
                }
                else
                {
                    deleteSelectively.emplace_back(e.file);
                }
            }

            // Not all files are in the directory that is marked to delete from
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
                std::ranges::sort(deleteSelectively);
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
