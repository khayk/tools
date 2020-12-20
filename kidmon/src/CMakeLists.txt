file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
   "common/*.h"
   "common/*.cpp"
)

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")

set(SPDLOG_COMPILED_LIB true)
find_package(spdlog REQUIRED)

message( "Include dirs   : ${spdlog_DIR}")

# Adding a library project
add_library(${projectLib} ${LIB_SRCS})

target_include_directories(${projectLib} INTERFACE "${spdlog_DIR}/../../include")
target_link_libraries(${projectLib} PRIVATE spdlog::spdlog)

# Adding executable project
add_executable(${projectApp} Main.cpp)
target_link_libraries (${projectApp} ${projectLib})