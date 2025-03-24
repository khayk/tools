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

#include <chrono>
#include <system_error>
#include <toml++/toml.hpp>
#include <fmt/format.h>
#include <iostream>
#include <fstream>

using tools::dups::DupGroup;
using tools::dups::DuplicateDetector;
using tools::dups::Node;
using tools::dups::Progress;
using tools::dups::stage2str;
using namespace std::literals;
using std::chrono::milliseconds;

namespace util = tools::dups::util;

namespace {

void printUsage()
{
    std::cout << R"(
Usage:
    duplicates <dir>   - Search duplicate items in the given directory.
)";
}

void experimenting(const fs::path& file)
{
    DuplicateDetector detector;
    StopWatch sw;

    // "/home/khayk/Code/repo/github/khayk/tools/home.txt"
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
        std::cout << "Skip dumping paths of all scanned files\n";
        return;
    }

    std::cout << "Printing the content of the directory...\n";
    std::ofstream outf(allFiles, std::ios::out | std::ios::binary);

    if (!outf)
    {
        const auto s = fmt::format("Unable to open file: {}", allFiles);
        throw std::system_error(
            std::make_error_code(std::errc::no_such_file_or_directory),
            s);
    }

    util::treeDump(detector.root(), outf);
    std::cout << "Printing completed.\n";
}


struct Config
{
    std::vector<std::string> scanDirs;
    std::vector<std::string> excludedDirs;
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

    config["exclude_directories"].as_array()->for_each([&cfg](const auto& value) {
        if constexpr (toml::is_string<decltype(value)>)
        {
            cfg.excludedDirs.emplace_back(value.value_or(""sv));
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
        std::cout << "Scanning directory: " << srcDir << '\n';

        file::enumFilesRecursive(
            srcDir,
            cfg.excludedDirs,
            [numFiles = 0, &detector, &progress](const auto& p,
                                                 const std::error_code& ec) mutable {
                if (ec)
                {
                    std::cerr << "Error while processing path: " << p << std::endl;
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

    std::cout << std::string(80, ' ') << '\r';
    std::cout << "Discovered files: " << detector.numFiles() << std::endl;
    std::cout << "Elapsed: " << sw.elapsedMs() << " ms" << std::endl;
    std::cout << "Nodes: " << detector.root()->nodesCount() << std::endl;

    // Dump content
    dumpContent(cfg.allFilesPath, detector);
}

void detectDuplicates(const Config& cfg,
                      DuplicateDetector& detector,
                      Progress& progress)
{
    if (cfg.skipDetection)
    {
        std::cout << "Skipping duplicate detection\n";
        return;
    }

    StopWatch sw;
    const DuplicateDetector::Options opts {.minSizeBytes = cfg.minFileSizeBytes,
                                           .maxSizeBytes = cfg.maxFileSizeBytes};

    std::cout << "Detecting duplicates...\n";

    detector.detect(opts,
                    [&progress](const DuplicateDetector::Stage stage,
                                const Node*,
                                size_t percent) mutable {
                        progress.update([&](std::ostream& os) {
                            os << "Stage: " << stage2str(stage) << " - " << percent
                               << "%";
                        });
                    });

    std::cout << "Detection took: " << sw.elapsedMs() << " ms" << std::endl;
}

void reportDuplicates(const Config& cfg, DuplicateDetector& detector)
{
    std::ofstream dupf(cfg.dupFilesPath, std::ios::out | std::ios::binary);
    size_t totalFiles = 0;
    size_t largestFileSize = 0;

    // Precalculations to produce nice output
    detector.enumDuplicates([&largestFileSize, &totalFiles](const DupGroup& group) {
        totalFiles += group.entires.size();
        largestFileSize = std::max(largestFileSize, group.entires.front().size);
    });

    const auto grpDigits = static_cast<int>(num::digits(detector.numGroups()));
    const auto sizeDigits = static_cast<int>(num::digits(largestFileSize));

    detector.enumDuplicates(
        [&out = dupf, grpDigits, sizeDigits](const DupGroup& group) {
            if (group.entires.size() > 2)
            {
                out << "Large group (" << group.entires.size() << ")\n";
            }

            for (const auto& e : group.entires)
            {
                out << "[" << std::setw(grpDigits) << group.groupId << "] - "
                    << std::string_view(e.sha256).substr(0, 10) << ','
                    << std::setw(sizeDigits + 1) << e.size << " " << e.dir << " -> "
                    << e.filename << "\n";
            }
            out << '\n';
        });

    std::cout << "Detected: " << detector.numGroups() << " duplicates groups\n";
    std::cout << "All groups combined have: " << totalFiles << " files\n";
    std::cout << "That means: " << totalFiles - detector.numGroups()
              << " duplicate files\n";
}

} // namespace

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printUsage();
        return 1;
    }

    try
    {
        const fs::path cfgFile(argv[1]);

        if (cfgFile.extension() != ".toml")
        {
            experimenting(cfgFile);
            return 0;
        }

        const Config cfg = loadConfig(cfgFile);
        DuplicateDetector detector;
        Progress progress(cfg.updateFrequency);

        scanDirectories(cfg, detector, progress);
        detectDuplicates(cfg, detector, progress);
        reportDuplicates(cfg, detector);
    }
    catch (const std::system_error& se)
    {
        std::cerr << "std::system_error: " << se.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "std::exception: " << e.what() << std::endl;
    }

    return 0;
}
