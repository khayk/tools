project(core)

set(PROJECT_LIB  ${PROJECT_NAME})

if(WIN32)
    add_compile_definitions(PSAPI_VERSION=1)
endif()

file(GLOB NETWORK_FILES
    "src/network/data/*.cpp"
    "src/network/data/*.h"
    "src/network/*.cpp"
    "src/network/*.h"
)

file(GLOB PATTERNS_FILES
    "src/patterns/*.cpp"
    "src/patterns/*.h"
)

file(GLOB UTILS_FILES
    "src/utils/*.cpp"
    "src/utils/*.h"
)

if(WIN32)
    list(APPEND UTILS_FILES src/utils/os/DirsWin32.cpp)
else()
    list(APPEND UTILS_FILES src/utils/os/DirsPosix.cpp)
    if(APPLE)
        list(APPEND UTILS_FILES src/utils/os/DirsMacOS.cpp)
    else()
        list(APPEND UTILS_FILES src/utils/os/DirsLinux.cpp)
    endif()
endif()

# set(SPDLOG_COMPILED_LIB true)
find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS iostreams)

if(Boost_FOUND)
    message("Boost include         : ${Boost_INCLUDE_DIRS}")
    message("Boost library         : ${Boost_LIBRARY_DIRS}")
endif()

message("fmt library           : std::fmt")
message("OpenSSL library       : ${OPENSSL_LIBRARIES}")
message("Spdlog include        : ${spdlog_DIR}")

# Source list for the library
set(LIB_SRCS  ${NETWORK_FILES} ${PATTERNS_FILES} ${UTILS_FILES})
add_library(${PROJECT_LIB} STATIC ${LIB_SRCS})
add_library(${PROJECT_LIB}::${PROJECT_LIB} ALIAS ${PROJECT_LIB})

target_include_directories(${PROJECT_LIB}
    PUBLIC  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}   # for internal headers
)

target_link_libraries(${PROJECT_LIB} PUBLIC
    OpenSSL::Crypto
    spdlog::spdlog
    Boost::iostreams
)

if(APPLE)
    target_link_libraries(${PROJECT_LIB} PUBLIC
        "-framework CoreFoundation"
        "-framework SystemConfiguration"
    )
endif()

# For code coverage
InstrumentForCoverage(${PROJECT_LIB})
CleanCoverage(${PROJECT_LIB})

AddClangTidy(${PROJECT_LIB})

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
