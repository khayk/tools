file(GLOB APP_SRCS
   "*.h"
   "*.cpp"
)

find_package(cxxopts CONFIG REQUIRED)

add_executable(${PROJECT_APP} ${APP_SRCS})
target_link_libraries (${PROJECT_APP} PRIVATE kidmon cxxopts::cxxopts)
