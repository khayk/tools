file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
)

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")


# Adding a library project
add_library(${projectLib} ${LIB_SRCS})
target_link_libraries(${projectLib} PUBLIC core)

# Adding executable project
add_executable(${projectApp} Main.cpp)
target_link_libraries (${projectApp} ${projectLib})

