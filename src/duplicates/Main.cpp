#include <duplicates/DuplicateDetector.h>
#include <duplicates/DeletionStrategy.h>
#include <duplicates/Utils.h>
#include <duplicates/Node.h>
#include <duplicates/Config.h>

#include <core/utils/StopWatch.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/Number.h>
#include <core/utils/Sys.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Log.h>
#include <core/utils/Tracer.h>

#include <filesystem>
#include <fmt/format.h>

#include <chrono>
#include <system_error>
#include <algorithm>
#include <iostream>
#include <fstream>

using tools::dups::BackupAndDelete;
using tools::dups::DeletionStrategy;
using tools::dups::DupGroup;
using tools::dups::DuplicateDetector;
using tools::dups::Node;
using tools::dups::Progress;
using tools::dups::stage2str;
using tools::dups::loadConfig;
using tools::dups::logConfig;

namespace util = tools::dups::util;

namespace {

void printUsage()
{
    std::cout << R"(
Usage:
    @ todo: revise this
    duplicates <cfg_file> - Scan directories and detect duplicates based on the
                            configuration file.
    duplicates <dir>   - Search duplicate items in the given directory.
)";
}

void experimenting(const fs::path& file)
{
    DuplicateDetector detector;
    StopWatch sw;

    file::readLines(file, [&detector](const std::string& line) {
        std::string_view sv = line;
        if (line.size() >= 2)
        {
            sv.remove_prefix(1);
            sv.remove_suffix(1);
            detector.addFile(sv);
        }
        return true;
    });

    util::dumpContent("tmp.txt", detector);

    std::cout << "Memory: " << str::humanizeBytes(sys::currentProcessMemoryUsage())
              << '\n';
    std::cout << "sizeof(path): " << sizeof(fs::path) << '\n';
    std::cout << "Files: " << detector.numFiles() << '\n';
    std::cout << "Nodes: " << detector.root()->nodesCount() << '\n';
    // std::cout << "MapSize: " << detector1.mapSize() << '\n';
    std::cout << "Elapsed: " << sw.elapsedMs() << " ms" << '\n';
    std::cout << sizeof(Node) << '\n';
    std::cout << "node: " << sizeof(Node) << '\n';
    std::cout << "wstring_view: " << sizeof(std::wstring_view) << '\n';
    std::cout << "string: " << sizeof(std::string) << '\n';
    std::cout << "parent_: " << sizeof(Node*) << '\n';
}

using tools::dups::Config;

void detectDuplicates(const Config& cfg,
                      DuplicateDetector& detector,
                      Progress& progress)
{
    if (cfg.skipDetection)
    {
        spdlog::warn("Skip duplicate detection");
        return;
    }

    StopWatch sw;
    const DuplicateDetector::Options opts {.minSizeBytes = cfg.minFileSizeBytes,
                                           .maxSizeBytes = cfg.maxFileSizeBytes};

    spdlog::trace("Detecting duplicates...");
    detector.detect(opts,
                    [&progress](const DuplicateDetector::Stage stage,
                                const Node*,
                                size_t percent) mutable {
                        progress.update([&](std::ostream& os) {
                            os << "Stage: " << stage2str(stage) << " - " << percent
                               << "%";
                        });
                    });

    spdlog::trace("Detection took: {} ms", sw.elapsedMs());
}


void reportDuplicates(const Config& cfg, const DuplicateDetector& detector)
{
    std::ofstream out(cfg.dupFilesPath, std::ios::out | std::ios::binary);
    size_t totalFiles = 0;
    size_t largestFileSize = 0;

    // Precalculations to produce nice output
    detector.enumDuplicates([&largestFileSize, &totalFiles](const DupGroup& group) {
        totalFiles += group.entires.size();
        largestFileSize = std::max(largestFileSize, group.entires.front().size);
        return true;
    });

    const auto separator = '|';
    std::ostringstream oss;
    std::vector<std::string> sortedLines;

    detector.enumDuplicates([&out, &oss, &sortedLines](const DupGroup& group) {
        sortedLines.clear();

        for (const auto& e : group.entires)
        {
            oss.str("");
            oss << group.groupId << separator
                << std::string_view(e.sha256).substr(0, 16) << separator << e.size
                << separator << file::path2s(e.file);
            sortedLines.emplace_back(oss.str());
        }

        std::ranges::sort(sortedLines);
        for (const auto& line : sortedLines)
        {
            out << line << '\n';
        }

        out << '\n';
        return true;
    });

    spdlog::info("Detected {} duplicates groups", detector.numGroups());
    spdlog::info("All groups combined have: {} files", totalFiles);
    spdlog::info("In other words: {} duplicate files",
                 totalFiles - detector.numGroups());
}

