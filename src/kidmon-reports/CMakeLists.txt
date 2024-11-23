project(kidmon-reports VERSION 0.0.1)

set(PROJECT_LIB "${PROJECT_NAME}")
set(PROJECT_APP "${PROJECT_NAME}-app")

file(GLOB APP_SRCS
   "condition/*.h"
   "condition/*.cpp"
   "transform/*.h"
   "transform/*.cpp"
   "*.h"
   "*.cpp"
)

find_package(cxxopts CONFIG REQUIRED)

add_executable(${PROJECT_APP} ${APP_SRCS})
target_link_libraries (${PROJECT_APP} PRIVATE kidmon cxxopts::cxxopts)
