#include <duplicates/DuplicateDetector.h>
#include <core/utils/StopWatch.h>
#include <core/utils/Str.h>
#include <core/utils/File.h>

#include <iostream>
#include <codecvt>
#include <locale>
#include <string_view>
#include <fstream>


using namespace tools::dups;

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

        detector.enumFiles([&outf](const fs::path& p) {
            outf << p << '\n';
        });
        std::cout << "\nPrinting completed.\n";

        sw.start();
        std::cout << "\nDetecting duplicates...:\n";
        detector.detect(DuplicateDetector::Options {});
        std::cout << "Detection took: " << sw.elapsedMs() << " ms" << std::endl;

        //detector.treeDump(std::cout);

        detector.enumDuplicates([](const DupGroup& group) {
            if (group.entires.size() > 2)
            {
                std::cout << "\nLarge group (" << group.entires.size() << ")\n";
            }

            for (const auto& e : group.entires)
            {
                std::cout << "[" << std::setw(3) << group.groupId << "] - "
                          << std::string_view(e.sha256).substr(0, 10) << ','
                          << std::setw(15) << e.size << " "
                          << e.dir
                          << " -> "
                          << e.filename
                          << "\n";
            }

            //<< "size: " << i->size()
            //<< ", sha256: " << std::string_view(i->sha256()).substr(0, 10)
            //<< ", path: " << ws2s(ws) << std::endl;
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
