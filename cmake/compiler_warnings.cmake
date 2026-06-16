function(swbt_apply_common_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wconversion
            -Wshadow
            -Wswitch-enum
            -Werror=implicit-function-declaration
        )
    endif()

    if(SWBT_ENABLE_CLANG_TIDY)
        find_program(CLANG_TIDY_EXE NAMES clang-tidy)
        if(CLANG_TIDY_EXE)
            set_target_properties(${target_name} PROPERTIES
                C_CLANG_TIDY "${CLANG_TIDY_EXE}"
            )
        else()
            message(WARNING "SWBT_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
        endif()
    endif()
endfunction()
