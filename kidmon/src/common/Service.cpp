#include "Service.h"
#include "Tracer.h"

#include <Windows.h>
#include <spdlog/spdlog.h>

#include <array>

namespace {

const char* serviceStateToStr(DWORD status) noexcept
{
    switch (status)
    {
        case SERVICE_STOPPED:
            return "SERVICE_STOPPED";
        case SERVICE_START_PENDING:
            return "SERVICE_START_PENDING";
        case SERVICE_STOP_PENDING:
            return "SERVICE_STOP_PENDING";
        case SERVICE_RUNNING:
            return "SERVICE_RUNNING";
        case SERVICE_CONTINUE_PENDING:
            return "SERVICE_CONTINUE_PENDING";
        case SERVICE_PAUSE_PENDING:
            return "SERVICE_PAUSE_PENDING";
        case SERVICE_PAUSED:
            return "SERVICE_PAUSED";
    }

    return "";
}

const char* serviceControlCodeToStr(DWORD controlCode) noexcept
{
    switch (controlCode)
    {
        case SERVICE_CONTROL_STOP:
            return "SERVICE_CONTROL_STOP";
        case SERVICE_CONTROL_PAUSE:
            return "SERVICE_CONTROL_PAUSE";
        case SERVICE_CONTROL_CONTINUE:
            return "SERVICE_CONTROL_CONTINUE";
        case SERVICE_CONTROL_INTERROGATE:
            return "SERVICE_CONTROL_INTERROGATE";
        case SERVICE_CONTROL_SHUTDOWN:
            return "SERVICE_CONTROL_SHUTDOWN";
        case SERVICE_CONTROL_PRESHUTDOWN:
            return "SERVICE_CONTROL_PRESHUTDOWN";
    }
    return "";
}

}

class Service::Impl
{
public:
    Impl(const std::shared_ptr<Runnable>& runnable, std::string name)
        : runnable_(runnable)
        , name_(std::move(name))
        , handle_()
        , status_()
    {
        instance_ = this;
    }

    ~Impl()
    {
        instance_ = nullptr;
    }
    
    void run()
    {
        auto name = name_;

        const std::array<SERVICE_TABLE_ENTRY, 2> startTable = {
            SERVICE_TABLE_ENTRY {name.data(), &Service::Impl::serviceMain},
            SERVICE_TABLE_ENTRY {nullptr, nullptr}};

        if (0 == ::StartServiceCtrlDispatcher(startTable.data()))
        {
            spdlog::error("Failed to start service ctrl dispatcher: {0}", GetLastError());
        }
    }

    void shutdown() noexcept
    {
        try
        {
            // Acquire shared pointer and use if it is alive
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
    static void serviceMain(unsigned long argc, char* argv[])
    {
        ScopedTrace sc(__FUNCTION__);
        //spdlog::info("WindowsService::serviceMain");

        if (instance_)
        {
            instance_->main(argc, argv);
        }
    }

    static void controlHandler(unsigned long controlCode)
    {
        if (instance_)
        {
            instance_->onControl(controlCode);
        }
    }

    void main(unsigned long argc, char* argv[])
    {
        std::ignore = argc;
        std::ignore = argv;

        ScopedTrace st(__FUNCTION__);

        status_.dwServiceType = SERVICE_WIN32;
        status_.dwCurrentState = SERVICE_START_PENDING;
        status_.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PRESHUTDOWN;
        status_.dwWin32ExitCode = 0;
        status_.dwServiceSpecificExitCode = 0;
        status_.dwCheckPoint = 0;
        status_.dwWaitHint = 0;

        handle_ = ::RegisterServiceCtrlHandler(name_.c_str(), &Impl::controlHandler);

        if (handle_ != nullptr)
        {
            status_.dwCurrentState = SERVICE_RUNNING;
            status_.dwCheckPoint = 0;
            status_.dwWaitHint = 0;

            if (!::SetServiceStatus(handle_, &status_))
            {
                spdlog::error("Failed to update service status: {0}", GetLastError());
            }
            else
            {
                spdlog::trace("Service state: {}",
                              serviceStateToStr(status_.dwCurrentState));
            }

            try
            {
                // Do the work
                std::shared_ptr<Runnable> runnable = runnable_.lock();

                if (runnable)
                {
                    runnable->run();
                }
            }
            catch (const std::exception& e)
            {
                spdlog::error("Exception inside the run function: {0}", e.what());
            }

            status_.dwCurrentState = SERVICE_STOPPED;
            status_.dwWin32ExitCode = 0;
            status_.dwCheckPoint = 0;
            status_.dwWaitHint = 0;

            if (!::SetServiceStatus(handle_, &status_))
            {
                spdlog::error("Failed to update service status: {0}", GetLastError());
            }
            else
            {
                spdlog::trace("Service state: {}",
                              serviceStateToStr(status_.dwCurrentState));
            }
        }
        else
        {
            spdlog::error("Failed to register service control handler: {0}",
                          GetLastError());
        }
    }
    
    void onControl(unsigned long controlCode)
    {
        spdlog::trace("Service action `{}` requested",
                      serviceControlCodeToStr(controlCode));

        switch (controlCode)
        {
            case SERVICE_CONTROL_STOP:
                [[fallthrough]];
            case SERVICE_CONTROL_PRESHUTDOWN:
                status_.dwCurrentState = SERVICE_STOP_PENDING;
                status_.dwWin32ExitCode = 0;
                status_.dwCheckPoint = 0;
                status_.dwWaitHint = 0;
                break;

            case SERVICE_CONTROL_PAUSE:
                status_.dwCurrentState = SERVICE_PAUSED;
                break;

            case SERVICE_CONTROL_CONTINUE:
                status_.dwCurrentState = SERVICE_RUNNING;
                break;

            case SERVICE_CONTROL_INTERROGATE:
                break;
        }

        // It is very important to update the status to SERVICE_STOP_PENDING, only then
        // call the shutdown, otherwise, if, as a result of shutdown, the other thread
        // sets service status to SERVICE_STOPPED and after that we set the status to
        // SERVICE_STOP_PENDING, service control manager will hang and wait until we set
        // the status to SERVICE_STOPPED.
        if (!::SetServiceStatus(handle_, &status_))
        {
            spdlog::error("Failed to update service status: {0}", GetLastError());
        }
        else
        {
            spdlog::trace("Service state: {}",
                          serviceStateToStr(status_.dwCurrentState));
        }

        if (status_.dwCurrentState == SERVICE_STOP_PENDING)
        {
            // Terminating
            shutdown();
        }
    }

    std::weak_ptr<Runnable> runnable_;
    std::string name_;
    SERVICE_STATUS_HANDLE handle_;
    SERVICE_STATUS status_;

    static Impl* instance_;
};

Service::Impl* Service::Impl::instance_ = nullptr;

Service::Service(const std::shared_ptr<Runnable>& runnable, std::string name)
    : impl_(std::make_unique<Impl>(runnable, std::move(name)))
{
    spdlog::info("Working as a windows service");
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

