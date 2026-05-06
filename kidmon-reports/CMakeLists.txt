project(kidmon-reports VERSION 0.0.1)

set(PROJECT_LIB "${PROJECT_NAME}")
set(PROJECT_APP "${PROJECT_NAME}-app")

file(GLOB APP_SRCS
   "src/condition/*.h"
   "src/condition/*.cpp"
   "src/transform/*.h"
   "src/transform/*.cpp"
   "src/*.h"
   "src/*.cpp"
)

find_package(cxxopts CONFIG REQUIRED)

add_executable(${PROJECT_APP} ${APP_SRCS})
target_link_libraries (${PROJECT_APP} PRIVATE kidmon cxxopts::cxxopts)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()