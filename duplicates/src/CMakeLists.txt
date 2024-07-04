file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
)

add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
add_compile_definitions(_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")

# Adding a library project
add_library(${PROJECT_LIB} ${LIB_SRCS})
target_include_directories(${PROJECT_LIB} PUBLIC ../include)
target_link_libraries(${PROJECT_LIB} PUBLIC core)

# Adding executable project
add_executable(${PROJECT_APP} Main.cpp)
target_link_libraries (${PROJECT_APP} ${PROJECT_LIB})

