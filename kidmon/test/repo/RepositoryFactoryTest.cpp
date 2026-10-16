#include <gtest/gtest.h>

#include <kidmon/repo/RepositoryFactory.h>
#include <core/utils/File.h>

#include <stdexcept>

using namespace km;

TEST(RepositoryFactoryTest, ExplicitTypePrefix)
{
    const auto fsSpec = parseRepositorySpec("fs:/no/such/dir");
    EXPECT_EQ(fsSpec.type, RepositoryType::FileSystem);
    EXPECT_EQ(fsSpec.location, fs::path("/no/such/dir"));

    const auto sqlSpec = parseRepositorySpec("sqlite:activity.data");
    EXPECT_EQ(sqlSpec.type, RepositoryType::Sqlite);
    EXPECT_EQ(sqlSpec.location, fs::path("activity.data"));
}

TEST(RepositoryFactoryTest, InfersTypeFromLocation)
{
    core::file::TempDir dir("kdmn-fct-tst");

    EXPECT_EQ(parseRepositorySpec(core::file::path2s(dir.path())).type,
              RepositoryType::FileSystem);
    EXPECT_EQ(parseRepositorySpec("activity.db").type, RepositoryType::Sqlite);
    EXPECT_EQ(parseRepositorySpec("ACTIVITY.SQLITE").type, RepositoryType::Sqlite);
    EXPECT_EQ(parseRepositorySpec("activity.sqlite3").type, RepositoryType::Sqlite);
}

TEST(RepositoryFactoryTest, DriveLetterIsNotAType)
{
    const auto spec = parseRepositorySpec("C:\\data\\activity.db");

    EXPECT_EQ(spec.type, RepositoryType::Sqlite);
    EXPECT_EQ(spec.location, fs::path("C:\\data\\activity.db"));
}

TEST(RepositoryFactoryTest, InvalidSpecsThrow)
{
    EXPECT_THROW(parseRepositorySpec("/no/such/dir"), std::invalid_argument);
    EXPECT_THROW(parseRepositorySpec("activity.txt"), std::invalid_argument);
    EXPECT_THROW(parseRepositorySpec("fs:"), std::invalid_argument);
    EXPECT_THROW(parseRepositorySpec("sqlite:"), std::invalid_argument);
}

TEST(RepositoryFactoryTest, ToString)
{
    EXPECT_EQ(toString({RepositoryType::FileSystem, "/data"}), "fs:/data");
    EXPECT_EQ(toString({RepositoryType::Sqlite, "a.db"}), "sqlite:a.db");
}

TEST(RepositoryFactoryTest, ReadRequiresExistingRepository)
{
    core::file::TempDir dir("kdmn-fct-tst");
    const RepositorySpec missingDb {RepositoryType::Sqlite, dir.path() / "missing.db"};
    const RepositorySpec missingDir {RepositoryType::FileSystem,
                                     dir.path() / "missing"};

    EXPECT_THROW(openRepository(missingDb, RepositoryAccess::Read),
                 std::invalid_argument);
    EXPECT_THROW(openRepository(missingDir, RepositoryAccess::Read),
                 std::invalid_argument);

    // Reading must not create the database as a side effect
    EXPECT_FALSE(fs::exists(missingDb.location));
}

TEST(RepositoryFactoryTest, WriteCreatesSqliteDatabase)
{
    core::file::TempDir dir("kdmn-fct-tst");
    const RepositorySpec spec {RepositoryType::Sqlite, dir.path() / "sub" / "new.db"};

    EXPECT_NE(openRepository(spec, RepositoryAccess::Write), nullptr);
    EXPECT_TRUE(fs::is_regular_file(spec.location));
    EXPECT_NE(openRepository(spec, RepositoryAccess::Read), nullptr);
}

TEST(RepositoryFactoryTest, OpensExistingFileSystemRepository)
{
    core::file::TempDir dir("kdmn-fct-tst");
    const RepositorySpec spec {RepositoryType::FileSystem, dir.path()};

    EXPECT_NE(openRepository(spec, RepositoryAccess::Read), nullptr);
}

TEST(RepositoryFactoryTest, MigrationRefusesSameRepository)
{
    core::file::TempDir dir("kdmn-fct-tst");
    const auto dbFile = dir.path() / "a.db";
    core::file::write(dbFile, "data");
    fs::create_hard_link(dbFile, dir.path() / "link.db");

    const RepositorySpec source {RepositoryType::Sqlite, dbFile};

    // Same file under another spelling or name
    EXPECT_THROW(
        validateMigration(source, {RepositoryType::Sqlite, dir.path() / "." / "a.db"}),
        std::invalid_argument);
    EXPECT_THROW(
        validateMigration(source, {RepositoryType::Sqlite, dir.path() / "link.db"}),
        std::invalid_argument);
}

TEST(RepositoryFactoryTest, MigrationRefusesTargetInsideFolderSource)
{
    core::file::TempDir dir("kdmn-fct-tst");
    const RepositorySpec source {RepositoryType::FileSystem, dir.path()};

    EXPECT_THROW(
        validateMigration(source, {RepositoryType::FileSystem, dir.path() / "backup"}),
        std::invalid_argument);
    EXPECT_THROW(
        validateMigration(source,
                          {RepositoryType::Sqlite, dir.path() / "sub" / "a.db"}),
        std::invalid_argument);

    // A sibling with a common name prefix is not inside
    const RepositorySpec sibling {
        RepositoryType::FileSystem,
        fs::path(core::file::path2s(dir.path()) + "-backup")};
    EXPECT_NO_THROW(validateMigration(source, sibling));
}

TEST(RepositoryFactoryTest, MigrationRefusesNonEmptyFolderTarget)
{
    core::file::TempDir dir("kdmn-fct-tst");
    const RepositorySpec source {RepositoryType::Sqlite, dir.path() / "a.db"};
    const RepositorySpec target {RepositoryType::FileSystem, dir.path() / "target"};

    // Missing or empty folders are fine
    EXPECT_NO_THROW(validateMigration(source, target));
    fs::create_directories(target.location);
    EXPECT_NO_THROW(validateMigration(source, target));

    fs::create_directories(target.location / "alice");
    EXPECT_THROW(validateMigration(source, target), std::invalid_argument);
}

TEST(RepositoryFactoryTest, MigrationRefusesWrongTargetKind)
{
    core::file::TempDir dir("kdmn-fct-tst");
    const RepositorySpec source {RepositoryType::Sqlite, dir.path() / "a.db"};
    core::file::write(dir.path() / "file", "");
    fs::create_directories(dir.path() / "folder.db");

    EXPECT_THROW(
        validateMigration(source, {RepositoryType::FileSystem, dir.path() / "file"}),
        std::invalid_argument);
    EXPECT_THROW(
        validateMigration(source, {RepositoryType::Sqlite, dir.path() / "folder.db"}),
        std::invalid_argument);
}

TEST(RepositoryFactoryTest, MigrationAllowsExistingSqliteTarget)
{
    core::file::TempDir dir("kdmn-fct-tst");
    const RepositorySpec source {RepositoryType::FileSystem, dir.path() / "reports"};
    const RepositorySpec target {RepositoryType::Sqlite, dir.path() / "a.db"};

    // A SQLite target skips entries it already has, so it may hold data
    core::file::write(target.location, "data");
    EXPECT_NO_THROW(validateMigration(source, target));
}
