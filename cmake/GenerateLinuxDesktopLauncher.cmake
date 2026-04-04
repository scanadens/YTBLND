function(ytblnd_generate_linux_desktop_launcher)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        return()
    endif()

    set(options)
    set(oneValueArgs TARGET TEMPLATE OUTPUT ICON_PATH WORKING_DIRECTORY APP_NAME COMMENT WM_CLASS EXEC_PATH CUSTOM_TARGET_NAME)
    set(multiValueArgs)
    cmake_parse_arguments(YTBLND_LAUNCHER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT YTBLND_LAUNCHER_TARGET)
        message(FATAL_ERROR "ytblnd_generate_linux_desktop_launcher requires TARGET")
    endif()

    if(NOT TARGET ${YTBLND_LAUNCHER_TARGET})
        message(FATAL_ERROR "Target '${YTBLND_LAUNCHER_TARGET}' does not exist")
    endif()

    if(NOT YTBLND_LAUNCHER_TEMPLATE)
        message(FATAL_ERROR "ytblnd_generate_linux_desktop_launcher requires TEMPLATE")
    endif()

    if(NOT YTBLND_LAUNCHER_OUTPUT)
        message(FATAL_ERROR "ytblnd_generate_linux_desktop_launcher requires OUTPUT")
    endif()

    if(NOT YTBLND_LAUNCHER_ICON_PATH)
        message(FATAL_ERROR "ytblnd_generate_linux_desktop_launcher requires ICON_PATH")
    endif()

    if(NOT YTBLND_LAUNCHER_WORKING_DIRECTORY)
        set(YTBLND_LAUNCHER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    endif()

    if(NOT YTBLND_LAUNCHER_APP_NAME)
        set(YTBLND_LAUNCHER_APP_NAME "YTBLND")
    endif()

    if(NOT YTBLND_LAUNCHER_COMMENT)
        set(YTBLND_LAUNCHER_COMMENT "Blend YouTube Watch Later data into a shared feed")
    endif()

    if(NOT YTBLND_LAUNCHER_WM_CLASS)
        set(YTBLND_LAUNCHER_WM_CLASS "ytblnd")
    endif()

    if(YTBLND_LAUNCHER_EXEC_PATH)
        set(YTBLND_EXEC_VALUE "${YTBLND_LAUNCHER_EXEC_PATH}")
    else()
        set(YTBLND_EXEC_VALUE "$<TARGET_FILE:${YTBLND_LAUNCHER_TARGET}>")
    endif()

    set(YTBLND_APP_NAME "${YTBLND_LAUNCHER_APP_NAME}")
    set(YTBLND_COMMENT "${YTBLND_LAUNCHER_COMMENT}")
    set(YTBLND_EXEC "${YTBLND_EXEC_VALUE}")
    set(YTBLND_ICON_PATH "${YTBLND_LAUNCHER_ICON_PATH}")
    set(YTBLND_WORKING_DIRECTORY "${YTBLND_LAUNCHER_WORKING_DIRECTORY}")
    set(YTBLND_WM_CLASS "${YTBLND_LAUNCHER_WM_CLASS}")

    file(READ "${YTBLND_LAUNCHER_TEMPLATE}" YTBLND_LAUNCHER_TEMPLATE_CONTENT)
    string(CONFIGURE "${YTBLND_LAUNCHER_TEMPLATE_CONTENT}" YTBLND_LAUNCHER_OUTPUT_CONTENT @ONLY)

    get_filename_component(YTBLND_LAUNCHER_OUTPUT_DIR "${YTBLND_LAUNCHER_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${YTBLND_LAUNCHER_OUTPUT_DIR}")
    file(GENERATE OUTPUT "${YTBLND_LAUNCHER_OUTPUT}" CONTENT "${YTBLND_LAUNCHER_OUTPUT_CONTENT}")

    if(YTBLND_LAUNCHER_CUSTOM_TARGET_NAME)
        set(YTBLND_LAUNCHER_TARGET_NAME "${YTBLND_LAUNCHER_CUSTOM_TARGET_NAME}")
    else()
        set(YTBLND_LAUNCHER_TARGET_NAME "${YTBLND_LAUNCHER_TARGET}_linux_desktop_entry")
    endif()

    add_custom_target(${YTBLND_LAUNCHER_TARGET_NAME}
        DEPENDS ${YTBLND_LAUNCHER_TARGET}
        COMMENT "Linux desktop entry available at ${YTBLND_LAUNCHER_OUTPUT}"
    )

    message(STATUS "Linux desktop entry will be generated at ${YTBLND_LAUNCHER_OUTPUT}")
endfunction()