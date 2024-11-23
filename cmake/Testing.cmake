
# include(GoogleTest)
# include(Coverage)
# include(Memcheck)

find_package(GTest REQUIRED)
message("Gtest libraries       : ${GTEST_BOTH_LIBRARIES}")

macro(AddTests target)
   message("Adding tests          : ${target}")
   target_link_libraries(${target} PRIVATE GTest::gmock_main)
   gtest_discover_tests(${target})

   #  AddCoverage(${target})
   #  AddMemcheck(${target})
endmacro()