#include <duplicates/DuplicateDetector.h>
#include <duplicates/Utils.h>
#include <duplicates/Node.h>
#include <core/utils/StopWatch.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>
#include <core/utils/Number.h>

#include <iostream>
#include <fstream>

using tools::dups::DupGroup;
using tools::dups::DuplicateDetector;
using tools::dups::Node;
using tools::dups::stage2str;

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

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printUsage();
        return 1;
    }

    try
    {
        fs::path srcDir(argv[1]);
        srcDir = srcDir.lexically_normal();
        std::cout << "Scanning directory: " << srcDir << '\n';

        DuplicateDetector detector;
        StopWatch sw(true);

        std::vector<std::string> excludedDirs {".git"};
        file::enumFilesRecursive(
            srcDir,
            excludedDirs,
            [&detector](const auto& p, const std::error_code& ec) {
                if (ec)
                {
                    std::cerr << "Error while processing path: " << p << std::endl;
                    return;
                }

                if (fs::is_regular_file(p))
                {
                    detector.addFile(p);
                }
            });

        std::cout << "Discovered files: " << detector.numFiles();
        std::cout << ", elapsed: " << sw.elapsedMs() << " ms" << std::endl;

        // Dump content
        std::cout << "\nPrinting the content of the directory...\n\n";
        std::ofstream outf("files.txt", std::ios::out | std::ios::binary);

        util::treeDump(detector.root(), outf);
        std::cout << "\nPrinting completed.\n";

        sw.start();
        std::cout << "\nDetecting duplicates...:\n";

        detector.detect(DuplicateDetector::Options {},
                        [lastUpdate = 0LL, &sw](const DuplicateDetector::Stage stage,
                                                const Node*,
                                                size_t percent) mutable {
                            if (sw.elapsedMs() - lastUpdate > 100)
                            {
                                std::cout << "Stage: " << stage2str(stage) << ", "
                                          << percent << "%" << '\r';
                                lastUpdate = sw.elapsedMs();
                            }
                        });

        std::cout << "Detection took: " << sw.elapsedMs() << " ms" << std::endl;
        std::ofstream dupf("dups.txt", std::ios::out | std::ios::binary);
        const auto grpDigits = static_cast<int>(num::digits(detector.numGroups()));

        detector.enumDuplicates(
            [&out = dupf, grpDigits, sizeDigits = 15](const DupGroup& group) {
                if (group.entires.size() > 2)
                {
                    out << "\nLarge group (" << group.entires.size() << ")\n";
                }

                for (const auto& e : group.entires)
                {
                    out << "[" << std::setw(grpDigits) << group.groupId << "] - "
                        << std::string_view(e.sha256).substr(0, 10) << ','
                        << std::setw(sizeDigits) << e.size << " " << e.dir << " -> "
                        << e.filename << "\n";
                }
            });
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
