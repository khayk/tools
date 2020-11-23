
file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
)

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")

find_package(OpenSSL REQUIRED)

# Adding a library project
add_library(${projectLib} ${LIB_SRCS})
target_include_directories(${projectLib} PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${projectLib} PRIVATE ${OPENSSL_LIBRARIES})

# Adding executable project
add_executable(${projectApp} Main.cpp)
target_link_libraries (${projectApp} ${projectLib})

