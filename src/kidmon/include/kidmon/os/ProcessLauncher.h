#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

class ProcessLauncher
{
public:
    virtual ~ProcessLauncher() = default;

    virtual bool launch(const fs::path& exec,
                        const std::vector<std::string>& args) = 0;
};

using ProcessLauncherPtr = std::unique_ptr<ProcessLauncher>;