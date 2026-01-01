project(duplicates VERSION 0.0.1)

set(PROJECT_LIB "${PROJECT_NAME}")
set(PROJECT_APP "${PROJECT_NAME}-app")

file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
)

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)

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
add_executable(${PROJECT_APP} Main.cpp)
target_link_libraries (${PROJECT_APP} ${PROJECT_LIB})

AddClangTidy(${PROJECT_LIB})
AddClangTidy(${PROJECT_APP})
