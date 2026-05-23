set(PROJECT_TST "duplicates-test")

set(TEST_SRCS
    CmdLineTest.cpp
    ConfigTest.cpp
    DeletionStrategyTest.cpp
    DuplicateDeletionTest.cpp
    DuplicateDetectorTest.cpp
    DuplicateOperationTest.cpp
    MenuTest.cpp
    NodeTest.cpp
    UtilsTest.cpp
)

add_executable(${PROJECT_TST} ${TEST_SRCS})

target_link_libraries(${PROJECT_TST} PRIVATE duplicates)

InstrumentForCoverage(${PROJECT_TST})
CleanCoverage(${PROJECT_TST})

AddTests(${PROJECT_TST})
