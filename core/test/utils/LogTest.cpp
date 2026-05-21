#include <gtest/gtest.h>
#include <core/utils/File.h>
#include <core/utils/Log.h>
#include <core/utils/LogCapture.h>

#include <spdlog/spdlog.h>

#include <filesystem>
#include <regex>
#include <string>

namespace fs = std::filesystem;
using namespace core::utl;
using core::file::TempDir;

namespace {

// ---------------------------------------------------------------------------
// makeLogFilename
// ---------------------------------------------------------------------------

TEST(LogTests, MakeLogFilenameContainsAppName)
{
    const auto path = makeLogFilename("myapp");
    EXPECT_TRUE(path.string().starts_with("myapp_"));
}

TEST(LogTests, MakeLogFilenameHasLogExtension)
{
    const auto path = makeLogFilename("myapp");
    EXPECT_EQ(path.extension(), ".log");
}

TEST(LogTests, MakeLogFilenameHasTimestampFormat)
{
    const auto name = makeLogFilename("app").string();
    // Expected pattern: app_YYYYMMDD_HHMMSS.log
    const std::regex pattern(R"(app_\d{8}_\d{6}\.log)");
    EXPECT_TRUE(std::regex_match(name, pattern));
}

TEST(LogTests, MakeLogFilenameUsesProvidedAppName)
{
    const auto a = makeLogFilename("alpha").string();
    const auto b = makeLogFilename("beta").string();
    EXPECT_TRUE(a.starts_with("alpha_"));
    EXPECT_TRUE(b.starts_with("beta_"));
}

// ---------------------------------------------------------------------------
// logBuildInfo
// ---------------------------------------------------------------------------

TEST(LogTests, LogBuildInfoEmitsVersionLine)
{
    LogCaptureSt cap;
    logBuildInfo("1.2.3", "abc123", "2024-01-01");
    EXPECT_TRUE(cap.contains("1.2.3"));
}

TEST(LogTests, LogBuildInfoEmitsCommitSha)
{
    LogCaptureSt cap;
    logBuildInfo("1.0.0", "deadbeef", "2024-06-01");
    EXPECT_TRUE(cap.contains("deadbeef"));
}

TEST(LogTests, LogBuildInfoEmitsBuildTime)
{
    LogCaptureSt cap;
    logBuildInfo("1.0.0", "sha", "build-time-string");
    EXPECT_TRUE(cap.contains("build-time-string"));
}

TEST(LogTests, LogBuildInfoEmitsThreeMessages)
{
    LogCaptureSt cap;
    logBuildInfo("v", "s", "t");
    EXPECT_EQ(cap.count(), 3U);
}

TEST(LogTests, LogBuildInfoMessagesAreInfoLevel)
{
    LogCaptureSt cap;
    logBuildInfo("v", "sha", "time");
    for (const auto& entry : cap.messages())
    {
        EXPECT_EQ(entry.level, spdlog::level::info);
    }
}

// ---------------------------------------------------------------------------
// MuteLogger
// ---------------------------------------------------------------------------

TEST(LogTests, MuteLoggerSilencesOutput)
{
    LogCaptureSt cap;
    {
        MuteLogger mute;
        spdlog::info("this should be suppressed");
    }
    EXPECT_EQ(cap.count(), 0U);
}

TEST(LogTests, MuteLoggerRestoresLevelOnDestruction)
{
    const auto before = spdlog::get_level();
    {
        MuteLogger mute;
        EXPECT_EQ(spdlog::get_level(), spdlog::level::off);
    }
    EXPECT_EQ(spdlog::get_level(), before);
}

TEST(LogTests, MuteLoggerAllowsLoggingAfterDestruction)
{
    LogCaptureSt cap;
    {
        MuteLogger mute;
    }
    spdlog::info("after mute");
    EXPECT_TRUE(cap.contains("after mute"));
}

// ---------------------------------------------------------------------------
// configureLogger
// ---------------------------------------------------------------------------

class LogConfigureTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        prevSinks_ = spdlog::default_logger()->sinks();
        prevLevel_ = spdlog::get_level();
    }

    void TearDown() override
    {
        spdlog::default_logger()->sinks() = prevSinks_;
        spdlog::set_level(prevLevel_);
    }

    TempDir& tempDir() { return tempDir_; }
private:
    TempDir tempDir_ {"log-configure"};
    std::vector<spdlog::sink_ptr> prevSinks_;
    spdlog::level::level_enum prevLevel_ {spdlog::level::info};
};

TEST_F(LogConfigureTests, ConfigureLoggerCreatesLogFile)
{
    const fs::path logFilename = "test_app.log";
    configureLogger(tempDir().path(), logFilename);

    // Emit a message so the file is flushed/created.
    spdlog::default_logger()->flush();

    EXPECT_TRUE(fs::exists(tempDir().path() / logFilename));
}

TEST_F(LogConfigureTests, ConfigureLoggerInstallsTwoSinks)
{
    const fs::path logFilename = "two_sinks.log";
    configureLogger(tempDir().path(), logFilename);

    // Console sink + file sink.
    EXPECT_EQ(spdlog::default_logger()->sinks().size(), 2U);
}

} // namespace