using PathsVec = std::vector<fs::path>;
using PathsSet = std::unordered_set<fs::path>;

bool isSafeToDelete(const std::vector<std::string>& delDirs, const fs::path& path)
{
    const auto pathStr = path.string();

    return std::ranges::any_of(delDirs, [&pathStr](const auto& deleteDir) {
        return pathStr.find(deleteDir, 0) != std::string::npos;
    });
};

void deleteFiles(DeletionStrategy& strategy, const PathsVec& files)
{
    for (const auto& file : files)
    {
        try
        {
            strategy.apply(file);
        }
        catch (const std::exception& e)
        {
            spdlog::error("Error deleting file: {} - {}", e.what(), file);
        }
    }
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

bool deleteInteractively(DeletionStrategy& strategy,
                         PathsVec& files,
                         PathsSet& ignoredFiles)
{
    // Display files to be deleted
    size_t i = 0;
    for (const auto& file : files)
    {
        const auto width = static_cast<int>(num::digits(files.size()));
        std::cout << std::setw(width + 1) << ++i << ": " << file << '\n';
    }

    static std::string lastInput;
    std::string input;

    while (input.empty())
    {
        std::cout << "Enter number to KEEP (q - quit, i - ignore) > ";
        std::getline(std::cin, input);

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
        std::cout << "Invalid choice, no file is deleted.\n";
    }

    return true;
};

class DryRunDelete : public DeletionStrategy
{
public:
    void apply(const fs::path& file) override
    {
        spdlog::info("Would delete: {}", file);
    }
};

void deleteDuplicates(const Config& cfg, const DuplicateDetector& detector)
{
    PathsSet ignoredFiles;
    PathsVec deleteWithoutAsking;
    PathsVec deleteSelectively;
    BackupAndDelete strategy(cfg.cacheDir);
    // DryRunDelete strategy;

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
            std::cout << "file size: " << group.entires.front().size
                      << ", sha256: " << group.entires.front().sha256 << '\n';
            resumeEnumeration =
                deleteInteractively(strategy, deleteSelectively, ignoredFiles);
        }

        // We left with one file, which is now unique in the system
        return resumeEnumeration;
    });

    saveIgnoredFiles(cfg.ignFilesPath, ignoredFiles);
}

} // namespace

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printUsage();
        return 2;
    }

    std::optional<ScopedTrace> trace;

    try
    {
        const fs::path cfgFile(argv[1]);

        if (cfgFile.extension() != ".toml")
        {
            experimenting(cfgFile);
            return 0;
        }

        const Config cfg = loadConfig(cfgFile);
        utl::configureLogger(cfg.logDir, cfg.logFilename);
        trace.emplace("",
                      fmt::format("{:-^80s}", "> START <"),
                      fmt::format("{:-^80s}\n", "> END <"));
        logConfig(cfgFile, cfg);

        DuplicateDetector detector;
        Progress progress(cfg.updateFrequency);

        util::scanDirectories(cfg, detector, progress);
        util::dumpContent(cfg.allFilesPath, detector);

        detectDuplicates(cfg, detector, progress);
        reportDuplicates(cfg, detector);
        deleteDuplicates(cfg, detector);
    }
    catch (const std::system_error& se)
    {
        spdlog::error("std::system_error: {}", se.what());
        return 1;
    }
    catch (const std::exception& e)
    {
        spdlog::error("std::exception: {}", e.what());
        return 1;
    }

    return 0;
}
