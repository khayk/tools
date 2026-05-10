#include <kidmon/os/Api.h>

class ApiImpl : public km::Api
{
public:
    km::WindowPtr foregroundWindow() override;
    km::ProcessLauncherPtr createProcessLauncher() override;
};
