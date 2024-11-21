file(GLOB_RECURSE TEST_SRCS
   "*.cpp"
   "common/*.cpp"
   "repo/*.cpp"
)

find_package(GTest REQUIRED)
add_executable(${PROJECT_TST} ${TEST_SRCS})

message("Gtest libraries       : ${GTEST_BOTH_LIBRARIES}")

target_link_libraries(${PROJECT_TST} PRIVATE
	${PROJECT_LIB}
	GTest::gmock_main
	${GTEST_BOTH_LIBRARIES}
)

enable_testing()
gtest_discover_tests(${PROJECT_TST})
