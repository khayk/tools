#include "../Api.h"

class ApiImpl : public Api
{
public:
    WindowPtr foregroundWindow() override;
    ProcessLauncherPtr createProcessLauncher() override;
};
