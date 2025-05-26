# Tools

Own implementation of different tools.

## Build & Test

### Prerequisite

1. Tested with
    * IDEs
        * Visual Studio 2022
        * Visual Studio Code
    * Compilers
        * msvc14+
        * gcc-{11, 12, 13}

2. Install `vcpkg`
    * Checkout tag 2024.12.16 - `git checkout tags/2024.12.16 -b tag-2024.12.16`
    * For the first time setup [see](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started)
    * Consider using `-disableMetrics` options while bootstraping vcpkg

3. Install libraries
    * `./vcpkg install gtest spdlog openssl boost-asio boost-iostreams nlohmann-json cxxopts glaze --triplet=<platform>`
        * Windows `<platform>` can be `x64-windows`
        * Linux   `<platform>` can be `x64-linux`

### Instructions

* Configure
    * Navigate the root directory of the project
    * If the environment variable `VCPKG_ROOT` is defined, use the command below
        * `cmake -B build -S .`
    * Otherwise
        * `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg_path>/scripts/buildsystems/vcpkg.cmake`
    * Additionally build type can be specified as below: `Debug`, `Release`, ...
        * `-DCMAKE_BUILD_TYPE=<cfg>`
* Build
    * `cmake --build build --parallel 8 --config <cfg>`
* Extras
    * Run the command below before configure step to build with `clang`. Change clang path if required.

        ```bash
        export CC=/usr/bin/clang
        export CXX=/usr/bin/clang++
        ```

* Tests
    * CTest
        * `ctest --test-dir build --build-config <cfg>`
        * `ctest --test-dir build --build-config <cfg> [--tests-regex <regex>]`  # .*Pattern.*
        * `ctest --test-dir build -C <cfg> [-R <regex>]`
        * `-N` additional options disables execution and only prints a list of test
    * Manually run application with ending with name `-test` (TBD, utilize `ctest`)
        * Windows - `build\core\test\Debug\core-test.exe`
        * Linux - `./build/core/test/core-test`
    * [Creating and running tests with CTest](https://coderefinery.github.io/cmake-workshop/testing/)
* Install
    * `cmake --install build --prefix ./`   # Configuration picked up from the build dir
    * `cmake --install build --prefix ./ --config <cfg>`    # Update this part

## Run

* Console application
    * Run executable
* Windows service
    * Install autostart service
        * `sc create kidmon binPath= "<absolute path of the kidmon app>" start= auto DisplayName= "Kidmon application"`

## Coverage

* Windows
    * Install OpenCppCoverage
    * Locate project root directory
    * Run tests with coverage

        ```bat
        OpenCppCoverage.exe --sources tools\src\core --modules test.exe --export_type=html:.reports/core/  -- out\build\x64-Debug\test\core\core-test.exe
        OpenCppCoverage.exe --sources tools\src\kidmon --modules test.exe --export_type=html:.reports/kidmon/  -- out\build\x64-Debug\test\kidmon\kidmon-test.exe
        ```

    * TBD - the command line above is for reference only, later create a script to produce coverage
* Linux
    * Generate
        * `cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug`
    * Build and run
        * `cmake --build build -t coverage-<target-name> -j 8`
    * Example
        * `cmake --build build -t coverage-core-test -j 8`
        * `cmake --build build -t coverage-duplicates-test -j 8`
        * `cmake --build build -t coverage-kidmon-test -j 8`
    * Coverage report will be available under the `build` directory, in the `coverage-<build_target>` folder

* Read about [codecov](https://docs.codecov.com/docs/quick-start) CI

## Formatting

* The project is formatted based on the settings in `.clang-format`
    * Open bash, navigate the root directory of the project
    * Ensure `clang-format` version `17.0.1` (or above) is installed
    * Run `find . -type f \( -name "*.cpp" -o -name "*.h" \) -not -path "./build/*" | xargs clang-format -i`, to format all `{h,cpp}` files in place
    * Alternatively checkout this approach `clang-format -i {src,tests}/*.{h,cpp}`

## How-to

* Debug and Run
    * Visual Studio 2022
        * Customize debug/launch arguments
            * Select configuration
            * Select target
            * Select `Debug -> Debug and Launch Settings For <Target>` from menu
            * Add `"args": []` section in the same level where located attribute `name` and provide desired command line arguments
    * Visual Studio Code
        * Install `CMake Tools` extension
        * From Command Palette
            * Select `CMake: Set Launch/Debug Target`
            * Select `CMake: Debug`
        * If you need to pass custom command line argument to debugger check [this](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/debug-launch.md) documentation.
        * Method 1
            * Activate `Run and Debug` in the Sidebar
            * Click `create a launch.json file` and select `C++ (GDB)/(LLDB)`
            * Copy paste desired settings from section `Debug using a launch.json file` from the link above.
                * If you debug with `msvc` use that setting
            * Add custom arguments into `"args": [],`
        * Method 2
            * Create `settings.json` file under the `vscode` directory
            * Consider adding this

                ```json
                {
                    "cmake.debugConfig": {
                        "args": ["command arguments"]
                    }
                }
                ```

* Profiling
    * Linux
        * [See usage of perf](https://stackoverflow.com/questions/2229336/linux-application-profiling) or gemini
