function(AddCoverage target)
	if(TOOLS_COVERAGE)
      find_program(LCOV_PATH lcov REQUIRED)
      find_program(GENHTML_PATH genhtml REQUIRED)

      string(FIND "${target}" "-test" pos)
      string(SUBSTRING "${target}" 0 ${pos} substr)
      message(STATUS "Coverage filter    : ${CMAKE_SOURCE_DIR}/${substr}/src/*")

      set(LCOV_IGNORE
         --ignore-errors mismatch
         --ignore-errors inconsistent
         --ignore-errors unsupported
         --ignore-errors format
         --ignore-errors unused
      )

      add_custom_target(coverage-${target}
         COMMENT "Running coverage for ${target}..."
         COMMAND ${LCOV_PATH} -d . --zerocounters
         COMMAND ${LCOV_PATH} -d . ${LCOV_IGNORE} --capture --initial -o baseline.info
         COMMAND $<TARGET_FILE:${target}>
         COMMAND ${LCOV_PATH} -d . ${LCOV_IGNORE} --capture -o coverage.info
         COMMAND ${LCOV_PATH} --add-tracefile baseline.info
                              --add-tracefile coverage.info
                              ${LCOV_IGNORE}
                              -o coverage.info
         COMMAND ${LCOV_PATH} --extract coverage.info
                              "${CMAKE_SOURCE_DIR}/${substr}/src/*"
                              ${LCOV_IGNORE}
                              -o filtered.info
         COMMAND ${LCOV_PATH} --remove filtered.info
                              "/usr/*"
                              "*/vcpkg_installed/*"
                              ${LCOV_IGNORE}
                              -o filtered.info
         COMMAND ${GENHTML_PATH} -o coverage-${target} filtered.info --legend
                              --ignore-errors category
                              --ignore-errors inconsistent
                              --ignore-errors corrupt
                              --ignore-errors unsupported
         COMMAND rm -rf baseline.info coverage.info filtered.info
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
                  # -fno-inline-small-functions
                  # -fno-default-inline
         )
         target_link_options(${target} PUBLIC --coverage)
      endif()
   endif()
endfunction()
