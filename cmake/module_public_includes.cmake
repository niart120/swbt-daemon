function(swbt_configure_module_public_includes target)
    set(options)
    set(one_value_args)
    set(multi_value_args MODULES)
    cmake_parse_arguments(SWBT_PUBLIC_INCLUDES
        "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
    )

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Unknown target for public include root: ${target}")
    endif()
    if(NOT SWBT_PUBLIC_INCLUDES_MODULES)
        message(FATAL_ERROR "MODULES is required for ${target} public include root")
    endif()

    get_filename_component(include_base
        "${CMAKE_CURRENT_BINARY_DIR}/swbt_public_includes"
        ABSOLUTE
    )
    get_filename_component(include_root "${include_base}/${target}" ABSOLUTE)
    cmake_path(NORMAL_PATH include_base OUTPUT_VARIABLE include_base_normalized)
    cmake_path(NORMAL_PATH include_root OUTPUT_VARIABLE include_root_normalized)
    string(FIND "${include_root_normalized}" "${include_base_normalized}/" root_prefix_index)
    if(NOT root_prefix_index EQUAL 0)
        message(FATAL_ERROR
            "Refusing to refresh public include root outside generated include base: "
            "${include_root_normalized}"
        )
    endif()

    file(REMOVE_RECURSE "${include_root_normalized}")
    file(MAKE_DIRECTORY "${include_root_normalized}")
    foreach(module IN LISTS SWBT_PUBLIC_INCLUDES_MODULES)
        set(module_dir "${CMAKE_CURRENT_SOURCE_DIR}/swbt/${module}")
        if(NOT IS_DIRECTORY "${module_dir}")
            message(FATAL_ERROR "Public include module does not exist: ${module}")
        endif()
        file(GLOB_RECURSE header_paths CONFIGURE_DEPENDS "${module_dir}/*.h")
        foreach(header_path IN LISTS header_paths)
            file(RELATIVE_PATH relative_header "${CMAKE_CURRENT_SOURCE_DIR}/swbt" "${header_path}")
            set(output_header "${include_root_normalized}/${relative_header}")
            get_filename_component(output_dir "${output_header}" DIRECTORY)
            file(MAKE_DIRECTORY "${output_dir}")
            configure_file("${header_path}" "${output_header}" COPYONLY)
        endforeach()
    endforeach()

    target_include_directories("${target}"
        PUBLIC
            "${include_root_normalized}"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/swbt"
    )
    set_property(TARGET "${target}" PROPERTY SWBT_PUBLIC_INCLUDE_ROOT "${include_root_normalized}")
    set_property(TARGET "${target}" PROPERTY SWBT_PUBLIC_INCLUDE_MODULES
                 "${SWBT_PUBLIC_INCLUDES_MODULES}")
endfunction()
