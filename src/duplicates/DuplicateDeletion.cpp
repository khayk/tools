#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Progress.h>
#include <core/utils/Number.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <algorithm>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <cassert>

#include <spdlog/spdlog.h>

namespace tools::dups {

namespace {

bool findPath(const PathsSet& delDirs, const fs::path& path)
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

bool deduceTheOneToKeep(const IDeletionStrategy& strategy,
                        PathsVec& files,
                        KeepFromPaths& keepFromPaths)
{
    // Find the first file that should be kept
    auto it = std::ranges::find_if(files, [&](const auto& file) {
        return findPath(keepFromPaths.files(), file.parent_path());
    });

    // If no file needs to be kept, we can't "deduce the one," so we exit
    if (it == files.end()) {
        return false;
    }

    // Check if there is a SECOND file that matches (ambiguity check)
    auto nextMatch = std::find_if(std::next(it), files.end(), [&](const auto& file) {
        return findPath(keepFromPaths.files(), file.parent_path());
    });

    if (nextMatch != files.end()) {
        return false; // Multiple candidates found; conflict!
    }

    // Move the 'keep' file out of the deletion list
    // We swap it to the end and pop it so 'files' now only contains targets for deletion
    std::iter_swap(it, std::prev(files.end()));
    files.pop_back();
    deleteFiles(strategy, files);

    return true;
}

bool deleteInteractively(const IDeletionStrategy& strategy,
                         PathsVec& files,
                         IgnoredPaths& ignoredPaths,
                         KeepFromPaths& keepFromPaths,
                         std::ostream& out,
                         std::istream& in)
{
    // Find out if there is a single file under the to "keep from" directories.
    // if there is only one such file, keep it and move on
    if (deduceTheOneToKeep(strategy, files, keepFromPaths))
    {
        return true;
    }

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
        ignoredPaths.add(files);
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


DeletionConfig::DeletionConfig(const IDuplicateGroups& duplicates,
                               const IDeletionStrategy& strategy,
                               std::ostream& out,
                               std::istream& in)
    : duplicates_ {&duplicates}
    , strategy_ {&strategy}
    , out_ {&out}
    , in_ {&in}
{
    static IgnoredPaths s_ignorePaths;
    static KeepFromPaths s_keepPaths;
    static DeleteFromPaths s_deletePaths;

    ignored_ = &s_ignorePaths;
    keepFrom_ = &s_keepPaths;
    deleteFrom_ = &s_deletePaths;
}

void DeletionConfig::setProgress(Progress& progress)
{
    progress_ = &progress;
}

void DeletionConfig::setIgnoredPaths(IgnoredPaths& ignored)
{
    ignored_ = &ignored;
}

void DeletionConfig::setKeepFromPaths(KeepFromPaths& keepFrom)
{
    keepFrom_ = &keepFrom;
}

void DeletionConfig::setDeleteFromPaths(DeleteFromPaths& deleteFrom)
{
    deleteFrom_ = &deleteFrom;
}

const IDuplicateGroups& DeletionConfig::duplicates() const
{
    return *duplicates_;
}

const IDeletionStrategy& DeletionConfig::strategy() const
{
    return *strategy_;
}

std::ostream& DeletionConfig::out()
{
    return *out_;
}

std::istream& DeletionConfig::in()
{
    return *in_;
}

Progress* DeletionConfig::progress()
{
    return progress_;
}

IgnoredPaths& DeletionConfig::ignoredPaths()
{
    return *ignored_;
}

KeepFromPaths& DeletionConfig::keepFromPaths()
{
    return *keepFrom_;
}

DeleteFromPaths& DeletionConfig::deleteFromPaths()
{
    return *deleteFrom_;
}

class GroupProcessor {
    DeletionConfig& cfg;
    bool sensitiveToExternalEvents = false;
    PathsVec autoDelete;
    PathsVec selective;

public:
    GroupProcessor(DeletionConfig& dcfg) : cfg(dcfg) {}

    // The entry point for the loop
    bool process(const DupGroup& group, size_t groupIdx, size_t totalGroups) {
        updateProgress(groupIdx, totalGroups);
        categorizeFiles(group.entires);

        // Safety: If every file is in a "DeleteFrom" path, we must treat them
        // as selective to avoid deleting the entire group by accident.
        if (selective.empty()) {
            selective = std::move(autoDelete);
            autoDelete.clear();
        } else {
            // Delete the "unwanted" ones immediately, keeping the "selective" ones for review
            deleteFiles(cfg.strategy(), autoDelete);
        }

        return handleReview(group, selective);
    }

private:
    void updateProgress(size_t current, size_t total) {
        if (auto* p = cfg.progress()) {
            p->update([&](auto& os) {
                os << "Processing group " << current << " of " << total << '\n';
            });
        }
    }

    void categorizeFiles(const std::vector<DupEntry>& entries) {
        autoDelete.clear();
        selective.clear();
        for (const auto& e : entries) {
            if (sensitiveToExternalEvents && !fs::exists(e.file)) continue;
            if (cfg.ignoredPaths().contains(e.file)) continue;

            if (findPath(cfg.deleteFromPaths().files(), e.file.parent_path())) {
                autoDelete.push_back(e.file);
            } else {
                selective.push_back(e.file);
            }
        }
    }

    bool handleReview(const DupGroup& group, PathsVec& selective) {
        if (selective.size() <= 1) return true;

        cfg.out() << "Size: " << group.entires.front().size
                  << " SHA256: " << group.entires.front().sha256 << '\n';

        std::ranges::sort(selective);
        return deleteInteractively(cfg.strategy(), selective, cfg.ignoredPaths(),
                                   cfg.keepFromPaths(), cfg.out(), cfg.in());
    }
};

// The main function is now very clean
void deleteDuplicates(DeletionConfig& cfg) {
    GroupProcessor processor {cfg};
    size_t total = cfg.duplicates().numGroups();

    cfg.duplicates().enumGroups([&, i = 0ULL](const DupGroup& group) mutable {
        return processor.process(group, ++i, total);
    });
}

} // namespace tools::dups
