# Tools

Own implementation of different tools. Feel free to use if you find them useful.

## Build

### Prerequisite

1. Install `vcpkg`
2. Install libraries 
    * Windows `./vcpkg install gtest spdlog openssl boost-asio --triplet=x64-windows`
    * Linux   `./vcpkg install gtest spdlog openssl boost-asio --triplet=x64-linux`

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
