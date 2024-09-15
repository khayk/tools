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

# Source list for the library
set(LIB_SRCS  ${NETWORK_FILES} ${PATTERNS_FILES} ${UTILS_FILES})
add_library(${PROJECT_LIB} STATIC ${LIB_SRCS})
target_include_directories(${PROJECT_LIB} PUBLIC ../include)

set(SPDLOG_COMPILED_LIB true)

find_package(fmt REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)

message("fmt library           : fmt::fmt")
message("OpenSsl library       : ${OPENSSL_LIBRARIES}")
message("Spdlog include        : ${spdlog_DIR}")

target_link_libraries(${PROJECT_LIB} PUBLIC
    fmt::fmt
    ${OPENSSL_LIBRARIES}
    spdlog::spdlog
)
