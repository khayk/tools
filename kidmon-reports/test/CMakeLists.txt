set(PROJECT_TST kidmon-reports-test)

set(TEST_SRCS
   OptionsTest.cpp
   QueryBuilderTest.cpp
   QueryVisualizerTest.cpp
)

add_executable(${PROJECT_TST} ${TEST_SRCS})
target_link_libraries(${PROJECT_TST} PRIVATE kidmon-reports)

# For code coverage
InstrumentForCoverage(${PROJECT_TST})
CleanCoverage(${PROJECT_TST})

AddTests(${PROJECT_TST})
