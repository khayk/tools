#include <duplicates/DuplicateDeletion.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Progress.h>
#include <duplicates/Menu.h>
#include <core/utils/Number.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <algorithm>
#include <iterator>
#include <cassert>
#include <stdexcept>
#include <tuple>
#include "core/utils/Str.h"

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


template <typename Paths>
void displayPaths(const std::string_view desc, const Paths& paths, std::ostream& out)
{
    out << desc << '\n';
    size_t i = 0;
    for (const auto& path : paths.paths())
    {
        out << "    " << std::setw(2) << ++i << ". " << path << '\n';
    }

    if (paths.paths().empty())
    {
        out << "    " << "Path list is empty\n";
    }
}

void menuOption(Menu& menu, std::string_view title, char key, Action action)
{
    std::string menuLabel = std::format("[{}] {}", key, title);

    menu.add(menuLabel, Matchers::Key(key), std::move(action));
}

void menuOption(Menu& menu, size_t count, Action action)
{
    std::string menuLabel = std::format("[?] Number from 1 and {}", count);

    menu.add(menuLabel, Matchers::Range(1, static_cast<int>(count)), std::move(action));
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

void displayPathOptions(std::string& out, const PathsVec& paths)
{
    std::ostringstream oss;
    displayPathOptions(oss, paths);
    out = oss.str();
}

template <typename Paths>
Navigation addPaths(const PathsVec& files,
                    std::string_view listName,
                    Paths& paths,
                    UserIO& io)
{
    auto dirs = createDirectoriesList(files);

    if (dirs.empty())
    {
        spdlog::info("The generated list of suggested paths is empty");
        return Navigation::Continue;
    }

    std::string out;
    displayPathOptions(out, dirs);
    io.printText(out);

    Menu menu(listName);

    menuOption(menu, dirs.size(), [&](UserIO& io) {
        auto index = num::s2num<size_t>(io.currentPrompt());
        paths.add(dirs[index - 1]);
        return Navigation::Continue;
    });

    return io.run(menu);
}

template <typename Paths>
Navigation deletePaths(std::string_view listName,
                       Paths& paths,
                       UserIO& io)
{
    if (paths.empty())
    {
        spdlog::info("Path list is empty");
        return Navigation::Continue;
    }

    PathsVec dirs;
    std::string out;
    const auto& input = paths.paths();
    std::copy(input.begin(), input.end(), std::back_inserter(dirs));
    displayPathOptions(out, dirs);
    io.printText(out);

    Menu menu(listName);

    menuOption(menu, paths.size(), [&](UserIO& io) {
        auto index = num::s2num<size_t>(io.currentPrompt());
        if (index > dirs.size())
        {
            spdlog::info("Attempt to access container with size {} at index {}", dirs.size(), index - 1);
            return Navigation::Back;
        }

        spdlog::info("Removing item: {}", dirs[index - 1]);
        paths.paths().erase(dirs[index - 1]);
        return Navigation::Back;
    });

    return io.run(menu);
}

std::string concat(std::string_view lhs, std::string_view rhs)
{
    std::string str(lhs);
    str.append(rhs);
    return str;
}

template <typename Paths>
Navigation editConfig(const PathsVec& files,
                      std::string_view listName,
                      Paths& paths,
                      UserIO& io)
{
    Menu menu(std::format("Edit {}", listName));

    menuOption(menu, "Add to list", 'a', [&](UserIO& io) {
        return addPaths(files, concat("Add to ", listName), paths, io);
    });

    menuOption(menu, "Delete from list", 'd', [&](UserIO& io) {
        return deletePaths(concat("Delete from ", listName), paths, io);
    });

    return io.run(menu);
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

bool isDuplicateNamingPattern(const IDeletionStrategy& strategy, PathsVec& files)
{
    if (2 > files.size())
    {
        return false;
    }

    // find the file with a shortest name
    auto it = files.begin();
    auto shortest = it->stem();
    for (auto it2 = files.begin(); it2 != files.end(); ++it2)
    {
        auto tmp = (*it2).stem();
        if (shortest.native().size() > tmp.native().size())
        {
            shortest = tmp;
            it = it2;
        }
    }

    auto ss = shortest.native();
    std::regex r(R"((\(\d+\)|_copy|copy)$)");

    for (const auto& file : files)
    {
        auto tmp = file.stem();
        if (tmp != shortest)
        {
            auto p = tmp.native().find(ss);
            if (p != 0)
            {
                return false;
            }

            const auto& ns = tmp.native();
            std::string_view sv(ns);
            sv.remove_prefix(ss.size());

            if (sv.size() < 2)
            {
                return false;
            }

            sv = str::trim(sv);
            if (!std::regex_search(std::string(sv), r))
            {
                return false;
            }
        }
    }

    std::iter_swap(it, std::prev(files.end()));
    files.pop_back();
    deleteFiles(strategy, files);

    return true;
}

Flow deleteInteractively(PathsVec& files, DeletionConfig& cfg)
{
    // Try automatic resolution first
    if (deduceTheOneToKeep(cfg.strategy(), files, cfg.keepFromPaths()))
    {
        return Flow::Done;
    }

    // From these name.txt, name(1).txt, name(2).txt ... only name.txt will be
    // preserved
    // @todo:hayk - if pattern emerges that tells we need to run various checks based
    // on different conditions, we might use template method design pattern to delegate
    // the decision to user defined logic
    if (isDuplicateNamingPattern(cfg.strategy(), files))
    {
        return Flow::Done;
    }

    displayPathOptions(cfg.out(), files);

    Menu menu("Enter a number to keep, or select an action");

    menuOption(menu, files.size(), [&](UserIO& io) {
        // User has chosen the number to KEEP.
        auto index = num::s2num<size_t>(io.currentPrompt());

        if (index == 0 || index > files.size())
        {
            assert(index > 0 && index <= files.size() && "Out of bound access");
            throw std::logic_error("Attempt to access an array out of bounds");
        }

        std::swap(files[index - 1], files.back());
        files.pop_back();
        deleteFiles(cfg.strategy(), files);
        return Navigation::Done;
    });

    menuOption(menu, "Ignore", 'i', [&](UserIO& ) {
        spdlog::info("Ignoring group...");
        cfg.ignoredPaths().add(files);
        return Navigation::Continue;
    });

    menuOption(menu, "Open directories", 'o', [&](UserIO&) {
        openDirectories(files);
        return Navigation::Continue;
    });

    menuOption(menu, "Edit keep-from list", 'k', [&](UserIO& io) {
        return editConfig(files, "keep-from list", cfg.keepFromPaths(), io);
    });

    menuOption(menu, "Edit delete-from list", 'd', [&](UserIO& io) {
        return editConfig(files, "delete-from list", cfg.deleteFromPaths(), io);
    });

    menuOption(menu, "View keep/delete list", 'v', [&](UserIO& ) {
        displayPaths("Keep from paths:", cfg.keepFromPaths(), cfg.out());
        displayPaths("Delete from paths:", cfg.deleteFromPaths(), cfg.out());
        return Navigation::Continue;
    });

    auto navigation = cfg.io().run(menu, false);
    return navigation == Navigation::Quit ? Flow::Quit : Flow::Done;
}


DeletionConfig::DeletionConfig(const IDeletionStrategy& strategy,
                               std::ostream& out,
                               std::istream& in,
                               Progress& progress,
                               StreamIO& io)
    : strategy_ {strategy}
    , out_ {out}
    , in_ {in}
    , progress_ {progress}
    , io_ {io}
{
}

const IDeletionStrategy& DeletionConfig::strategy() const
{
    return strategy_;
}

StreamIO& DeletionConfig::io()
{
    return io_;
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
        auto flow = Flow::Retry;

        updateProgress(groupIdx, totalGroups);

        while (flow == Flow::Retry)
        {
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
                // Delete the "unwanted" ones immediately, keeping the "selective" ones
                // for review
                deleteFiles(cfg.strategy(), autoDelete);
            }

            flow = handleReview(group, selective);
        }

        return flow != Flow::Quit;
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

    Flow handleReview(const DupGroup& group, PathsVec& selective)
    {
        if (selective.size() <= 1)
        {
            return Flow::Done;
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
