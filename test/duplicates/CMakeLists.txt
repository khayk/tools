set(PROJECT_TST "duplicates-test")

file(GLOB TEST_SRCS
   "*.cpp"
)

add_executable(${PROJECT_TST} ${TEST_SRCS})

target_link_libraries(${PROJECT_TST} PRIVATE duplicates)

AddTests(${PROJECT_TST})
