#include <kidmon/common/Service.h>
#include <kidmon/common/Runnable.h>
#include <core/utils/Throw.h>

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
        std::ignore = name_;
        core::throwNotImplemented();
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
