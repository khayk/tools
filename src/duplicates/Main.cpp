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

#include <chrono>
#include <system_error>
#include <iostream>
#include <fstream>
#include <regex>

using tools::dups::DupGroup;
using tools::dups::DuplicateDetector;
using tools::dups::Node;
using tools::dups::Progress;
using tools::dups::stage2str;
using std::chrono::milliseconds;
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

    spdlog::trace("Dumping paths of all scanned files to: {}", allFiles);
    std::ofstream outf(allFiles, std::ios::out | std::ios::binary);

    if (!outf)
    {
        const auto s = fmt::format("Unable to open file: {}", allFiles);
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            s);
    }

    util::treeDump(detector.root(), outf);
    spdlog::info("Dumped {} files", detector.numFiles());
}


struct Config
{
    std::vector<std::string> scanDirs;
    std::vector<std::regex> exclusionPatterns;
    fs::path cacheDir;
    fs::path allFilesPath;
    fs::path dupFilesPath;
    size_t minFileSizeBytes {};
    size_t maxFileSizeBytes {};
    std::chrono::milliseconds updateFrequency {};
    bool skipDetection {false};
};

Config loadConfig(const fs::path& cfgFile)
{
    Config cfg;
    auto config = toml::parse_file(cfgFile.string());

    config["exclusion_patterns"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.exclusionPatterns.emplace_back(
                std::regex(std::string(value.value_or(""sv)),
                           std::regex_constants::ECMAScript)
            );
        }
    });

    config["scan_directories"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.scanDirs.emplace_back(value.value_or(""sv));
        }
    });

    cfg.minFileSizeBytes = config["min_file_size_bytes"].value_or(0ULL);
    cfg.maxFileSizeBytes = config["max_file_size_bytes"].value_or(0ULL);
    cfg.updateFrequency = milliseconds(config["update_freq_ms"].value_or(0));
    cfg.allFilesPath = config["all_files"].value_or("");
    cfg.dupFilesPath = config["dup_files"].value_or("");
    cfg.cacheDir = config["cache_directory"].value_or("");

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
        spdlog::info("Scanning directory: {}", srcDir);

        file::enumFilesRecursive(
            srcDir,
            cfg.exclusionPatterns,
            [numFiles = 0, &detector, &progress](const auto& p,
                                                 const std::error_code& ec) mutable {
                if (ec)
                {
                    spdlog::error("Error: '{}' while processing path '{}'", ec.message(), p);
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
                << std::string_view(e.sha256).substr(0, 16) << separator
                << e.size << separator
                << e.dir / e.filename;
            sortedLines.emplace_back(oss.str());
        }

        std::sort(sortedLines.begin(), sortedLines.end());
        for (const auto& line : sortedLines)
        {
            out << line << '\n';
        }

        out << '\n';
    });

    spdlog::info("Detected {} duplicates groups", detector.numGroups());
    spdlog::info("All groups combined have: {} files", totalFiles);
    spdlog::info("In other words: {} duplicate files", totalFiles - detector.numGroups());
}


void deleteDuplicateInteractive(const DuplicateDetector& detector)
{
    std::ignore = detector;
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
        const fs::path logsDir = dirs::config() / "duplicates";
        const fs::path logFilename = "duplicates.log";
        utl::configureLogger(logsDir, logFilename);
        trace.emplace("",
            fmt::format("{:-^80s}", "> START <"),
            fmt::format("{:-^80s}\n", "> END <"));

        DuplicateDetector detector;
        Progress progress(cfg.updateFrequency);

        scanDirectories(cfg, detector, progress);
        detectDuplicates(cfg, detector, progress);
        reportDuplicates(cfg, detector);
        deleteDuplicateInteractive(detector);

        return 0;
    }
    catch (const std::system_error& se)
    {
        spdlog::error("std::system_error: {}", se.what());
    }
    catch (const std::exception& e)
    {
        spdlog::error("std::exception: {}", e.what());
    }

    return 1;
}
