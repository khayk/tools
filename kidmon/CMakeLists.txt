project(kidmon)

set(PROJECT_LIB "${PROJECT_NAME}")
set(PROJECT_APP "${PROJECT_NAME}-app")

file(GLOB LIB_SRCS
   "src/*.h"
   "src/*.cpp"
   "src/agent/*.cpp"
   "src/agent/*.h"
   "src/common/*.cpp"
   "src/common/*.h"
   "src/config/*.cpp"
   "src/config/*.h"
   "src/data/*.cpp"
   "src/data/*.h"
   "src/geometry/*.cpp"
   "src/geometry/*.h"
   "src/os/*.cpp"
   "src/os/*.h"
   "src/repo/*.cpp"
   "src/repo/*.h"
   "src/server/*.cpp"
   "src/server/*.h"
   "src/server/handler/*.cpp"
   "src/server/handler/*.h"
)

if(WIN32)
   # Windows
   file(GLOB OS_WIN
      "src/os/win/*.h"
      "src/os/win/*.cpp"
   )
   list(APPEND LIB_SRCS ${OS_WIN})
elseif (APPLE)
    # macOS
    file(GLOB OS_MAC
        "src/os/mac/*.h"
        "src/os/mac/*.cpp"
    )
    list(APPEND LIB_SRCS ${OS_MAC})
elseif(UNIX)
   # Linux, BSD, Solaris, Minix
   file(GLOB OS_LINUX
      "src/os/linux/*.h"
      "src/os/linux/*.cpp"
   )
   list(APPEND LIB_SRCS ${OS_LINUX})
endif()

list(FILTER LIB_SRCS EXCLUDE REGEX ".*Main.cpp")

find_package(nlohmann_json CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)
# find_package(msgpack-c CONFIG REQUIRED)

# find_package(SQLite3 REQUIRED)
# find_package(SqliteOrm REQUIRED)
# if(SqliteOrm_FOUND)
#    message("SqliteOrm found    : ${SqliteOrm_INCLUDE_DIRS}")
# endif()

# message("Sqlite3 library    : ${SQLite3_LIBRARY}")

# Adding a library project
add_library(${PROJECT_LIB} ${LIB_SRCS})

message("${PROJECT_NAME} sources        : ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_LIB} PUBLIC include)

target_link_libraries(${PROJECT_LIB} PUBLIC
    nlohmann_json::nlohmann_json
    core
)

target_link_libraries(${PROJECT_LIB} PRIVATE
    glaze::glaze
)

if(APPLE)
    target_link_libraries(${PROJECT_LIB} PRIVATE
        "-framework CoreGraphics"
        "-framework ImageIO"
    )
endif()

# target_link_libraries(${PROJECT_LIB} PRIVATE
#    msgpack-c
# )

# Adding executable project
add_executable(${PROJECT_APP} src/Main.cpp)
BuildInfo(${PROJECT_APP})
target_link_libraries (${PROJECT_APP} PRIVATE ${PROJECT_LIB} cxxopts::cxxopts)

# target_link_libraries(${PROJECT_APP} sqlite_orm::sqlite_orm)
# target_link_libraries(${PROJECT_APP} PRIVATE ${SQLite3_LIBRARY})

# For code coverage
InstrumentForCoverage(${PROJECT_LIB})
CleanCoverage(${PROJECT_LIB})

AddClangTidy(${PROJECT_LIB})
AddClangTidy(${PROJECT_APP})

if(WIN32)
   ADD_CUSTOM_COMMAND(
      TARGET ${PROJECT_APP}
 		POST_BUILD
 		COMMAND "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\src\\os\\win\\manifest\\app.manifest\" -inputresource:\"$<TARGET_FILE:${PROJECT_APP}>\"\;\#1 -outputresource:\"$<TARGET_FILE:${PROJECT_APP}>\"\;\#1
 		COMMENT "Adding display aware manifest..."
   )
endif(WIN32)

install(
    TARGETS ${PROJECT_APP}
    DESTINATION bin
    RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
    POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()