project(kidmon-reports)

set(PROJECT_LIB "${PROJECT_NAME}")
set(PROJECT_APP "${PROJECT_NAME}-app")

# Sources are listed explicitly (not globbed) so adding/removing a file is a
# CMakeLists edit that reliably triggers a reconfigure. Main.cpp is built into
# the app target below, not the library.
set(LIB_SRCS
   src/aggregate/Aggregate.h
   src/aggregate/Predicate.h
   src/condition/Conditions.cpp
   src/condition/Conditions.h
   src/condition/ICondition.h
   src/transform/ITransform.h
   src/transform/Transforms.cpp
   src/transform/Transforms.h
   src/Options.cpp
   src/Options.h
   src/QueryBuilder.cpp
   src/QueryBuilder.h
   src/QueryVisualizer.cpp
   src/QueryVisualizer.h
)

find_package(cxxopts CONFIG REQUIRED)

add_library(${PROJECT_LIB} ${LIB_SRCS})
target_include_directories(${PROJECT_LIB} PUBLIC src)
target_link_libraries(${PROJECT_LIB} PUBLIC kidmon cxxopts::cxxopts)

add_executable(${PROJECT_APP} src/Main.cpp)
BuildInfo(${PROJECT_APP})
target_link_libraries (${PROJECT_APP} PRIVATE ${PROJECT_LIB})

# For code coverage
InstrumentForCoverage(${PROJECT_LIB})
CleanCoverage(${PROJECT_LIB})

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
