file(GLOB LIB_SRCS
   "*.h"
   "*.cpp"
   "config/*.h"
   "config/*.cpp"
   "common/*.h"
   "common/*.cpp"
   "geometry/*.h"
   "geometry/*.cpp"
   "network/*.h"
   "network/*.cpp"
   "os/*.h"
   "os/*.cpp"
)

if(WIN32)
   # Windows
   file(GLOB OS_WIN
      "os/win/*.h"
      "os/win/*.cpp"
   )
   list(APPEND LIB_SRCS ${OS_WIN})
elseif(UNIX AND NOT APPLE)
   # Linux, BSD, Solaris, Minix
   file(GLOB OS_LINUX
      "os/linux/*.h"
      "os/linux/*.cpp"
   )
   list(APPEND LIB_SRCS ${OS_LINUX})
endif()

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
add_library(${PROJECT_LIB} ${LIB_SRCS})

message("${PROJECT_NAME} sources     : ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_LIB} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_LIB} PUBLIC  
    spdlog::spdlog 
    core
)


# Adding executable project
add_executable(${PROJECT_APP} Main.cpp)
target_link_libraries (${PROJECT_APP} PRIVATE ${PROJECT_LIB})
# target_link_libraries(${PROJECT_APP} sqlite_orm::sqlite_orm)
# target_link_libraries(${PROJECT_APP} PRIVATE ${SQLite3_LIBRARY})
# target_link_libraries(${PROJECT_APP} PRIVATE ${OPENSSL_LIBRARIES})


if(MSVC)
   ADD_CUSTOM_COMMAND(
      TARGET ${PROJECT_APP}
 		POST_BUILD
 		COMMAND "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\os\\win\\manifest\\app.manifest\" -inputresource:\"$<TARGET_FILE:${PROJECT_APP}>\"\;\#1 -outputresource:\"$<TARGET_FILE:${PROJECT_APP}>\"\;\#1
 		COMMENT "Adding display aware manifest..."
   )
else()
endif(MSVC)
