project(core)

set(PROJECT_LIB  ${PROJECT_NAME})

if(WIN32)
    add_compile_definitions(PSAPI_VERSION=1)
endif()

set(NETWORK_FILES
    src/network/data/Packer.cpp
    src/network/data/StringSource.cpp
    src/network/data/Unpacker.cpp
    src/network/TcpClient.cpp
    src/network/TcpCommunicator.cpp
    src/network/TcpConnection.cpp
    src/network/TcpServer.cpp
)

set(PATTERNS_FILES
    # header-only, no .cpp sources
)

set(APP_FILES
    src/app/Console.cpp
)

if(WIN32)
    list(APPEND APP_FILES src/app/os/ServiceWin32.cpp)
elseif(APPLE)
    list(APPEND APP_FILES src/app/os/ServiceMacOS.cpp)
else()
    list(APPEND APP_FILES src/app/os/ServiceLinux.cpp)
endif()

set(UTILS_FILES
    src/utils/Crypto.cpp
    src/utils/Dirs.cpp
    src/utils/File.cpp
    src/utils/FileDetail.h
    src/utils/Log.cpp
    src/utils/MemoryMonitor.cpp
    src/utils/SingleInstanceChecker.cpp
    src/utils/StopWatch.cpp
    src/utils/Str.cpp
    src/utils/Sys.cpp
    src/utils/Throw.cpp
    src/utils/Tracer.cpp
)

if(WIN32)
    list(APPEND UTILS_FILES
        src/utils/os/DirsWin32.cpp
        src/utils/os/SysWin32.cpp
        src/utils/os/SingleInstanceCheckerWin32.cpp
    )
else()
    list(APPEND UTILS_FILES
        src/utils/os/DirsPosix.cpp
        src/utils/os/SysPosix.cpp
    )
    if(APPLE)
        list(APPEND UTILS_FILES
            src/utils/os/DirsMacOS.cpp
            src/utils/os/SysMacOS.cpp
            src/utils/os/SingleInstanceCheckerMacOS.cpp
        )
    else()
        list(APPEND UTILS_FILES
            src/utils/os/DirsLinux.cpp
            src/utils/os/SysLinux.cpp
            src/utils/os/SingleInstanceCheckerLinux.cpp
        )
    endif()
endif()

# set(SPDLOG_COMPILED_LIB true)
find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS iostreams)

if(Boost_FOUND)
    message("Boost include         : ${Boost_INCLUDE_DIRS}")
    message("Boost library         : ${Boost_LIBRARY_DIRS}")
endif()

message("fmt library           : std::fmt")
message("OpenSSL library       : ${OPENSSL_LIBRARIES}")
message("Spdlog include        : ${spdlog_DIR}")

# Source list for the library
set(LIB_SRCS  ${NETWORK_FILES} ${PATTERNS_FILES} ${UTILS_FILES} ${APP_FILES})
add_library(${PROJECT_LIB} STATIC ${LIB_SRCS})
add_library(${PROJECT_LIB}::${PROJECT_LIB} ALIAS ${PROJECT_LIB})

target_include_directories(${PROJECT_LIB}
    PUBLIC  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}   # for internal headers
)

target_link_libraries(${PROJECT_LIB} PUBLIC
    OpenSSL::Crypto
    spdlog::spdlog
    Boost::iostreams
)

if(WIN32)
    target_link_libraries(${PROJECT_LIB} PUBLIC psapi Wtsapi32)
endif()

if(APPLE)
    target_link_libraries(${PROJECT_LIB} PUBLIC
        "-framework CoreFoundation"
        "-framework SystemConfiguration"
    )
endif()

# For code coverage
InstrumentForCoverage(${PROJECT_LIB})
CleanCoverage(${PROJECT_LIB})

AddClangTidy(${PROJECT_LIB})

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
