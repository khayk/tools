file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
   "config/*.h"
   "config/*.cpp"
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

include_directories(".")

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")

# Spdlog
# set(SPDLOG_COMPILED_LIB true)
find_package(spdlog REQUIRED)

# Boost
find_package(Boost REQUIRED)


if(Boost_FOUND)
   message("Boost found        : ${Boost_INCLUDE_DIRS}")
   include_directories(${Boost_INCLUDE_DIRS})
endif()

# find_package(SQLite3 REQUIRED)
# find_package(SqliteOrm REQUIRED)

if(SqliteOrm_FOUND)
   message("SqliteOrm found    : ${SqliteOrm_INCLUDE_DIRS}")
endif()

message("Spdlog include     : ${spdlog_DIR}")
message("Boost include      : ${Boost_INCLUDE_DIRS}")
message("Boost library      : ${Boost_LIBRARY_DIRS}")
# message("Sqlite3 library    : ${SQLite3_LIBRARY}")

# Adding a library project
add_library(${projectLib} ${LIB_SRCS})
target_link_libraries(${projectLib} PUBLIC  
    spdlog::spdlog 
    core
)

# Adding executable project
add_executable(${projectApp} Main.cpp)
target_link_libraries (${projectApp} PRIVATE ${projectLib})
# target_link_libraries(${projectApp} sqlite_orm::sqlite_orm)
# target_link_libraries(${projectApp} PRIVATE ${SQLite3_LIBRARY})
# target_link_libraries(${projectApp} PRIVATE ${OPENSSL_LIBRARIES})


if(MSVC)
   ADD_CUSTOM_COMMAND(
      TARGET ${projectApp}
 		POST_BUILD
 		COMMAND "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\os\\win\\manifest\\app.manifest\" -inputresource:\"$<TARGET_FILE:${projectApp}>\"\;\#1 -outputresource:\"$<TARGET_FILE:${projectApp}>\"\;\#1
 		COMMENT "Adding display aware manifest..."
   )
else()
endif(MSVC)
