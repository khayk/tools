#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Progress.h>
#include <core/utils/Number.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
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
        out << "Enter a number to KEEP  ('i' - ignore, 'o' - open dirs, 'q' - quit) > ";
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
    if (it == files.end())
    {
        return false;
    }

    // Check if there is a SECOND file that matches (ambiguity check)
    auto nextMatch = std::find_if(std::next(it), files.end(), [&](const auto& file) {
        return findPath(keepFromPaths.files(), file.parent_path());
    });

    if (nextMatch != files.end())
    {
        return false; // Multiple candidates found; conflict!
    }

    // Move the 'keep' file out of the deletion list
    // We swap it to the end and pop it so 'files' now only contains targets for
    // deletion
    std::iter_swap(it, std::prev(files.end()));
    files.pop_back();
    deleteFiles(strategy, files);

    return true;
}

enum class UserAction : std::uint8_t
{
    KeepIndex,
    IgnoreGroup,
    OpenFolders,
    Quit,
    Retry
};

struct ParsedInput
{
    UserAction action {};
    size_t index = 0;
};

// Handles the console rendering logic
void displayFileOptions(std::ostream& out, const PathsVec& files)
{
    const size_t maxVisible = 10;
    const int width = static_cast<int>(num::digits(files.size()));

    for (size_t i = 0; i < files.size(); ++i)
    {
        if (i < maxVisible - 1 || i == files.size() - 1)
        {
            out << std::setw(width + 1) << (i + 1) << ": " << files[i] << '\n';
        }
        else if (i == maxVisible - 1)
        {
            out << std::setw(width + 1) << " " << ": ...\n";
        }
    }
}

// Translates string input into a structured Command
ParsedInput parseUserInput(const std::string& input, size_t maxIdx)
{
    if (input.empty())
    {
        return {UserAction::Quit};
    }
    char cmd = static_cast<char>(std::tolower(input[0]));

    if (cmd == 'q')
    {
        return {UserAction::Quit};
    }
    if (cmd == 'i')
    {
        return {UserAction::IgnoreGroup};
    }
    if (cmd == 'o')
    {
        return {UserAction::OpenFolders};
    }

    const auto choice = num::s2num<size_t>(input);
    if (choice > 0 && choice <= maxIdx)
    {
        return {UserAction::KeepIndex, choice};
    }

    return {UserAction::Retry, choice};
}

bool deleteInteractively(const IDeletionStrategy& strategy,
                         PathsVec& files,
                         IgnoredPaths& ignoredPaths,
                         KeepFromPaths& keepFromPaths,
                         std::ostream& out,
                         std::istream& in)
{
    // Try automatic resolution first
    if (deduceTheOneToKeep(strategy, files, keepFromPaths))
    {
        return true;
    }

    // UI Loop
    while (true)
    {
        displayFileOptions(out, files);
        auto input = parseUserInput(promptUser(out, in), files.size());

        switch (input.action)
        {
            case UserAction::OpenFolders:
                openDirectories(files);
                continue; // Re-prompt after opening folders

            case UserAction::IgnoreGroup:
                spdlog::info("Ignoring group...");
                ignoredPaths.add(files);
                return true;

            case UserAction::Quit:
                lastInput().clear();
                spdlog::info("Stopping deletion...");
                return false;

            case UserAction::KeepIndex:
                // The user chose which one to KEEP. Swap it away and delete the rest.
                std::swap(files[input.index - 1], files.back());
                files.pop_back();
                deleteFiles(strategy, files);
                return true;

            case UserAction::Retry:
            default:
                // We don't return false here so the user gets another chance
                out << "'" << input.index << "'" " is an invalid choice, no file is deleted.\n";
        }
    }
}


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

class GroupProcessor
{
    DeletionConfig& cfg;
    bool sensitiveToExternalEvents = false;
    PathsVec autoDelete;
    PathsVec selective;

public:
    GroupProcessor(DeletionConfig& dcfg)
        : cfg(dcfg)
    {
    }

    // The entry point for the loop
    bool process(const DupGroup& group, size_t groupIdx, size_t totalGroups)
    {
        updateProgress(groupIdx, totalGroups);
        categorizeFiles(group.entires);

        // Safety: If every file is in a "DeleteFrom" path, we must treat them
        // as selective to avoid deleting the entire group by accident.
        if (selective.empty())
        {
            selective = std::move(autoDelete);
            autoDelete.clear();
        }
        else
        {
            // Delete the "unwanted" ones immediately, keeping the "selective" ones for
            // review
            deleteFiles(cfg.strategy(), autoDelete);
        }

        return handleReview(group, selective);
    }

private:
    void updateProgress(size_t current, size_t total)
    {
        if (auto* p = cfg.progress())
        {
            p->update([&](auto& os) {
                os << "Processing group " << current << " of " << total << '\n';
            });
        }
    }

    void categorizeFiles(const std::vector<DupEntry>& entries)
    {
        autoDelete.clear();
        selective.clear();
        for (const auto& e : entries)
        {
            if (sensitiveToExternalEvents && !fs::exists(e.file))
            {
                continue;
            }
            if (cfg.ignoredPaths().contains(e.file))
            {
                continue;
            }

            if (findPath(cfg.deleteFromPaths().files(), e.file.parent_path()))
            {
                autoDelete.push_back(e.file);
            }
            else
            {
                selective.push_back(e.file);
            }
        }
    }

    bool handleReview(const DupGroup& group, PathsVec& selective)
    {
        if (selective.size() <= 1)
        {
            return true;
        }

        cfg.out() << "Size: " << group.entires.front().size
                  << " SHA256: " << group.entires.front().sha256 << '\n';

        std::ranges::sort(selective);
        return deleteInteractively(cfg.strategy(),
                                   selective,
                                   cfg.ignoredPaths(),
                                   cfg.keepFromPaths(),
                                   cfg.out(),
                                   cfg.in());
    }
};

// The main function is now very clean
void deleteDuplicates(DeletionConfig& cfg)
{
    GroupProcessor processor {cfg};
    size_t total = cfg.duplicates().numGroups();

    cfg.duplicates().enumGroups([&, i = 0ULL](const DupGroup& group) mutable {
        return processor.process(group, ++i, total);
    });
}

} // namespace tools::dups
