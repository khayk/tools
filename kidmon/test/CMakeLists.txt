set(PROJECT_TST kidmon-test)

set(TEST_SRCS
   common/TracerTest.cpp
   common/UtilsTest.cpp
   data/MessagesTest.cpp
   os/ApiTest.cpp
   repo/AsyncRepositoryTest.cpp
   repo/FileSystemRepositoryTest.cpp
   repo/RawFileRangeTest.cpp
   server/AgentManagerTest.cpp
)

add_executable(${PROJECT_TST} ${TEST_SRCS})
target_link_libraries(${PROJECT_TST} PRIVATE kidmon)

# RawFileRangeTest exercises an internal header that lives under src/ (it is not
# part of the library's public include surface).
target_include_directories(${PROJECT_TST} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# For code coverage
InstrumentForCoverage(${PROJECT_TST})
CleanCoverage(${PROJECT_TST})

AddTests(${PROJECT_TST})
