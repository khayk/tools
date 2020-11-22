project(duplicates CXX)

file(GLOB PROJ_FILES
   "src/*.h"
   "src/*.cpp"
)

file(GLOB PROJ_TEST_FILES
   "test/*.h"
   "test/*.cpp"
)

find_path(OpenSSL_INCLUDE_DIR openssl/sha.h)
find_library(LIBSSL_LIBRARY libssl)
find_library(LIBCRYPTOL_LIBRARY libcrypto)

include_directories(${OpenSSL_INCLUDE_DIR})

message("Project binary dir:  " ${PROJECT_BINARY_DIR})
message("CMake binary dir:    " ${CMAKE_BINARY_DIR})
message("CMake source dir:    " ${CMAKE_SOURCE_DIR})
message("Object dir:          " ${OBJECT_DIR})

include_directories(src)

add_executable(${PROJECT_NAME} ${PROJ_FILES} ${PROJ_TEST_FILES})

target_link_libraries(${PROJECT_NAME} ${LIBSSL_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${LIBCRYPTOL_LIBRARY})

