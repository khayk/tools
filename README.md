# Tools

Own implementation of different tools.

## Build & Test

### Prerequisite

1. Install `vcpkg`
2. Install libraries
    * `./vcpkg install gtest spdlog openssl boost-asio nlohmann-json cxxopts --triplet=<platform>`
        * Windows `<platform>` can be `x64-windows`
        * Linux   `<platform>` can be `x64-linux`

### Instructions

* Configure
    * Navigate the root directory of the project
    * If the environment variable `VCPKG_ROOT` is defined, use the command below
        * `cmake -B build -S .`
    * Otherwise
        * `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg_path>/scripts/buildsystems/vcpkg.cmake`
    * Additionally build type can be specified as below
        * `-DCMAKE_BUILD_TYPE=Release`
* Build
    * `cmake --build build --parallel 8 --config <cfg>`
* Extras
    * Run the command below before configure step to build with `clang`. Change clang path if required.

        ```bash
        export CC=/usr/bin/clang
        export CXX=/usr/bin/clang++
        ```

* Tests
    * TBD
    * Manually run application with ending with name `-test` (TBD, utilize `ctest`)
        * Windows - `build\core\test\Debug\core-test.exe`
        * Linux - `./build/core/test/core-test`
    * [Creating and running tests with CTest](https://coderefinery.github.io/cmake-workshop/testing/)
* Install
    * `cmake --install build --prefix ./ --config <cfg>`

## Coverage

### Windows

* Install OpenCppCoverage
* `OpenCppCoverage.exe --sources tools\core --modules test.exe --export_type=html:.reports/core/  -- out\build\x64-Debug\core\core-test.exe`
* `OpenCppCoverage.exe --sources tools\kidmon --modules test.exe --export_type=html:.reports/kidmon/  -- out\build\x64-Debug\kidmon\test\kidmon-test.exe`
* TBD - the command line above is for reference only, later create a script to produce coverage

## Formatting

* The project is formatted based on the settings in `.clang-format`
    * Open bash, navigate the root directory of the project
    * Ensure `clang-format` version `17.0.1` is installed
    * Run `find . -iname *.h -o -iname *.cpp | xargs clang-format -i`, to format all `{h,cpp}` files in place
