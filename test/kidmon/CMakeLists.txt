set(PROJECT_TST kidmon-test)

file(GLOB_RECURSE TEST_SRCS
   "*.cpp"
   "common/*.cpp"
   "repo/*.cpp"
)

add_executable(${PROJECT_TST} ${TEST_SRCS})
target_link_libraries(${PROJECT_TST} PRIVATE kidmon)

AddTests(${PROJECT_TST})
