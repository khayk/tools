
#include <duplicates/DuplicateOperation.h>
#include <duplicates/DuplicateDetector.h>
#include <duplicates/Utils.h>
#include <core/utils/FmtExt.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>

namespace tools::dups {

// @todo: make scandirs in the config as vector of paths
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

    spdlog::info("Discovered files: {}", detector.numFiles());
    spdlog::trace("Scanning took: {} ms", sw.elapsedMs());
    spdlog::trace("Nodes: {}", detector.root()->nodesCount());
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
    const Options opts {.minSizeBytes = cfg.minFileSizeBytes,
                                           .maxSizeBytes = cfg.maxFileSizeBytes};

    spdlog::trace("Detecting duplicates...");
    detector.detect(opts,
                    [&progress](const Stage stage,
                                const Node*,
                                size_t percent) mutable {
                        progress.update([&](std::ostream& os) {
                            os << "Stage: " << stage2str(stage) << " - " << percent
                               << "%";
                        });
                    });

    spdlog::trace("Detection took: {} ms", sw.elapsedMs());
}

void outputFiles(const fs::path& allFiles, const DuplicateDetector& detector)
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

    util::outputTree(detector.root(), outf);
    spdlog::info("Dumped {} files", detector.numFiles());
}

void reportDuplicates(const fs::path& reportPath, const DuplicateDetector& detector)
{
    std::ofstream out(reportPath, std::ios::out | std::ios::binary);
    size_t totalFiles = 0;
    size_t largestFileSize = 0;

    // Precalculations to produce nice output
    detector.enumGroups([&largestFileSize, &totalFiles](const DupGroup& group) {
        totalFiles += group.entires.size();
        largestFileSize = std::max(largestFileSize, group.entires.front().size);
        return true;
    });

    const auto separator = '|';
    std::ostringstream oss;
    std::vector<std::string> sortedLines;

    detector.enumGroups([&out, &oss, &sortedLines](const DupGroup& group) {
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

} // namespace tools::dups
