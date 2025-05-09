#include <duplicates/DuplicateDetector.h>
#include <duplicates/Utils.h>
#include <duplicates/Node.h>
#include <duplicates/Progress.h>
#include <core/utils/StopWatch.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/Number.h>
#include <core/utils/Sys.h>
#include <core/utils/FmtExt.h>
#include <core/utils/Log.h>
#include <core/utils/Dirs.h>
#include <core/utils/Tracer.h>

#include <toml++/toml.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <system_error>
#include <iostream>
#include <fstream>
#include <regex>

using std::chrono::milliseconds;
using tools::dups::DupGroup;
using tools::dups::DuplicateDetector;
using tools::dups::Node;
using tools::dups::Progress;
using tools::dups::stage2str;
using namespace std::literals;
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

    std::cout << "Memory: " << str::humanizeBytes(sys::currentProcessMemoryUsage())
              << std::endl;
    std::cout << "sizeof(path): " << sizeof(fs::path) << std::endl;
    std::cout << "Files: " << detector.numFiles() << std::endl;
    std::cout << "Nodes: " << detector.root()->nodesCount() << std::endl;
    // std::cout << "MapSize: " << detector1.mapSize() << std::endl;
    std::cout << "Elapsed: " << sw.elapsedMs() << " ms" << std::endl;
    std::cout << sizeof(Node) << std::endl;
    std::cout << "node: " << sizeof(Node) << std::endl;
    std::cout << "wstring_view: " << sizeof(std::wstring_view) << std::endl;
    std::cout << "string: " << sizeof(std::string) << std::endl;
    std::cout << "parent_: " << sizeof(Node*) << std::endl;
}

void dumpContent(const fs::path& allFiles, const DuplicateDetector& detector)
{
    if (allFiles.empty())
    {
        // File to dump paths of all scanned files
        spdlog::warn("Skip dumping paths of all scanned files");
        return;
    }

    spdlog::trace("Dumping paths of all scanned files to: '{}'", allFiles);
    std::ofstream outf(allFiles, std::ios::out | std::ios::binary);

    if (!outf)
    {
        const auto s = fmt::format("Unable to open file: '{}'", allFiles);
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            s);
    }

    util::treeDump(detector.root(), outf);
    spdlog::info("Dumped {} files", detector.numFiles());
}

std::string concat(const std::vector<std::string>& vec, const std::string& sep)
{
    std::ostringstream oss;
    for (const auto& item : vec)
    {
        oss << item << sep;
    }

    auto res = oss.str();
    if (!res.empty() && !sep.empty())
    {
        res.erase(res.size() - sep.size());
    }

    return res;
}

struct Config
{
    std::vector<std::string> scanDirs;
    std::vector<std::string> safeToDeleteDirs;
    std::vector<std::regex> exclusionPatterns;
    fs::path cacheDir;
    fs::path allFilesPath;
    fs::path dupFilesPath;
    fs::path ignFilesPath;
    fs::path logDir;
    fs::path logFilename;
    size_t minFileSizeBytes {};
    size_t maxFileSizeBytes {};
    std::chrono::milliseconds updateFrequency {};
    bool skipDetection {false};
};

void dumpConfig(const fs::path& cfgFile, const Config& cfg)
{
    constexpr std::string_view pattern = "{:<27}: '{}'";
    spdlog::trace(pattern, "Configuration file", cfgFile);
    spdlog::trace(pattern, "Log file", cfg.logDir / cfg.logFilename);
    spdlog::trace(pattern, "All files path", cfg.allFilesPath);
    spdlog::trace(pattern, "Duplicate files path", cfg.dupFilesPath);
    spdlog::trace(pattern, "Ignored files path", cfg.ignFilesPath);
    spdlog::trace(pattern, "Scan directories", concat(cfg.scanDirs, ", "));
    spdlog::trace(pattern, "Safe to delete directories", concat(cfg.safeToDeleteDirs, ", "));
    spdlog::trace(pattern, "Cache directory", cfg.cacheDir);
    // spdlog::trace(pattern, "Exclusion patterns", concat(cfg.exclusionPatterns, ", "));
}

Config loadConfig(const fs::path& cfgFile)
{
    Config cfg;
    auto config = toml::parse_file(cfgFile.string());

    config["exclusion_patterns"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.exclusionPatterns.emplace_back(
                std::regex(std::string(value.value_or(""sv)),
                           std::regex_constants::ECMAScript));
        }
    });

    config["scan_directories"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.scanDirs.emplace_back(value.value_or(""sv));
        }
    });

    config["safe_to_delete_paths"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.safeToDeleteDirs.emplace_back(value.value_or(""sv));
        }
    });

    const fs::path dataDir = dirs::config() / "duplicates";

    cfg.minFileSizeBytes = config["min_file_size_bytes"].value_or(0ULL);
    cfg.maxFileSizeBytes = config["max_file_size_bytes"].value_or(0ULL);
    cfg.updateFrequency = milliseconds(config["update_freq_ms"].value_or(0));
    cfg.cacheDir = config["cache_directory"].value_or("");
    cfg.allFilesPath = config["all_files"].value_or("");
    cfg.dupFilesPath = config["dup_files"].value_or("");
    cfg.ignFilesPath = config["ign_files"].value_or("");
    cfg.logDir = dataDir / "logs";
    cfg.logFilename = "duplicates.log";

    const auto adjustPath = [&](fs::path& path) {
        if (!path.empty())
        {
            path = dataDir / path;
        }
    };

    // Ensure that auxilary files are located under the application data directory
    adjustPath(cfg.allFilesPath);
    adjustPath(cfg.dupFilesPath);
    adjustPath(cfg.ignFilesPath);

    return cfg;
}

