set(PROJECT_TST "duplicates-test")

file(GLOB TEST_SRCS
   "*.cpp"
)

add_executable(${PROJECT_TST} ${TEST_SRCS})
# target_include_directories(${PROJECT_TST} PUBLIC ${GTEST_INCLUDE_DIR})
# target_link_libraries(${PROJECT_TST} PRIVATE ${GTEST_BOTH_LIBRARIES})

AddTests(${PROJECT_TST})
