#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <duplicates/DeletionStrategy.h>
#include <core/utils/File.h>
#include <core/utils/Crypto.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <array>

namespace tools::dups {
namespace {

class SilenceLogger
{
    spdlog::level::level_enum prevLevel_ {spdlog::get_level()};

public:
    SilenceLogger()
    {
        spdlog::set_level(spdlog::level::level_enum::off);
    }

    ~SilenceLogger()
    {
        spdlog::set_level(prevLevel_);
    }
};

} // namespace

TEST(DeletionStrategyTest, DeletePermanently)
{
    SilenceLogger silenceLogger;
    file::TempDir data("dups");
    DeletePermanently strategy;

    const fs::path file = data.path() / "test.txt";
    file::write(file, "test content");

    EXPECT_TRUE(fs::exists(file));
    strategy.apply(file);
    EXPECT_FALSE(fs::exists(file));
}

TEST(DeletionStrategyTest, BackupAndDelete)
{
    SilenceLogger silenceLogger;
    file::TempDir data("dups");
    const auto backupDir = data.path() / "backup";
    auto strategy = std::make_unique<BackupAndDelete>(backupDir);
    const auto journalFile = strategy->journalFile();

    // create journal file when there is something to write to it
    EXPECT_FALSE(fs::exists(backupDir / journalFile.filename()));

    const std::array files {
        data.path() / "test1.txt",
        data.path() / "test2.txt",
        data.path() / "test3.txt",
    };
    constexpr auto sampleContent = "test content";

    for (const auto& file : files)
    {
        file::write(file, sampleContent);
        EXPECT_TRUE(fs::exists(file));
        strategy->apply(file);
        EXPECT_FALSE(fs::exists(file));

        const auto parentPath = files.front().parent_path();
        const auto hash = crypto::md5(parentPath.string());

        EXPECT_TRUE(fs::exists(backupDir / journalFile.filename()));
        EXPECT_TRUE(fs::exists(backupDir / hash / file.filename()));
    }

    EXPECT_TRUE(journalFile.filename().string().starts_with("deleted_files_"));
    strategy.reset(); // This will force the journal file to be closed

    file::readLines(
        journalFile,
        [it = files.begin()](const std::string& line) mutable {
            EXPECT_FALSE(line.empty());
            EXPECT_TRUE(line.contains(file::path2s(it->filename()).c_str()));
            ++it;
            return true;
        });
}

} // namespace tools::dups