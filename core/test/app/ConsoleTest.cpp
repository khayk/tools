#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <core/app/Console.h>
#include <core/utils/LogCapture.h>

#include <csignal>

using core::Console;
using core::Runnable;
using core::utl::LogCaptureSt;

namespace {

class MockRunnable : public Runnable
{
public:
    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, shutdown, (), (noexcept, override));
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(ConsoleTests, ConstructorThrowsOnNullRunnable)
{
    EXPECT_THROW(Console(nullptr), std::runtime_error);
}

TEST(ConsoleTests, ConstructorLogsMessage)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    LogCaptureSt cap;
    Console console(runnable);
    EXPECT_TRUE(cap.contains("console application"));
}

TEST(ConsoleTests, DestructorLogsMessage)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    LogCaptureSt cap;
    { Console console(runnable); }
    EXPECT_TRUE(cap.contains("closed"));
}

TEST(ConsoleTests, DuplicateInstanceThrows)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    Console first(runnable);
    EXPECT_THROW({ Console second(runnable); }, std::runtime_error);
}

TEST(ConsoleTests, AfterDestructionNewInstanceCanBeCreated)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(2);

    { Console first(runnable); }
    EXPECT_NO_THROW({ Console second(runnable); });
}

// ---------------------------------------------------------------------------
// run / shutdown delegation
// ---------------------------------------------------------------------------

TEST(ConsoleTests, RunDelegatesToRunnable)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, run()).Times(1);
    EXPECT_CALL(*runnable, shutdown()).Times(1); // destructor

    Console console(runnable);
    console.run();
}

TEST(ConsoleTests, ShutdownDelegatesToRunnable)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    Console console(runnable);
    console.shutdown();
    // ~Impl() calls shutdown() again, but stopped_ is true so it's a no-op
}

TEST(ConsoleTests, ShutdownIsIdempotent)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    Console console(runnable);
    console.shutdown();
    console.shutdown(); // second call must not reach the runnable
}

TEST(ConsoleTests, RunDoesNothingWhenRunnableExpired)
{
    std::unique_ptr<Console> console;
    {
        auto runnable = std::make_shared<MockRunnable>();
        EXPECT_CALL(*runnable, run()).Times(0);
        EXPECT_CALL(*runnable, shutdown()).Times(0);
        console = std::make_unique<Console>(runnable);
        // runnable expires here; weak_ptr inside Console goes dead
    }

    console->run();  // weak_ptr expired → no-op
    console.reset(); // ~Impl() shutdown() also a no-op
}

// ---------------------------------------------------------------------------
// Signal handling
// ---------------------------------------------------------------------------

TEST(ConsoleTests, SigintCallsShutdown)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    Console console(runnable);
    std::raise(SIGINT);
    // stopped_ is now true; ~Impl() shutdown() is a no-op
}

} // namespace
