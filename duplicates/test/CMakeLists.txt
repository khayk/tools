file(GLOB TEST_SRCS
   "*.cpp"
)

find_package(GTest REQUIRED)

add_executable(${projectTest} ${TEST_SRCS})
target_include_directories(${projectTest} PUBLIC ${GTEST_INCLUDE_DIR})
target_link_libraries(${projectTest} PRIVATE ${GTEST_BOTH_LIBRARIES})

gtest_discover_tests(${projectTest})
