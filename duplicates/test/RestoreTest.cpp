#include <gtest/gtest.h>

#include <duplicates/Restore.h>
#include <duplicates/DeletionStrategy.h>
#include <core/utils/File.h>
#include <core/utils/Log.h>

#include <sstream>
#include <string>
#include <vector>

namespace tools::dups {

using namespace core;
using utl::MuteLogger;

namespace {

std::string content(const fs::path& file)
{
    std::string data;
    std::error_code ec;
    EXPECT_TRUE(file::read(file, data, ec)) << file;
    return data;
}

class RestoreTest : public testing::Test
{
protected:
    MuteLogger mute_;
    file::TempDir data_ {"dups"};
    fs::path backupDir_ {data_.path() / "backup"};
    fs::path srcDir_ {data_.path() / "src"};

    void SetUp() override
    {
        fs::create_directories(srcDir_);
    }

    fs::path makeFile(const fs::path& relPath, std::string_view text)
    {
        auto path = srcDir_ / relPath;
        fs::create_directories(path.parent_path());
        file::write(path, text);
        return path;
    }

    /**
     * @brief Delete the given files through BackupAndDelete, as one run.
     * @return The run directory.
     */
    fs::path deleteRun(const std::vector<fs::path>& files)
    {
        BackupAndDelete strategy(backupDir_);
        for (const auto& f : files)
        {
            strategy.remove(f);
        }
        return strategy.runDir();
    }

