function(AddClangTidy target)
	if(TOOLS_TIDY)
		find_program(CLANG_TIDY_PATH NAMES "clang-tidy-19" REQUIRED)
		message("Found clang-tidy      : ${CLANG_TIDY_PATH}")

		set_target_properties(
			${target}
			PROPERTIES CXX_CLANG_TIDY
			"${CLANG_TIDY_PATH}"
			# "${CLANG_TIDY_PATH};-checks=*;--warnings-as-errors=*"
			# "${CLANG_TIDY_PATH};--extra-arg=/EHsc"
			#CXX_CLANG_TIDY"${CLANG_TIDY_PATH};--extra-arg=/EHsc;-checks=*;--warnings-as-errors=*"
		)
	endif()
endfunction()