#include <duplicates/DuplicateDetector.h>
#include <duplicates/Utils.h>
#include <duplicates/Node.h>
#include <core/utils/StopWatch.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/Number.h>

#include <chrono>
#include <toml++/toml.hpp>

#include <iostream>
#include <fstream>

using tools::dups::DupGroup;
using tools::dups::DuplicateDetector;
using tools::dups::Node;
using tools::dups::stage2str;
using namespace std::literals;

namespace util = tools::dups::util;

namespace {

void printUsage()
{
    std::cout << R"(
Usage:
    duplicates <dir>   - Search duplicate items in the given directory.
)";
}

} // namespace

class Progress
{
public:
    using DisplayCb = std::function<void(std::ostream&)>;

    explicit Progress(std::chrono::milliseconds freq = std::chrono::milliseconds(100))
        : sw_(true)
        , freq_ {freq}
    {
    }

    void update(const DisplayCb& cb)
    {
        if (sw_.elapsed() > freq_)
        {
            cb(std::cout);
            std::cout << '\r';
            std::cout.flush();
            sw_.restart();
        }
    }

private:
    StopWatch sw_;
    std::chrono::milliseconds freq_ {};
};

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printUsage();
        return 1;
    }

    try
    {
        auto config = toml::parse_file(argv[1]);

        std::vector<std::string> excludedDirs;
        config["exclude_directories"].as_array()->for_each([&excludedDirs](const auto& value) {
            if constexpr (toml::is_string<decltype(value)>) {
                excludedDirs.emplace_back(value.value_or(""sv));
            }
        });

        std::vector<std::string> scanDirs;
        config["scan_directories"].as_array()->for_each([&scanDirs](const auto& value) {
            if constexpr (toml::is_string<decltype(value)>) {
                scanDirs.emplace_back(value.value_or(""sv));
            }
        });

        const DuplicateDetector::Options opts{
            .minSizeBytes = config["min_file_size_bytes"].value_or(0ULL),
            .maxSizeBytes = config["max_file_size_bytes"].value_or(0ULL)
        };

        std::chrono::milliseconds updateFrequency(config["update_freq_ms"].value_or(0));
        std::string allFiles = config["all_files"].value_or("");
        std::string dupFiles = config["dup_files"].value_or("");;

        DuplicateDetector detector;
        Progress progress(updateFrequency);
        StopWatch sw;

        for (const auto& scanDir: scanDirs)
        {
            fs::path srcDir = scanDir;
            srcDir = srcDir.lexically_normal();
            std::cout << "Scanning directory: " << srcDir << '\n';

            file::enumFilesRecursive(
                srcDir,
                excludedDirs,
                [numFiles = 0, &detector, &progress](const auto& p, const std::error_code& ec) mutable {
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

        std::cout << "Discovered files: " << detector.numFiles();
        std::cout << ", elapsed: " << sw.elapsedMs() << " ms" << std::endl;

        // Dump content
        std::cout << "Printing the content of the directory...\n";
        std::ofstream outf(allFiles, std::ios::out | std::ios::binary);

        util::treeDump(detector.root(), outf);
        std::cout << "Printing completed.\n";

        sw.start();
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

        std::ofstream dupf(dupFiles, std::ios::out | std::ios::binary);
        const auto grpDigits = static_cast<int>(num::digits(detector.numGroups()));
        size_t totalFiles = 0;

        detector.enumDuplicates(
            [&out = dupf, grpDigits, sizeDigits = 15, &totalFiles](const DupGroup& group) {
                if (group.entires.size() > 2)
                {
                    out << "\nLarge group (" << group.entires.size() << ")\n";
                }

                totalFiles += group.entires.size();
                for (const auto& e : group.entires)
                {
                    out << "[" << std::setw(grpDigits) << group.groupId << "] - "
                        << std::string_view(e.sha256).substr(0, 10) << ','
                        << std::setw(sizeDigits) << e.size << " " << e.dir << " -> "
                        << e.filename << "\n";
                }
                out << '\n';
            });

        std::cout << "Detected: " << detector.numGroups() << " duplicates groups\n";
        std::cout << "All groups combined have: " << totalFiles << " files\n";
        std::cout << "That means: " << totalFiles - detector.numGroups() << " duplicate files\n";
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
