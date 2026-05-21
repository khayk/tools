set(PROJECT_TST core-test)

set(NETWORK_FILES
    network/data/PackerTest.cpp
    network/data/UnpackerTest.cpp
    network/TcpClientTest.cpp
    network/TcpCommunicatorTest.cpp
    network/TcpConnectionTest.cpp
    network/TcpServerTest.cpp
)

set(UTILS_FILES
    utils/CryptoTest.cpp
    utils/DirsTest.cpp
    utils/FileTest.cpp
    utils/FmtExtTest.cpp
    utils/LogTest.cpp
    utils/NumberTest.cpp
    utils/SingleInstanceCheckerTest.cpp
    utils/StopWatchTest.cpp
    utils/StrTest.cpp
    utils/SysTest.cpp
    utils/ThrowTest.cpp
    utils/TracerTest.cpp
)

set(PATTERNS_FILES
    patterns/CallbackTest.cpp
    patterns/ObserverTest.cpp
)

set(TEST_SRCS ${NETWORK_FILES} ${UTILS_FILES} ${PATTERNS_FILES})
add_executable(${PROJECT_TST} ${TEST_SRCS})

target_link_libraries(${PROJECT_TST} PRIVATE core)
target_include_directories(${PROJECT_TST} PRIVATE ${PROJECT_SOURCE_DIR}/src)

# # For code coverage
InstrumentForCoverage(${PROJECT_TST})
CleanCoverage(${PROJECT_TST})

AddTests(${PROJECT_TST})

