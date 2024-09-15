file(GLOB_RECURSE TEST_SRCS
   "*.cpp"
   "common/*.cpp"
   "repo/*.cpp"
)

find_package(GTest REQUIRED)
add_executable(${PROJECT_TST} ${TEST_SRCS})

target_link_libraries(${PROJECT_TST} PRIVATE
	GTest::gmock_main
	${GTEST_BOTH_LIBRARIES}
	${PROJECT_LIB}
)

enable_testing()
gtest_discover_tests(${PROJECT_TST})
