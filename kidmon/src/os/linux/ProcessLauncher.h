#pragma once

#include "../ProcessLauncher.h"

class ProcessLauncherImpl : public ProcessLauncher
{
public:
    bool launch(const fs::path& exec,
                const std::vector<std::string>& args) override;
};