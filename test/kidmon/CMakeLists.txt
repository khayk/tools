set(PROJECT_TST kidmon-test)

file(GLOB_RECURSE TEST_SRCS
   "*.cpp"
   "common/*.cpp"
   "repo/*.cpp"
)

add_executable(${PROJECT_TST} ${TEST_SRCS})
target_link_libraries(${PROJECT_TST} PRIVATE kidmon)

# For code coverage
InstrumentForCoverage(${PROJECT_TST})
CleanCoverage(${PROJECT_TST})

AddTests(${PROJECT_TST})
