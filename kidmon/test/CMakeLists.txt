set(PROJECT_TST kidmon-test)

set(TEST_SRCS
   common/TracerTest.cpp
   common/UtilsTest.cpp
   data/MessagesTest.cpp
   os/ApiTest.cpp
   repo/AsyncRepositoryTest.cpp
   repo/FileSystemRepositoryTest.cpp
   server/AgentManagerTest.cpp
)

add_executable(${PROJECT_TST} ${TEST_SRCS})
target_link_libraries(${PROJECT_TST} PRIVATE kidmon)

# For code coverage
InstrumentForCoverage(${PROJECT_TST})
CleanCoverage(${PROJECT_TST})

AddTests(${PROJECT_TST})
