#include "Console.h"

#include <spdlog/spdlog.h>

#include <csignal>
#include <mutex>

std::mutex g_mutex;
void* g_instance{ nullptr };

class Console::Impl
{
    std::weak_ptr<Runnable> runnable_;
    std::atomic<bool> stopped_{ false };
    _crt_signal_t prevSignal_{ nullptr };

    static void signalHandler(int signal)
    {
        if (signal == SIGINT)
        {
            std::unique_lock guard(g_mutex);
            Console::Impl* instance = reinterpret_cast<Console::Impl*>(g_instance);

            if (instance)
            {
                instance->shutdown();
                instance = nullptr;
            }
        }
    }

public:
    Impl(const std::shared_ptr<Runnable>& runnable)
        : runnable_(runnable)
    {
        if (!runnable)
        {
            throw std::runtime_error("Runnable initialiation attempt with nullptr");
        }

        {
            std::unique_lock guard(g_mutex);

            if (g_instance)
            {
                throw std::runtime_error("Console instance still in use");
            }

            g_instance = reinterpret_cast<void*>(this);

            // Install a signal handler, to handle stop event
            prevSignal_ = std::signal(SIGINT, signalHandler);
        }
    }

    ~Impl()
    {
        shutdown();

        // Wait until all other references dropped on this object, so that the distruction
        // takes place in the main thread
        while (runnable_.use_count() > 1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::unique_lock guard(g_mutex);

        std::signal(SIGINT, prevSignal_);
        g_instance = nullptr;
    }

    void run()
    {
        // Acquire shared pointer and use if it is alive
        std::shared_ptr<Runnable> runnable = runnable_.lock();

        if (runnable)
        {
            stopped_ = false;
            runnable->run();
        }
    }

    void shutdown()
    {
        if (stopped_)
        {
            return;
        }

        try
        {
            // Acquire shared pointer and use if it is alive
            std::shared_ptr<Runnable> runnable = runnable_.lock();

            if (runnable)
            {
                runnable->shutdown();
                stopped_ = true;
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("Error inside the {}, desc: {}", __FUNCTION__, e.what());
        }
    }
};

Console::Console(const std::shared_ptr<Runnable>& runnable)
    : impl_(std::make_unique<Impl>(runnable))
{
    spdlog::info("Working as a console application");
}

Console::~Console()
{
    impl_.reset();
    spdlog::info("Console application is closed");
}

void Console::run()
{
    impl_->run();
}

void Console::shutdown()
{
    impl_->shutdown();
}
