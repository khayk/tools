#include <duplicates/Utils.h>
#include <duplicates/Node.h>
#include <duplicates/DuplicateDetector.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/FmtExt.h>

#include <spdlog/spdlog.h>
#include <ostream>
#include <fstream>

namespace tools::dups::util {

uint16_t digits(size_t n)
{
    uint16_t d = 0;

    while (n != 0)
    {
        d++;
        n /= 10;
    }

    return d;
}

void treeDump(const Node* root, std::ostream& os)
{
    root->enumNodes([&os](const Node* node) {
        if (node->depth() == 0)
        {
            return;
        }

        os << std::string(1UL * (node->depth() - 1), ' ');
        os << file::path2s(node->name()) << (node->leaf() || node->depth() == 1 ? "" : "/") << '\n';
    });
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

} // namespace tools::dups::util
