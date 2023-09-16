#include "StopWatch.h"
#include "DuplicateDetector.h"
#include "Utils.h"

#include <iostream>
#include <codecvt>
#include <locale>
#include <string_view>

void printUsage()
{
    std::cout << R"(
Usage:
    duplicates <dir>   - Search duplicate items in the given directory.
)";
}

void dumpToFile(const DuplicateDetector& detector, std::string_view filePath)
{
    std::wofstream pathsFile(std::string(filePath), std::ios::out | std::ios::binary);

    if (!pathsFile)
    {
        std::cerr << "Failed to open file: " << filePath << std::endl;
    }

    const unsigned long MaxCode = 0x10ffff;
    const std::codecvt_mode Mode = std::generate_header;
    std::locale utf16_locale(pathsFile.getloc(), new std::codecvt_utf16<wchar_t, MaxCode, Mode>);
    pathsFile.imbue(utf16_locale);

    detector.enumFiles([&pathsFile](const std::wstring& ws) {
        pathsFile << ws << '\n';
        });
}

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
        std::cout << "Scanning directory: " << srcDir.u8string() << '\n';

        DuplicateDetector detector;
        StopWatch sw(true);

        for (fs::recursive_directory_iterator i(srcDir), end; i != end; ++i)
        {
            if (!fs::is_directory(i->path()))
            {
                const auto& p = i->path();
                detector.add(p);
            }
        }

        std::cout << "Discovered files: " << detector.files();
        std::cout << ", elapsed: " << sw.elapsedMls() << " ms" << std::endl;

        // Dump content
        std::cout << "\nPrinting the content of the directory...\n\n";
        detector.enumFiles([](const std::wstring& ws) {
            std::cout << ws2s(ws) << '\n';
            });
        std::cout << "\nPrinting completed.\n";

        sw.start();
        std::cout << "\nDetecting duplicates...:\n";
        detector.detect(Options {});
        std::cout << "Detection took: " << sw.elapsedMls() << " ms" << std::endl;

        detector.treeDump(std::cout);

        detector.enumDuplicates([](const DupGroup& group) {
            if (group.entires.size() > 2)
            {
                std::cout << "\nLarge group (" << group.entires.size() << ")\n";
            }

            for (const auto& e : group.entires)
            {
                std::cout
                    << group.groupId << ','
                    << ws2s(e.dir) << ','
                    << ws2s(e.filename) << ','
                    << std::string_view(e.sha).substr(0, 10) << ','
                    << e.size << "\n";
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
