function(swbt_apply_common_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-Wall>
            $<$<COMPILE_LANGUAGE:C>:-Wextra>
            $<$<COMPILE_LANGUAGE:C>:-Wconversion>
            $<$<COMPILE_LANGUAGE:C>:-Wshadow>
            $<$<COMPILE_LANGUAGE:C>:-Wswitch-enum>
            $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>
            $<$<COMPILE_LANGUAGE:CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
            $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
            $<$<COMPILE_LANGUAGE:CXX>:-Wswitch-enum>
        )
    endif()

    if(SWBT_ENABLE_CLANG_TIDY)
        find_program(CLANG_TIDY_EXE NAMES clang-tidy)
        if(CLANG_TIDY_EXE)
            set_target_properties(${target_name} PROPERTIES
                C_CLANG_TIDY "${CLANG_TIDY_EXE}"
                CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
            )
        else()
            message(FATAL_ERROR "SWBT_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
        endif()
    endif()
endfunction()
