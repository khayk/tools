project(core VERSION 0.0.1)

set(PROJECT_LIB  ${PROJECT_NAME})
set(PROJECT_TST ${PROJECT_NAME}-test)

add_subdirectory(src)

if(BUILD_TESTS)
    add_subdirectory(test)
    enable_testing()
endif()
