#include "../Api.h"

class ApiImpl : public Api
{
public:
    WindowPtr forgroundWindow() override;
    ProcessLauncherPtr createProcessLauncher() override;
};
