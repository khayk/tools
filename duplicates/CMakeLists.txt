project(duplicates CXX)

file(GLOB PROJ_FILES
   "src/*.h"
   "src/*.cpp"
)

file(GLOB PROJ_TEST_FILES
   "test/*.h"
   "test/*.cpp"
)

find_package(OpenSSL REQUIRED)

message("Project binary dir:  " ${PROJECT_BINARY_DIR})
message("CMake binary dir:    " ${CMAKE_BINARY_DIR})
message("CMake source dir:    " ${CMAKE_SOURCE_DIR})
message("Object dir:          " ${OBJECT_DIR})

include_directories(src)

add_executable(${PROJECT_NAME} ${PROJ_FILES} ${PROJ_TEST_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})

