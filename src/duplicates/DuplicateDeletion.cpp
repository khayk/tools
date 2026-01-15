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
#include <tuple>

#include <spdlog/spdlog.h>

namespace tools::dups {

constexpr std::string_view MAIN_PROMPT = "Enter a number to KEEP  ( 'i' - ignore, 'o' - open dirs, 'k' - edit keep from, 'd' - edit delete from, 'q' - quit ) > ";
constexpr std::string_view KEEP_PROMPT = "Chose a number to mark as 'KEEP FROM'  ( 'b' - back, 'q' - quit ) > ";
constexpr std::string_view DELT_PROMPT = "Chose a number to mark as 'DELETE FROM'  ( 'b' - back, 'q' - quit ) > ";

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

std::string promptUser(std::string_view prompt, std::ostream& out, std::istream& in)
{
    std::string input;

    while (in && input.empty())
    {
        out << prompt;
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

enum class UserChoice : std::uint8_t
{
    Number,
    IgnoreGroup,
    OpenFolders,
    EditKeepFrom,
    EditDeleteFrom,
    GoBack,
    Retry,
    Quit
};

struct ParsedInput
{
    UserChoice action {};
    size_t index = 0;
};

// Translates string input into a structured Command
ParsedInput parseUserInput(const std::string& input, size_t maxIdx)
{
    if (input.empty())
    {
        return {UserChoice::Quit};
    }
    char cmd = static_cast<char>(std::tolower(input[0]));

    if (cmd == 'q')
    {
        return {UserChoice::Quit};
    }
    if (cmd == 'i')
    {
        return {UserChoice::IgnoreGroup};
    }
    if (cmd == 'o')
    {
        return {UserChoice::OpenFolders};
    }
    if (cmd == 'k')
    {
        return {UserChoice::EditKeepFrom};
    }
    if (cmd == 'd')
    {
        return {UserChoice::EditDeleteFrom};
    }
    if (cmd == 'b')
    {
        return {UserChoice::GoBack};
    }

    const auto choice = num::s2num<size_t>(input);
    if (choice > 0 && choice <= maxIdx)
    {
        return {UserChoice::Number, choice};
    }

    return {UserChoice::Retry, choice};
}

PathsVec createDirectoriesList(const PathsVec& files)
{
    PathsVec dirs;
    fs::path prefix;
    std::map<fs::path, int> frequency;

    for (const auto& file : files)
    {
        prefix.clear();
        for (const auto& part : file)
        {
            prefix /= part;
            ++frequency[prefix];
        }

        frequency[file] = std::numeric_limits<int>::max(); // exclude full paths
    }

    for (const auto& [p, count] : frequency)
    {
        if (count == 1)
        {
            dirs.push_back(p);
        }
    }

    return dirs;
}

// Handles the console rendering logic
void displayPathOptions(std::ostream& out, const PathsVec& paths)
{
    const size_t maxVisible = 10;
    const int width = static_cast<int>(num::digits(paths.size()));

    for (size_t i = 0; i < paths.size(); ++i)
    {
        if (i < maxVisible - 1 || i == paths.size() - 1)
        {
            out << std::setw(width + 1) << (i + 1) << ": " << paths[i] << '\n';
        }
        else if (i == maxVisible - 1)
        {
            out << std::setw(width + 1) << " " << ": ...\n";
        }
    }
}

template <typename Paths>
bool editConfig(const PathsVec& files,
                std::string_view prompt,
                Paths& paths,
                std::ostream& out,
                std::istream& in)
{
    auto dirs = createDirectoriesList(files);

    while (true)
    {
        displayPathOptions(out, dirs);
        auto input = parseUserInput(promptUser(prompt, out, in), dirs.size());
        std::ignore = input;

        switch (input.action)
        {
            case UserChoice::Quit:
                lastInput().clear();
                return false;

            case UserChoice::GoBack:
                return true;

            case UserChoice::Number:
                // Selected path will be added into directories of the given paths
                paths.add(dirs[input.index - 1]);
                return true;

            case UserChoice::Retry:
            default:
                // We don't return false here so the user gets another chance
                out << "'" << input.index << "'" " is an invalid choice.\n";
        }
    }

    return true;
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
        return findPath(keepFromPaths.paths(), file.parent_path());
    });

    // If no file needs to be kept, we can't "deduce the one," so we exit
    if (it == files.end())
    {
        return false;
    }

    // Check if there is a SECOND file that matches (ambiguity check)
    auto nextMatch = std::find_if(std::next(it), files.end(), [&](const auto& file) {
        return findPath(keepFromPaths.paths(), file.parent_path());
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

bool deleteInteractively(PathsVec& files,
                         DeletionConfig& cfg)
{
    // Try automatic resolution first
    if (deduceTheOneToKeep(cfg.strategy(), files, cfg.keepFromPaths()))
    {
        return true;
    }

    // UI Loop
    while (true)
    {
        displayPathOptions(cfg.out(), files);
        auto input = parseUserInput(promptUser(MAIN_PROMPT, cfg.out(), cfg.in()), files.size());

        switch (input.action)
        {
            case UserChoice::EditKeepFrom:
                if (!editConfig(files, KEEP_PROMPT, cfg.keepFromPaths(), cfg.out(), cfg.in()))
                {
                    // Stop is requested
                    spdlog::info("Stopping deletion...");
                    return false;
                }
                continue; // Re-prompt after opening folders

            case UserChoice::EditDeleteFrom:
                if (!editConfig(files, DELT_PROMPT, cfg.deleteFromPaths(), cfg.out(), cfg.in()))
                {
                    // Stop is requested
                    spdlog::info("Stopping deletion...");
                    return false;
                }
                continue; // Re-prompt after opening folders

            case UserChoice::OpenFolders:
                openDirectories(files);
                continue; // Re-prompt after opening folders

            case UserChoice::IgnoreGroup:
                spdlog::info("Ignoring group...");
                cfg.ignoredPaths().add(files);
                return true;

            case UserChoice::Quit:
                lastInput().clear();
                spdlog::info("Stopping deletion...");
                return false;

            case UserChoice::Number:
                // The user chose which one to KEEP. Swap it away and delete the rest.
                std::swap(files[input.index - 1], files.back());
                files.pop_back();
                deleteFiles(cfg.strategy(), files);
                return true;

            case UserChoice::Retry:
            default:
                // We don't return false here so the user gets another chance
                cfg.out() << "'" << input.index << "'" " is an invalid choice, no file is deleted.\n";
        }
    }
}


DeletionConfig::DeletionConfig(const IDeletionStrategy& strategy,
                               std::ostream& out,
                               std::istream& in,
                               Progress& progress)
    : strategy_ {strategy}
    , out_ {out}
    , in_ {in}
    , progress_ {progress}
{
}

const IDeletionStrategy& DeletionConfig::strategy() const
{
    return strategy_;
}

std::ostream& DeletionConfig::out()
{
    return out_;
}

std::istream& DeletionConfig::in()
{
    return in_;
}

Progress& DeletionConfig::progress()
{
    return progress_;
}

IgnoredPaths& DeletionConfig::ignoredPaths()
{
    return ignored_;
}

KeepFromPaths& DeletionConfig::keepFromPaths()
{
    return keepFrom_;
}

DeleteFromPaths& DeletionConfig::deleteFromPaths()
{
    return deleteFrom_;
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
        cfg.progress().update([&](auto& os) {
            os << "Processing group " << current << " of " << total << '\n';
        });
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

            if (findPath(cfg.deleteFromPaths().paths(), e.file.parent_path()))
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
        return deleteInteractively(selective, cfg);
    }
};

// The main function is now very clean
void deleteDuplicates(const IDuplicateGroups& duplicates, DeletionConfig& cfg)
{
    GroupProcessor processor {cfg};
    size_t total = duplicates.numGroups();

    duplicates.enumGroups([&, i = 0ULL](const DupGroup& group) mutable {
        return processor.process(group, ++i, total);
    });
}

} // namespace tools::dups
