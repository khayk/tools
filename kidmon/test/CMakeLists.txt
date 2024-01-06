file(GLOB_RECURSE TEST_SRCS
   "*.cpp"
)

find_package(GTest REQUIRED)
add_executable(${PROJECT_TST} ${TEST_SRCS})

target_link_libraries(${PROJECT_TST} PRIVATE
	GTest::gmock_main
	${GTEST_BOTH_LIBRARIES}
	${PROJECT_LIB}
)

gtest_discover_tests(${PROJECT_TST})
