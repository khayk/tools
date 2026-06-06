#pragma once

#include <kidmon/os/ProcessLauncher.h>

class ProcessLauncherImpl : public km::ProcessLauncher
{
public:
    bool launch(const fs::path& exec,
                const km::Args& args,
                const km::Env& env) override;
};