include_guard(GLOBAL)

function(swbt_btstack_glob out_var)
    set(collected_sources)
    foreach(pattern IN LISTS ARGN)
        file(GLOB pattern_sources "${pattern}")
        list(APPEND collected_sources ${pattern_sources})
    endforeach()
    set(${out_var} ${collected_sources} PARENT_SCOPE)
endfunction()

function(swbt_btstack_require_existing_sources)
    foreach(source IN LISTS ARGN)
        if(NOT EXISTS "${source}")
            message(FATAL_ERROR "BTstack source does not exist: ${source}")
        endif()
    endforeach()
endfunction()

function(swbt_collect_btstack_sources)
    set(one_value_args
        BACKEND
        BTSTACK_DIR
        OUT_INCLUDE_DIRS
        OUT_LINK_LIBRARIES
        OUT_SOURCES
    )
    cmake_parse_arguments(SWBT "" "${one_value_args}" "" ${ARGN})

    if(NOT SWBT_BTSTACK_DIR)
        message(FATAL_ERROR "BTSTACK_DIR is required")
    endif()
    if(NOT SWBT_BACKEND)
        message(FATAL_ERROR "BACKEND is required")
    endif()
    if(NOT SWBT_OUT_SOURCES)
        message(FATAL_ERROR "OUT_SOURCES is required")
    endif()
    if(NOT SWBT_OUT_INCLUDE_DIRS)
        message(FATAL_ERROR "OUT_INCLUDE_DIRS is required")
    endif()
    if(NOT SWBT_OUT_LINK_LIBRARIES)
        message(FATAL_ERROR "OUT_LINK_LIBRARIES is required")
    endif()

    get_filename_component(btstack_dir "${SWBT_BTSTACK_DIR}" ABSOLUTE)
    if(NOT EXISTS "${btstack_dir}/src/hci.c")
        message(FATAL_ERROR "BTstack source tree is missing at ${btstack_dir}")
    endif()

    swbt_btstack_glob(common_sources
        "${btstack_dir}/src/*.c"
    )
    swbt_btstack_glob(core_transport_sources
        "${btstack_dir}/src/hci_transport_*.c"
    )
    list(REMOVE_ITEM common_sources ${core_transport_sources})

    swbt_btstack_glob(classic_sources
        "${btstack_dir}/src/classic/*.c"
    )
    swbt_btstack_glob(ble_sources
        "${btstack_dir}/src/ble/*.c"
    )
    list(REMOVE_ITEM ble_sources "${btstack_dir}/src/ble/le_device_db_memory.c")

    swbt_btstack_glob(gatt_sources
        "${btstack_dir}/src/ble/gatt-service/*.c"
    )
    swbt_btstack_glob(bluedroid_sources
        "${btstack_dir}/3rd-party/bluedroid/encoder/srce/*.c"
        "${btstack_dir}/3rd-party/bluedroid/decoder/srce/*.c"
    )
    swbt_btstack_glob(hxcmod_sources
        "${btstack_dir}/3rd-party/hxcmod-player/*.c"
        "${btstack_dir}/3rd-party/hxcmod-player/mods/*.c"
    )
    swbt_btstack_glob(lc3_google_sources
        "${btstack_dir}/3rd-party/lc3-google/src/*.c"
    )
    set(third_party_sources
        ${bluedroid_sources}
        ${hxcmod_sources}
        ${lc3_google_sources}
        "${btstack_dir}/3rd-party/md5/md5.c"
        "${btstack_dir}/3rd-party/micro-ecc/uECC.c"
        "${btstack_dir}/3rd-party/qr-code-generator/qrcodegen.c"
        "${btstack_dir}/3rd-party/rijndael/rijndael.c"
        "${btstack_dir}/3rd-party/yxml/yxml.c"
    )

    set(include_dirs
        "${btstack_dir}/3rd-party/bluedroid/decoder/include"
        "${btstack_dir}/3rd-party/bluedroid/encoder/include"
        "${btstack_dir}/3rd-party/hxcmod-player"
        "${btstack_dir}/3rd-party/hxcmod-player/mods"
        "${btstack_dir}/3rd-party/lc3-google/include"
        "${btstack_dir}/3rd-party/md5"
        "${btstack_dir}/3rd-party/micro-ecc"
        "${btstack_dir}/3rd-party/qr-code-generator"
        "${btstack_dir}/3rd-party/rijndael"
        "${btstack_dir}/3rd-party/yxml"
        "${btstack_dir}/platform/embedded"
        "${btstack_dir}/src"
    )

    if(SWBT_BACKEND STREQUAL "libusb")
        swbt_btstack_glob(posix_sources
            "${btstack_dir}/platform/posix/*.c"
        )
        list(REMOVE_ITEM posix_sources "${btstack_dir}/platform/posix/le_device_db_fs.c")

        swbt_btstack_glob(backend_sources
            "${btstack_dir}/platform/libusb/*.c"
            "${btstack_dir}/chipset/realtek/*.c"
            "${btstack_dir}/chipset/zephyr/*.c"
        )

        list(APPEND include_dirs
            "${btstack_dir}/chipset/realtek"
            "${btstack_dir}/chipset/zephyr"
            "${btstack_dir}/platform/libusb"
            "${btstack_dir}/platform/posix"
            "${btstack_dir}/port/libusb"
        )
        set(link_libraries
            libusb-1.0
            m
        )
        set(platform_sources ${posix_sources} ${backend_sources})
    elseif(SWBT_BACKEND STREQUAL "windows-winusb")
        swbt_btstack_glob(platform_sources
            "${btstack_dir}/platform/windows/*.c"
            "${btstack_dir}/chipset/zephyr/*.c"
        )

        list(APPEND include_dirs
            "${btstack_dir}/chipset/zephyr"
            "${btstack_dir}/platform/windows"
            "${btstack_dir}/port/windows-winusb"
        )
        set(link_libraries
            setupapi
            winusb
        )
    else()
        message(FATAL_ERROR "Unsupported SWBT_BACKEND '${SWBT_BACKEND}'. Expected libusb or windows-winusb.")
    endif()

    set(sources
        ${third_party_sources}
        ${platform_sources}
        ${common_sources}
        ${ble_sources}
        ${gatt_sources}
        ${classic_sources}
    )
    list(REMOVE_DUPLICATES sources)
    list(REMOVE_DUPLICATES include_dirs)
    list(SORT sources)
    list(SORT include_dirs)
    swbt_btstack_require_existing_sources(${sources})

    set(${SWBT_OUT_SOURCES} ${sources} PARENT_SCOPE)
    set(${SWBT_OUT_INCLUDE_DIRS} ${include_dirs} PARENT_SCOPE)
    set(${SWBT_OUT_LINK_LIBRARIES} ${link_libraries} PARENT_SCOPE)
endfunction()

function(swbt_define_btstack_source_selection target_name)
    swbt_collect_btstack_sources(
        BTSTACK_DIR "${SWBT_BTSTACK_DIR}"
        BACKEND "${SWBT_BACKEND}"
        OUT_SOURCES selected_sources
        OUT_INCLUDE_DIRS selected_include_dirs
        OUT_LINK_LIBRARIES selected_link_libraries
    )

    add_library(${target_name} INTERFACE)
    target_include_directories(${target_name} INTERFACE ${selected_include_dirs})
    target_link_libraries(${target_name} INTERFACE ${selected_link_libraries})
    set_property(TARGET ${target_name} PROPERTY SWBT_BTSTACK_SELECTED_SOURCES "${selected_sources}")
    set_property(TARGET ${target_name} PROPERTY SWBT_BTSTACK_SELECTED_INCLUDE_DIRS "${selected_include_dirs}")
    set_property(TARGET ${target_name} PROPERTY SWBT_BTSTACK_SELECTED_LINK_LIBRARIES "${selected_link_libraries}")
    list(LENGTH selected_sources selected_source_count)
    message(STATUS "BTstack backend: ${SWBT_BACKEND}")
    message(STATUS "BTstack selected sources: ${selected_source_count}")
endfunction()
