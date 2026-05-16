project(kidmon-reports)

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
BuildInfo(${PROJECT_APP})
target_link_libraries (${PROJECT_APP} PRIVATE kidmon cxxopts::cxxopts)

install(
    TARGETS ${PROJECT_APP}
    DESTINATION bin
    RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
    POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()