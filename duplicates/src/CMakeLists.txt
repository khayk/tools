file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
)

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")


# Adding a library project
add_library(${PROJECT_LIB} ${LIB_SRCS})
target_link_libraries(${PROJECT_LIB} PUBLIC core)

# Adding executable project
add_executable(${PROJECT_APP} Main.cpp)
target_link_libraries (${PROJECT_APP} ${PROJECT_LIB})

