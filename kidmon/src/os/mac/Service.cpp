#include <kidmon/common/Service.h>
#include <kidmon/common/Runnable.h>

#include <csignal>
#include <spdlog/spdlog.h>

namespace km {

class Service::Impl
{
public:
    Impl(const std::shared_ptr<Runnable>& runnable, std::string name)
        : runnable_(runnable)
        , name_(std::move(name))
    {
        instance_ = this;
    }

    ~Impl()
    {
        instance_ = nullptr;
    }

    void run()
    {
        installSignalHandlers();

        spdlog::info("Daemon '{}' starting", name_);

        try
        {
            std::shared_ptr<Runnable> runnable = runnable_.lock();

            if (runnable)
            {
                runnable->run();
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("Exception inside the run function: {}", e.what());
        }

        spdlog::info("Daemon '{}' stopped", name_);
    }

    void shutdown() noexcept
    {
        try
        {
            std::shared_ptr<Runnable> runnable = runnable_.lock();

            if (runnable)
            {
                runnable->shutdown();
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("Error inside the {}, desc: {}", __FUNCTION__, e.what());
        }
    }

private:
    // Must only call async-signal-safe functions — no spdlog, no malloc.
    // asio::io_context::stop() (called transitively via shutdown()) uses
    // atomics and is safe to call from a signal handler.
    static void onSignal(int /*signum*/) noexcept
    {
        if (instance_)
        {
            instance_->shutdown();
        }
    }

    static void installSignalHandlers() noexcept
    {
        struct sigaction sa {};
        sa.sa_handler = &Impl::onSignal;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGINT, &sa, nullptr);
    }

    std::weak_ptr<Runnable> runnable_;
    std::string name_;

    static Impl* instance_;
};

Service::Impl* Service::Impl::instance_ = nullptr;

Service::Service(const std::shared_ptr<Runnable>& runnable, std::string name)
    : impl_(std::make_unique<Impl>(runnable, std::move(name)))
{
    spdlog::info("Working as a service");
}

Service::~Service()
{
    impl_.reset();
    spdlog::info("Service is stopped");
}

void Service::run()
{
    impl_->run();
}

void Service::shutdown() noexcept
{
    impl_->shutdown();
}

} // namespace km
