file(GLOB TEST_SRCS
   "*.cpp"
)

find_package(GTest REQUIRED)

add_executable(${PROJECT_TST} ${TEST_SRCS})
target_include_directories(${PROJECT_TST} PUBLIC ${GTEST_INCLUDE_DIR})
target_link_libraries(${PROJECT_TST} PRIVATE ${GTEST_BOTH_LIBRARIES})

enable_testing()
gtest_discover_tests(${PROJECT_TST})