    static std::vector<JournalEntry> journalOf(const fs::path& runDir)
    {
        return readJournal(runDir / JOURNAL_FILE_NAME);
    }
};

} // namespace

TEST_F(RestoreTest, RestoresAllFilesAndRemovesRunDir)
{
    const auto a = makeFile("a.txt", "alpha");
    const auto b = makeFile("nested/deeper/b.txt", "beta");
    const auto c = makeFile("with space/c d.txt", "gamma");
    const auto runDir = deleteRun({a, b, c});

    ASSERT_FALSE(fs::exists(a));
    ASSERT_FALSE(fs::exists(b));
    ASSERT_FALSE(fs::exists(c));

    const auto report = restoreRun(runDir);

    EXPECT_EQ(report.restored.size(), 3U);
    EXPECT_EQ(report.total(), 3U);
    EXPECT_TRUE(report.complete());
    EXPECT_TRUE(report.runDirRemoved);
    EXPECT_FALSE(fs::exists(runDir));
    EXPECT_TRUE(fs::exists(backupDir_)); // only the run is removed, not the root

    EXPECT_EQ(content(a), "alpha");
    EXPECT_EQ(content(b), "beta");
    EXPECT_EQ(content(c), "gamma");
}

TEST_F(RestoreTest, RecreatesRemovedParentDirectories)
{
    const auto file = makeFile("gone/away/photo.jpg", "pixels");
    const auto runDir = deleteRun({file});

    // e.g. the user removed the folder after it was emptied by the deletion
    fs::remove_all(srcDir_ / "gone");

    const auto report = restoreRun(runDir);

    EXPECT_EQ(report.restored.size(), 1U);
    EXPECT_EQ(content(file), "pixels");
}

TEST_F(RestoreTest, DropsBackupWhenIdenticalFileExists)
{
    const auto file = makeFile("same.txt", "unchanged");
    const auto runDir = deleteRun({file});
    file::write(file, "unchanged");

    const auto report = restoreRun(runDir);

    EXPECT_EQ(report.identical.size(), 1U);
    EXPECT_TRUE(report.restored.empty());
    EXPECT_TRUE(report.complete());
    EXPECT_TRUE(report.runDirRemoved);
    EXPECT_EQ(content(file), "unchanged");
}

TEST_F(RestoreTest, KeepsBothFilesOnConflict)
{
    const auto conflicting = makeFile("conflict.txt", "original content");
    const auto other = makeFile("other.txt", "other content");
    const auto runDir = deleteRun({conflicting, other});

    // A different file now lives at the original path (same size, to be sure the
    // comparison looks at the content and not only at the size).
    file::write(conflicting, "differs content!");

    const auto report = restoreRun(runDir);

    ASSERT_EQ(report.conflicts.size(), 1U);
    EXPECT_EQ(report.conflicts.front().original, conflicting);
    EXPECT_EQ(report.restored.size(), 1U);
    EXPECT_FALSE(report.complete());
    EXPECT_FALSE(report.runDirRemoved);

    // Neither side was touched...
    EXPECT_EQ(content(conflicting), "differs content!");
    EXPECT_EQ(content(report.conflicts.front().backup), "original content");
    EXPECT_EQ(content(other), "other content");

    // ...and the journal now holds only what is still pending.
    EXPECT_EQ(journalOf(runDir), report.conflicts);
}

TEST_F(RestoreTest, CompletesOnRetryOnceConflictIsResolved)
{
    const auto file = makeFile("photo.jpg", "original");
    const auto runDir = deleteRun({file});
    file::write(file, "imposter");

    ASSERT_EQ(restoreRun(runDir).conflicts.size(), 1U);

    // The user resolves the conflict by removing the newer file.
    fs::remove(file);
    const auto report = restoreRun(runDir);

    EXPECT_EQ(report.restored.size(), 1U);
    EXPECT_EQ(report.total(), 1U); // the earlier restore left only the conflict
    EXPECT_TRUE(report.runDirRemoved);
    EXPECT_EQ(content(file), "original");
}

TEST_F(RestoreTest, SkipsEntriesWhoseBackupIsMissing)
{
    const auto kept = makeFile("kept.txt", "kept");
    const auto lost = makeFile("lost.txt", "lost");
    const auto runDir = deleteRun({kept, lost});

    // Simulates a crash between journaling the deletion and moving the file:
    // the journal names a backup that does not exist.
    const auto entries = journalOf(runDir);
    ASSERT_EQ(entries.size(), 2U);
    fs::remove(entries.back().backup);

    const auto report = restoreRun(runDir);

    EXPECT_EQ(report.restored.size(), 1U);
    ASSERT_EQ(report.missing.size(), 1U);
    EXPECT_EQ(report.missing.front().original, lost);
    EXPECT_TRUE(report.complete());
    EXPECT_TRUE(report.runDirRemoved);
    EXPECT_EQ(content(kept), "kept");
    EXPECT_FALSE(fs::exists(lost));
}

TEST_F(RestoreTest, DryRunChangesNothing)
{
    const auto toRestore = makeFile("restore.txt", "restore me");
    const auto same = makeFile("same.txt", "same");
    const auto conflict = makeFile("conflict.txt", "old");
    const auto runDir = deleteRun({toRestore, same, conflict});
    file::write(same, "same");
    file::write(conflict, "new");

    const auto journalBefore = journalOf(runDir);
    const auto report = restoreRun(runDir, {.dryRun = true});

    EXPECT_EQ(report.restored.size(), 1U);
    EXPECT_EQ(report.identical.size(), 1U);
    EXPECT_EQ(report.conflicts.size(), 1U);
    EXPECT_FALSE(report.runDirRemoved);

    EXPECT_FALSE(fs::exists(toRestore));
    EXPECT_EQ(content(conflict), "new");
    EXPECT_EQ(journalOf(runDir), journalBefore);
    for (const auto& entry : journalBefore)
    {
        EXPECT_TRUE(fs::exists(entry.backup)) << entry.backup;
    }
}

TEST_F(RestoreTest, ReportsFailureAndKeepsBackup)
{
    const auto file = makeFile("blocked/file.txt", "content");
    const auto runDir = deleteRun({file});

    // A regular file where the parent directory should be: the restore cannot
    // recreate the directory, so the entry fails but must not be lost.
    fs::remove_all(srcDir_ / "blocked");
    file::write(srcDir_ / "blocked", "not a directory");

    const auto report = restoreRun(runDir);

    ASSERT_EQ(report.failed.size(), 1U);
    EXPECT_EQ(report.failed.front().entry.original, file);
    EXPECT_FALSE(report.failed.front().error.empty());
    EXPECT_FALSE(report.complete());
    EXPECT_FALSE(report.runDirRemoved);
    EXPECT_TRUE(fs::exists(report.failed.front().entry.backup));

    const std::vector<JournalEntry> pending {report.failed.front().entry};
    EXPECT_EQ(journalOf(runDir), pending);
}

TEST_F(RestoreTest, KeepsUnexpectedFilesInRunDir)
{
    const auto file = makeFile("a.txt", "alpha");
    const auto runDir = deleteRun({file});
    file::write(runDir / "notes.txt", "not from the journal");

    const auto report = restoreRun(runDir);

    EXPECT_TRUE(report.complete());
    EXPECT_FALSE(report.runDirRemoved);
    EXPECT_TRUE(fs::exists(runDir / "notes.txt"));
    EXPECT_FALSE(fs::exists(runDir / JOURNAL_FILE_NAME));
    EXPECT_EQ(content(file), "alpha");
}

TEST_F(RestoreTest, UpdatesProgress)
{
    const auto runDir = deleteRun({makeFile("1.txt", "1"), makeFile("2.txt", "2")});

    std::ostringstream out;
    Progress progress(&out, std::chrono::milliseconds(0));
    restoreRun(runDir, {.progress = &progress});

    EXPECT_TRUE(out.str().contains("Restoring: 2 out of 2"));
}

TEST_F(RestoreTest, ThrowsWithoutJournal)
{
    EXPECT_THROW(restoreRun(data_.path() / "no-such-run"), std::runtime_error);

    fs::create_directories(backupDir_ / "deleted_empty");
    EXPECT_THROW(restoreRun(backupDir_ / "deleted_empty"), std::runtime_error);
}

} // namespace tools::dups
