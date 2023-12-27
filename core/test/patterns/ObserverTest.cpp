#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <patterns/Observer.h>

using namespace dp;
using ::testing::_;

namespace {

class ITestEvents
{
public:
    virtual ~ITestEvents() = default;

    virtual void zero() const noexcept = 0;
    virtual void one(int val) const = 0;
    virtual void two(const std::string& msg, double val) = 0;
};

class TestObserver : public Observable<ITestEvents>
{
public:
    void triggerZero()
    {
        notify(&ITestEvents::zero);
    }

    void triggerOne(int val)
    {
        notify(&ITestEvents::one, val);
    }

    void triggerTwo(const std::string& str, double val)
    {
        notify(&ITestEvents::two, str, val);
    }
};

class MockEvents : public ITestEvents
{
public:
    MOCK_METHOD(void, zero, (), (const, noexcept, override));
    MOCK_METHOD(void, one, (int), (const, override));
    MOCK_METHOD(void, two, (const std::string&, double), (override));
};

TEST(ObserverTests, ManualObserve)
{
    TestObserver obs;
    MockEvents listener;
    constexpr int n = 3;

    EXPECT_CALL(listener, zero()).Times(n);
    EXPECT_CALL(listener, one(_)).Times(n);
    EXPECT_CALL(listener, two(_, _)).Times(n);

    obs.add(listener);

    // To show, that callbacks will be invoked as many
    // times as we trigger them
    for (int i = 0; i < n; ++i)
    {
        obs.triggerZero();
        obs.triggerOne(1);
        obs.triggerTwo("foo", 2);
    }
}

TEST(ObserverTests, ScopedObserve)
{
    TestObserver obs;
    MockEvents listener;

    EXPECT_CALL(listener, zero()).Times(1);
    EXPECT_CALL(listener, one(_)).Times(1);
    EXPECT_CALL(listener, two(_, _)).Times(1);

    {
        ScopedObserve<ITestEvents> so(obs, listener);

        // Those should trigger events
        obs.triggerZero();
        obs.triggerOne(1);
        obs.triggerTwo("foo", 2);
    }

    // Listener won't be notified
    for (int i = 0; i < 2; ++i)
    {
        obs.triggerZero();
        obs.triggerOne(1);
        obs.triggerTwo("foo", 2);
    }
}

} // namespace
