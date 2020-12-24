file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
   "common/*.h"
   "common/*.cpp"
   "geometry/*.h"
   "geometry/*.cpp"
   "os/*.h"
   "os/*.cpp"
if(WIN32)
   "os/win/*.h"
   "os/win/*.cpp"
elseif(APPLE)
   "os/mac/*.h"
   "os/mac/*.cpp"
endif()
)

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")

set(SPDLOG_COMPILED_LIB true)
find_package(spdlog REQUIRED)

find_package(boost REQUIRED)

if(Boost_FOUND)
   message("Boost found: ${Boost_INCLUDE_DIRS}")
   include_directories(${Boost_INCLUDE_DIRS})
endif()

message("Spdlog Include dirs : ${spdlog_DIR}")
message("Boost include dirs  : ${Boost_INCLUDE_DIRS}")
message("Boost library dirs  : ${Boost_LIBRARY_DIRS}")

# Adding a library project
add_library(${projectLib} ${LIB_SRCS})

target_include_directories(${projectLib} INTERFACE "${spdlog_DIR}/../../include")
target_link_libraries(${projectLib} PRIVATE spdlog::spdlog)

# Adding executable project
add_executable(${projectApp} Main.cpp)
target_link_libraries (${projectApp} ${projectLib})
