# Tools

Own implementation of different tools.

## Build

### Prerequisite

1. Install `vcpkg`
2. Install libraries
    * `./vcpkg install gtest spdlog openssl boost-asio nlohmann-json cxxopts --triplet=<platform>`
        * Windows `<platform>` can be `x64-windows`
        * Linux   `<platform>` can be `x64-linux`

### Windows

* TODO

### Linux

1. Configure from the project root directory
    * `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg_path>/scripts/buildsystems/vcpkg.cmake`
2. Build
    * `cmake --build build --parallel 4`

* If you want to build with `clang` you can run following command before configuring the project. Change clang path if required.

    ```bash
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    ```

## Coverage

### Windows

* Install OpenCppCoverage
* `OpenCppCoverage.exe --sources tools\core --modules test.exe --export_type=html:.reports/core/  -- out\build\x64-Debug\core\core-test.exe`
* `OpenCppCoverage.exe --sources tools\kidmon --modules test.exe --export_type=html:.reports/kidmon/  -- out\build\x64-Debug\kidmon\test\kidmon-test.exe`
* TBD - the command line above is for reference only, later create a script to produce coverage

## Formatting

* The project formatted based on the settings in .clang-format file under the project root directory
    * Open bash, navigate to project root directory
    * Ensure you have installed `clang-format` version `17.0.1`
    * Run `find . -iname *.h -o -iname *.cpp | xargs clang-format -i`, to format all {h,cpp} files in place
