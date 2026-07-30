project(kidmon)

set(PROJECT_LIB "${PROJECT_NAME}")
set(PROJECT_APP "${PROJECT_NAME}-app")

# Sources are listed explicitly (not globbed) so adding/removing a file is a
# CMakeLists edit that reliably triggers a reconfigure. Main.cpp is built into
# the app target below, not the library.
set(LIB_SRCS
   src/agent/KidmonAgent.cpp
   src/common/Enums.cpp
   src/common/Utils.cpp
   src/config/Config.cpp
   src/data/Messages.cpp
   src/data/Types.cpp
   src/repo/AsyncRepository.cpp
   src/repo/FileSystemRepository.cpp
   src/server/AgentConnection.cpp
   src/server/AgentManager.cpp
   src/server/KidmonServer.cpp
   src/server/handler/AuthorizationHandler.cpp
   src/server/handler/DataHandler.cpp
)

if(WIN32)
   # Windows
   list(APPEND LIB_SRCS
      src/os/win/Api.cpp
      src/os/win/Api.h
      src/os/win/GuiUtils.h
      src/os/win/ProcessLauncher.cpp
      src/os/win/ProcessLauncher.h
      src/os/win/Window.cpp
      src/os/win/Window.h
   )
elseif (APPLE)
    # macOS
    list(APPEND LIB_SRCS
       src/os/mac/Api.cpp
       src/os/mac/Api.h
       src/os/mac/AppPath.cpp
       src/os/mac/AppPath.h
       src/os/mac/ProcessLauncher.cpp
       src/os/mac/ProcessLauncher.h
       src/os/mac/Window.cpp
       src/os/mac/Window.h
    )
elseif(UNIX)
   # Linux, BSD, Solaris, Minix
   list(APPEND LIB_SRCS
      src/os/linux/Api.cpp
      src/os/linux/Api.h
      src/os/linux/ProcessLauncher.cpp
      src/os/linux/ProcessLauncher.h
      src/os/linux/Window.cpp
      src/os/linux/Window.h
   )
endif()

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
        # LaunchServices, for locating an application by its bundle identifier.
        "-framework CoreServices"
    )
endif()

if(WIN32)
    # PowrProf provides CallNtPowerInformation (display-on execution-state query).
    target_link_libraries(${PROJECT_LIB} PRIVATE PowrProf)
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
    set(APP_BINARY "$<TARGET_FILE:${PROJECT_APP}>")
    set(APP_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/src/os/win/manifest/app.manifest")

    # mt.exe requires each option and its value to be a SINGLE token
    # (-inputresource:VALUE, no space). With VERBATIM, every argument on its own
    # line becomes a separate token, so the option and path must be joined.
    add_custom_command(
        TARGET  ${PROJECT_APP}
        POST_BUILD
        COMMAND mt.exe
            -manifest "${APP_MANIFEST}"
            "-inputresource:${APP_BINARY};#1"
            "-outputresource:${APP_BINARY};#1"
        COMMENT "Embedding DPI-aware manifest..."
        VERBATIM
    )
endif()

if(APPLE)
    # Build kidmon-app as a .app bundle so it has a stable bundle identifier.
    # macOS TCC (Screen Recording, etc.) keys its permission grants on a code
    # signature / bundle id, so a bare CLI binary can never appear in the
    # Privacy list by its own name -- only a signed bundle can.
    set(KIDMON_CODESIGN_IDENTITY "Kidmon Self-Signed"
        CACHE STRING "Code-signing identity used to sign the macOS bundle")
    set(KIDMON_BUNDLE_ID "com.kidmon.app"
        CACHE STRING "macOS bundle identifier for kidmon-app")

    set_target_properties(${PROJECT_APP} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/os/mac/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "Kidmon"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${KIDMON_BUNDLE_ID}"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )

    # Sign after every build of the app. The helper warns (and exits 0) if the
    # identity is missing, so a fresh checkout still builds without a cert.
    add_custom_command(
        TARGET ${PROJECT_APP}
        POST_BUILD
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/codesign-macos.sh"
                "${KIDMON_CODESIGN_IDENTITY}"
                "$<TARGET_BUNDLE_DIR:${PROJECT_APP}>"
                "${KIDMON_BUNDLE_ID}"
        COMMENT "Code-signing kidmon-app.app..."
        VERBATIM
    )
endif(APPLE)

if(APPLE)
    # Into bin/ so it sits alongside the other tools and the release workflow
    # (which packages staging/bin) picks up kidmon-app.app.
    install(
        TARGETS ${PROJECT_APP}
        BUNDLE DESTINATION bin
    )
else()
    install(
        TARGETS ${PROJECT_APP}
        DESTINATION bin
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()