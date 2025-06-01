#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DeletionStrategy.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Number.h>

#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace tools::dups {

namespace {

bool isSafeToDelete(const std::vector<std::string>& delDirs, const fs::path& path)
{
    const auto pathStr = path.string();

    return std::ranges::any_of(delDirs, [&pathStr](const auto& deleteDir) {
        return pathStr.find(deleteDir, 0) != std::string::npos;
    });
};

void saveIgnoredFiles(const fs::path& filePath, const PathsSet& files)
{
    if (files.empty())
    {
        return;
    }

    std::ofstream out(filePath, std::ios::out | std::ios::binary);
    for (const auto& file : files)
    {
        out << file::path2s(file) << '\n';
    }
};

} // namespace

void deleteFiles(IDeletionStrategy& strategy, PathsVec& files)
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

bool deleteInteractively(IDeletionStrategy& strategy,
                         PathsVec& files,
                         PathsSet& ignoredFiles,
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
        std::ranges::copy(files, std::inserter(ignoredFiles, ignoredFiles.end()));
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


void deleteDuplicates(const Config& cfg,
                      const DuplicateDetector& detector,
                      std::ostream& out,
                      std::istream& in)
{
    PathsSet ignoredFiles;
    PathsVec deleteWithoutAsking;
    PathsVec deleteSelectively;

    std::unique_ptr<IDeletionStrategy> strategy;
    if (cfg.dryRun)
    {
        strategy = std::make_unique<DryRunDelete>();
    }
    else
    {
        strategy = std::make_unique<BackupAndDelete>(cfg.cacheDir);
    }

    bool resumeEnumeration = true;

    if (fs::exists(cfg.ignFilesPath))
    {
        spdlog::info("Loading ignored files from: {}", cfg.ignFilesPath);
        file::readLines(cfg.ignFilesPath, [&](const std::string& line) {
            if (!line.empty())
            {
                ignoredFiles.emplace(line);
            }
            return true;
        });
    }

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

            if (isSafeToDelete(cfg.safeToDeleteDirs, e.file.parent_path()))
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
            deleteFiles(*strategy, deleteWithoutAsking);
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
            resumeEnumeration = deleteInteractively(*strategy,
                                                    deleteSelectively,
                                                    ignoredFiles,
                                                    out,
                                                    in);
        }

        // We left with one file, which is now unique in the system
        return resumeEnumeration;
    });

    saveIgnoredFiles(cfg.ignFilesPath, ignoredFiles);
}

} // namespace tools::dups
