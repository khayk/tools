function(AddClangTidy target)
	if(TOOLS_TIDY)
		find_program(CLANG-TIDY_PATH clang-tidy REQUIRED)
		message("Found clang-tidy      : ${CLANG-TIDY_PATH}")
	
		set_target_properties(
			${target}
			PROPERTIES CXX_CLANG_TIDY
			"${CLANG-TIDY_PATH};--extra-arg=/EHsc"
			#CXX_CLANG_TIDY"${CLANG-TIDY_PATH};--extra-arg=/EHsc;-checks=*;--warnings-as-errors=*"
		)
	endif()
endfunction()