#include "Service.h"

class Service::Impl
{
    void onControl(unsigned long controlCode);
    void main(unsigned long argc, char* argv[]);

    static void serviceMain(unsigned long argc, char* argv[]);
    static void controlHandler(unsigned long controlCode);


    std::weak_ptr<Runnable> runnable_;
    //std::string           serviceName_;
    //SERVICE_STATUS_HANDLE serviceHandle_;
    //SERVICE_STATUS        serviceStatus_;
};