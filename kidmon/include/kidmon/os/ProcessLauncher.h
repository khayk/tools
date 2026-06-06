#pragma once

#include <string>
#include <vector>
#include <utility>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

namespace km {

using Args = std::vector<std::string>;
using Env = std::vector<std::pair<std::string, std::string>>;

class ProcessLauncher
{
public:
    virtual ~ProcessLauncher() = default;

    virtual bool launch(const fs::path& exec, 
                        const Args& args, 
                        const Env& env = {}) = 0;
};

using ProcessLauncherPtr = std::unique_ptr<ProcessLauncher>;

} // namespace km