void scanDirectories(const Config& cfg,
                     DuplicateDetector& detector,
                     Progress& progress)
{
    StopWatch sw;

    for (const auto& scanDir : cfg.scanDirs)
    {
        const auto srcDir = fs::path(scanDir).lexically_normal();
        spdlog::info("Scanning directory: '{}'", srcDir);

        file::enumFilesRecursive(
            srcDir,
            cfg.exclusionPatterns,
            [numFiles = 0, &detector, &progress](const auto& p,
                                                 const std::error_code& ec) mutable {
                if (ec)
                {
                    spdlog::error("Error: '{}' while processing path: '{}'",
                                  ec.message(),
                                  p);
                    return;
                }

                if (fs::is_regular_file(p))
                {
                    detector.addFile(p);
                    ++numFiles;
                    progress.update([&numFiles](std::ostream& os) {
                        os << "Scanned files: " << numFiles;
                    });
                }
            });
    }

    // std::cout << std::string(80, ' ') << '\r';
    spdlog::info("Discovered files: {}", detector.numFiles());
    spdlog::trace("Elapsed: {} ms", sw.elapsedMs());
    spdlog::trace("Nodes: {}", detector.root()->nodesCount());

    // Dump content
    dumpContent(cfg.allFilesPath, detector);
}


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
                << separator << e.dir / e.filename;
            sortedLines.emplace_back(oss.str());
        }

        std::sort(sortedLines.begin(), sortedLines.end());
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


void deleteDuplicates(const Config& cfg, const DuplicateDetector& detector)
{
    using PathsVec = std::vector<fs::path>;
    using PathsSet = std::unordered_set<fs::path>;

    auto isSafeToDelete = [&delDirs = cfg.safeToDeleteDirs](const fs::path& path) {
        const auto pathStr = path.string();
        for (const auto& deleteDir : delDirs)
        {
            if (pathStr.find(deleteDir, 0) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    };

    auto deleteFiles = [&](const PathsVec& files) {
        for (const auto& file : files)
        {
            try
            {
                // fs::remove(file);
                spdlog::info("Deleted: {}", file);
            }
            catch (const std::exception& e)
            {
                spdlog::error("Error deleting file: {} - {}", e.what(), file);
            }
        }
    };

    auto saveIgnoredFiles = [&](const PathsSet& files) {
        if (files.empty())
        {
            return;
        }

        std::ofstream out(cfg.ignFilesPath, std::ios::out | std::ios::binary);
        for (const auto& file : files)
        {
            out << file << '\n';
        }
    };

    PathsSet ignoredFiles;
    if (fs::exists(cfg.ignFilesPath))
    {
        spdlog::info("Loading ignored files from: {}", cfg.ignFilesPath);
        file::readLines(cfg.ignFilesPath, [&](const std::string& line) {
            ignoredFiles.emplace(line);
            return true;
        });
    }

    bool resumeEnumeration = true;
    auto deleteInteractively = [&](PathsVec& files) {
        // Display files to be deleted
        size_t i = 0;
        for (const auto& file : files)
        {
            const auto width = static_cast<int>(num::digits(files.size()));
            std::cout << std::setw(width + 1) << ++i << ": " << file << '\n';
        }

        std::string input;
        while (input.empty())
        {
            std::cout << "Enter number te be KEEP (q - quit, i - ignore): ";
            std::cin >> input;
        }

        if (tolower(input[0]) == 'q')
        {
            spdlog::info("User requested to stop deletion...");
            resumeEnumeration = false;
            return;
        }
        else if (tolower(input[0]) == 'i')
        {
            spdlog::info("User requested to ignore this group...");
            std::copy(files.begin(), files.end(),
                      std::inserter(ignoredFiles, ignoredFiles.end()));
            return;
        }

        const auto choice = num::s2num<size_t>(input);
        if (choice > 0 && choice <= files.size())
        {
            std::swap(files[choice - 1], files.back());
            files.pop_back();
            deleteFiles(files);
        }
        else
        {
            std::cout << "Invalid choice, no file is deleted.\n";
        }
    };

    PathsVec deleteWithoutAsking;
    PathsVec deleteSelectively;

    detector.enumDuplicates([&](const DupGroup& group) {
        deleteWithoutAsking.clear();
        deleteSelectively.clear();

        // Categorize files into 2 groups
        for (const auto& e : group.entires)
        {
            const auto file = e.dir / e.filename;

            if (ignoredFiles.contains(file))
            {
                spdlog::warn("File is ignored: {}", file);
                return true;
            }

            if (isSafeToDelete(e.dir))
            {
                deleteWithoutAsking.emplace_back(file);
            }
            else
            {
                deleteSelectively.emplace_back(file);
            }
        }

        // Not all files fall under the safe to delete category
        if (!deleteSelectively.empty())
        {
            // So after deleting the files below, we know that at least one
            // file will be left in the system
            deleteFiles(deleteWithoutAsking);
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
            deleteInteractively(deleteSelectively);
        }

        // We left with one file, which is now unique in the system
        return resumeEnumeration;
    });

    saveIgnoredFiles(ignoredFiles);
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
        dumpConfig(cfgFile, cfg);

        DuplicateDetector detector;
        Progress progress(cfg.updateFrequency);

        scanDirectories(cfg, detector, progress);
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
