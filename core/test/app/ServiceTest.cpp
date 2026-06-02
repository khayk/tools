#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <core/app/Service.h>
#include <core/utils/LogCapture.h>
#include <core/utils/Log.h>

#include <csignal>

using core::Service;
using core::Runnable;
using core::utl::LogCaptureSt;
using core::utl::MuteLogger;

namespace {

class MockRunnable : public Runnable
{
public:
    MOCK_METHOD(void, run, (), (override));
    MOCK_METHOD(void, shutdown, (), (noexcept, override));
};

// ---------------------------------------------------------------------------
// Construction / destruction — all platforms
// ---------------------------------------------------------------------------

TEST(ServiceTests, ConstructorLogsMessage)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(0);

    LogCaptureSt cap;
    Service service(runnable, "test");
    EXPECT_TRUE(cap.contains("Working as a service"));
}

TEST(ServiceTests, DestructorLogsMessage)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(0);

    LogCaptureSt cap;
    { Service service(runnable, "test"); }
    EXPECT_TRUE(cap.contains("Service is stopped"));
}

TEST(ServiceTests, DestructorDoesNotCallShutdown)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(0);

    MuteLogger mute;
    Service service(runnable, "test");
}

// ---------------------------------------------------------------------------
// shutdown — all platforms (same implementation on macOS and Linux)
// ---------------------------------------------------------------------------

TEST(ServiceTests, ShutdownDelegatesToRunnable)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    MuteLogger mute;
    Service service(runnable, "test");
    service.shutdown();
}

TEST(ServiceTests, ShutdownDoesNothingWhenRunnableExpired)
{
    MuteLogger mute;
    std::unique_ptr<Service> service;
    {
        auto runnable = std::make_shared<MockRunnable>();
        EXPECT_CALL(*runnable, shutdown()).Times(0);
        service = std::make_unique<Service>(runnable, "test");
    } // runnable expires; weak_ptr inside Service goes dead

    service->shutdown();
    service.reset();
}

// ---------------------------------------------------------------------------
// Windows — run() calls StartServiceCtrlDispatcher(), which fails immediately
// with ERROR_FAILED_SERVICE_CONTROLLER_CONNECT (1063) when the process was not
// launched by the SCM. serviceMain / controlHandler / onControl are only
// reachable when the SCM calls back into the process, so they cannot be
// exercised in a unit test.
// ---------------------------------------------------------------------------

#ifdef _WIN32

TEST(ServiceTests, RunFailsGracefullyWhenNotLaunchedByScm)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, run()).Times(0);
    EXPECT_CALL(*runnable, shutdown()).Times(0);

    LogCaptureSt cap;
    Service service(runnable, "test");
    service.run(); // StartServiceCtrlDispatcher returns 0, logs error, returns
    EXPECT_TRUE(cap.contains("Failed to start service ctrl dispatcher"));
}

// ---------------------------------------------------------------------------
// macOS — full daemon implementation
// ---------------------------------------------------------------------------

#elif defined(__APPLE__)

TEST(ServiceTests, RunDelegatesToRunnable)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, run()).Times(1);
    EXPECT_CALL(*runnable, shutdown()).Times(0);

    MuteLogger mute;
    Service service(runnable, "test");
    service.run();
}

TEST(ServiceTests, RunLogsServiceNameOnStartAndStop)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, run()).Times(1);
    EXPECT_CALL(*runnable, shutdown()).Times(0);

    LogCaptureSt cap;
    Service service(runnable, "my-daemon");
    service.run();

    EXPECT_TRUE(cap.contains("my-daemon"));
}

TEST(ServiceTests, RunDoesNothingWhenRunnableExpired)
{
    MuteLogger mute;
    std::unique_ptr<Service> service;
    {
        auto runnable = std::make_shared<MockRunnable>();
        EXPECT_CALL(*runnable, run()).Times(0);
        EXPECT_CALL(*runnable, shutdown()).Times(0);
        service = std::make_unique<Service>(runnable, "test");
    } // runnable expires; weak_ptr inside Service goes dead

    service->run();
    service.reset();
}

// Signal handlers are installed by run() via sigaction(). std::raise() delivers
// the signal synchronously to the calling thread, so the handler completes
// before raise() returns, making the shutdown call observable immediately.

TEST(ServiceTests, SigtermCallsShutdown)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, run())
        .Times(1)
        .WillOnce([&]() { std::raise(SIGTERM); });
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    MuteLogger mute;
    Service service(runnable, "test");
    service.run();
}

TEST(ServiceTests, SigintCallsShutdown)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, run())
        .Times(1)
        .WillOnce([&]() { std::raise(SIGINT); });
    EXPECT_CALL(*runnable, shutdown()).Times(1);

    MuteLogger mute;
    Service service(runnable, "test");
    service.run();
}

#else

// ---------------------------------------------------------------------------
// Linux — run() is a stub that throws NotImplemented
// ---------------------------------------------------------------------------

TEST(ServiceTests, RunThrowsNotImplemented)
{
    auto runnable = std::make_shared<MockRunnable>();
    EXPECT_CALL(*runnable, run()).Times(0);
    EXPECT_CALL(*runnable, shutdown()).Times(0);

    MuteLogger mute;
    Service service(runnable, "test");
    EXPECT_THROW(service.run(), std::runtime_error);
}

#endif

} // namespace
