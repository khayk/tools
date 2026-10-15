#include <gtest/gtest.h>

#include <Migration.h>
#include <core/utils/File.h>

#include <stdexcept>

using namespace km;
using namespace km::reports;

TEST(MigrationTest, RefusesMigratingIntoItself)
{
    core::file::TempDir dir("kdmn-mig-tst");
    const RepositorySpec source {RepositoryType::FileSystem, dir.path()};
    const RepositorySpec sameDir {RepositoryType::FileSystem, dir.path() / "."};

    EXPECT_THROW(validateMigration(source, sameDir), std::invalid_argument);
}

TEST(MigrationTest, RefusesNonEmptyFileSystemTarget)
{
    core::file::TempDir dir("kdmn-mig-tst");
    const RepositorySpec source {RepositoryType::Sqlite, dir.path() / "a.db"};
    const RepositorySpec target {RepositoryType::FileSystem, dir.path() / "target"};

    // Missing or empty folders are fine
    EXPECT_NO_THROW(validateMigration(source, target));
    fs::create_directories(target.location);
    EXPECT_NO_THROW(validateMigration(source, target));

    fs::create_directories(target.location / "alice");
    EXPECT_THROW(validateMigration(source, target), std::invalid_argument);
}

TEST(MigrationTest, AllowsExistingSqliteTarget)
{
    core::file::TempDir dir("kdmn-mig-tst");
    const RepositorySpec source {RepositoryType::FileSystem, dir.path() / "reports"};
    const RepositorySpec target {RepositoryType::Sqlite, dir.path() / "a.db"};

    // A SQLite target skips entries it already has, so it may hold data
    core::file::write(target.location, "data", 4);
    EXPECT_NO_THROW(validateMigration(source, target));
}
