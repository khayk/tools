project(kidmon VERSION 0.0.1)

set(PROJECT_LIB "${PROJECT_NAME}")
set(PROJECT_APP "${PROJECT_NAME}-app")
set(PROJECT_TST "${PROJECT_NAME}-test")

if(MSVC)
    add_compile_definitions(PSAPI_VERSION=1)
endif()

add_subdirectory(src)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
