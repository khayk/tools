function(AddCoverage target)
	if(TOOLS_COVERAGE)
      find_program(LCOV_PATH lcov REQUIRED)
      find_program(GENHTML_PATH genhtml REQUIRED)

      string(FIND "${target}" "-test" pos)
      string(SUBSTRING "${target}" 0 ${pos} substr)
      message(STATUS "Coverage filter    : /src/${substr}/*")

      add_custom_target(coverage-${target}
         COMMENT "Running coverage for ${target}..."
         COMMAND ${LCOV_PATH} -d . --zerocounters
         COMMAND $<TARGET_FILE:${target}>
         COMMAND ${LCOV_PATH} -d .
                              --ignore-errors mismatch
                              --capture -o coverage.info
         COMMAND ${LCOV_PATH} -e coverage.info '/src/${substr}/*'
                              # -r coverage.info '/usr/include/*'
                              # -r coverage.info '/build/*'

                              -o filtered.info
         COMMAND ${GENHTML_PATH} -o coverage-${target} filtered.info --legend
         COMMAND rm -rf coverage.info filtered.info
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
	endif()
endfunction()

function(CleanCoverage target)
	if(TOOLS_COVERAGE)
      add_custom_command(TARGET ${target} PRE_BUILD COMMAND
                        find ${CMAKE_BINARY_DIR} -type f
                        -name '*.gcda' -exec cmake -E rm {} +)
	endif()
endfunction()

function(InstrumentForCoverage target)
	if(TOOLS_COVERAGE)
      if (CMAKE_BUILD_TYPE STREQUAL Debug)
         target_compile_options(
            ${target}
            PRIVATE --coverage -fno-inline
                  -fno-inline-small-functions
                  -fno-default-inline
         )
         target_link_options(${target} PUBLIC --coverage)
      endif()
   endif()
endfunction()
