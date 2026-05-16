project(duplicates)

set(PROJECT_LIB "${PROJECT_NAME}")
set(PROJECT_APP "${PROJECT_NAME}-app")

file(GLOB LIB_SRCS
   "src/*.h"
   "src/*.cpp"
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")

# Adding a library project
add_library(${PROJECT_LIB} ${LIB_SRCS})
target_include_directories(${PROJECT_LIB} PUBLIC include)
target_link_libraries(${PROJECT_LIB} PUBLIC core tomlplusplus::tomlplusplus)

# For code coverage
InstrumentForCoverage(${PROJECT_LIB})
CleanCoverage(${PROJECT_LIB})

# Adding executable project
add_executable(${PROJECT_APP} src/Main.cpp)
BuildInfo(${PROJECT_APP})
target_link_libraries (${PROJECT_APP} ${PROJECT_LIB})

install(
    TARGETS ${PROJECT_APP}
    DESTINATION bin
    RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
    POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)

AddClangTidy(${PROJECT_LIB})
AddClangTidy(${PROJECT_APP})

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
