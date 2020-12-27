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
   # Windows
   "os/win/*.h"
   "os/win/*.cpp"
elseif(APPLE)
   # Apple
   "os/mac/*.h"
   "os/mac/*.cpp"
else(UNIX AND NOT APPLE)
   # Linux, BSD, Solaris, Minix
   "os/linux/*.h"
   "os/linux/*.cpp"
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


if(MSVC)
   ADD_CUSTOM_COMMAND(
      TARGET ${projectApp}
 		POST_BUILD
 		COMMAND "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\os\\win\\manifest\\app.manifest\" -inputresource:\"$<TARGET_FILE:${projectApp}>\"\;\#1 -outputresource:\"$<TARGET_FILE:${projectApp}>\"\;\#1
 		COMMENT "Adding display aware manifest..."
   )
else()
endif(MSVC)

