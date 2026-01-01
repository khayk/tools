project(core VERSION 0.0.1)

set(PROJECT_LIB  ${PROJECT_NAME})

if(WIN32)
    add_compile_definitions(PSAPI_VERSION=1)
endif()

file(GLOB NETWORK_FILES
    "network/data/*.cpp"
    "network/data/*.h"
    "network/*.cpp"
    "network/*.h"
)

file(GLOB PATTERNS_FILES
    "patterns/*.cpp"
    "patterns/*.h"
)

file(GLOB UTILS_FILES
    "utils/*.cpp"
    "utils/*.h"
)

set(SPDLOG_COMPILED_LIB true)
find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS iostreams)

if(Boost_FOUND)
    message("Boost include         : ${Boost_INCLUDE_DIRS}")
    message("Boost library         : ${Boost_LIBRARY_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

message("fmt library           : fmt::fmt")
message("OpenSsl library       : ${OPENSSL_LIBRARIES}")
message("Spdlog include        : ${spdlog_DIR}")

# Source list for the library
set(LIB_SRCS  ${NETWORK_FILES} ${PATTERNS_FILES} ${UTILS_FILES})
add_library(${PROJECT_LIB} STATIC ${LIB_SRCS})
target_include_directories(${PROJECT_LIB} PUBLIC include)

target_link_libraries(${PROJECT_LIB} PUBLIC
    OpenSSL::Crypto
    spdlog::spdlog
    Boost::iostreams
)

# For code coverage
InstrumentForCoverage(${PROJECT_LIB})
CleanCoverage(${PROJECT_LIB})

AddClangTidy(${PROJECT_LIB})