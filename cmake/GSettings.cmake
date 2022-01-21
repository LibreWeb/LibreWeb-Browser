# Source: https://git.jami.net/savoirfairelinux/jami-client-gnome/-/blob/master/cmake/GSettings.cmake
option(GSETTINGS_LOCALCOMPILE "Compile GSettings schemas locally during build to the location of the binary (no need to run 'make install')" ON)
option(GSETTINGS_PREFIXINSTALL "Install GSettings Schemas relative to the location specified by the install prefix (instead of relative to where GLib is installed)" ON)
option(GSETTINGS_COMPILE "Compile GSettings Schemas after installation" ${GSETTINGS_LOCALCOMPILE})

if(GSETTINGS_LOCALCOMPILE)
    message(STATUS "GSettings schemas will be compiled to the build directory during the build.")
endif()

if(GSETTINGS_PREFIXINSTALL)
    message (STATUS "GSettings schemas will be installed relative to the cmake install prefix.")
else()
    message (STATUS "GSettings schemas will be installed relative to the GLib install location.")
endif()

if(GSETTINGS_COMPILE)
    message (STATUS "GSettings shemas will be compiled after install.")
endif()

macro(add_schema SCHEMA_NAME OUTPUT)

    set(PKG_CONFIG_EXECUTABLE pkg-config)

    if(GSETTINGS_PREFIXINSTALL)
        set (GSETTINGS_DIR "${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas/")
    else()
        execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} glib-2.0 --variable prefix OUTPUT_VARIABLE _glib_prefix OUTPUT_STRIP_TRAILING_WHITESPACE)
        set (GSETTINGS_DIR "${_glib_prefix}/share/glib-2.0/schemas/")
    endif()

    # Validate the schema
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} gio-2.0 --variable glib_compile_schemas  OUTPUT_VARIABLE _glib_comple_schemas OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${_glib_comple_schemas} --dry-run --schema-file=${CMAKE_CURRENT_SOURCE_DIR}/schema/${SCHEMA_NAME} ERROR_VARIABLE _schemas_invalid OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(_schemas_invalid)
      message(SEND_ERROR "Schema validation error: ${_schemas_invalid}")
    endif(_schemas_invalid)


    if(GSETTINGS_LOCALCOMPILE)
        # compile locally during build to not force the user to 'make install'
        # when running from the build dir
        add_custom_command(
            OUTPUT "${PROJECT_BINARY_DIR}/gschemas.compiled"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src/schema"
            COMMAND
                "${_glib_comple_schemas}"
            ARGS
                "${PROJECT_SOURCE_DIR}/src/schema"
                "--targetdir=${PROJECT_BINARY_DIR}"
            DEPENDS
                "${PROJECT_SOURCE_DIR}/src/schema/${SCHEMA_NAME}"
            VERBATIM
        )

        set(${OUTPUT} "${PROJECT_BINARY_DIR}/gschemas.compiled")
    endif(GSETTINGS_LOCALCOMPILE)

    # Install
    if(WIN32)
        message (STATUS "GSettings schema ${SCHEMA_NAME} will be installed into share/glib-2.0/schemas")
        # For Windows the destination should always be relative
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/${SCHEMA_NAME} DESTINATION share/glib-2.0/schemas)
        # Also we want to ship the compiled binary to Windows
        install(FILES ${PROJECT_BINARY_DIR}/gschemas.compiled DESTINATION share/glib-2.0/schemas)
    else()
        message (STATUS "GSettings schema ${SCHEMA_NAME} will be installed into ${GSETTINGS_DIR}")
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/schema/${SCHEMA_NAME} DESTINATION ${GSETTINGS_DIR})
    endif()

    # Compile
    if(GSETTINGS_COMPILE)
        install(CODE "message (STATUS \"Compiling GSettings schemas\")")
        install(CODE "execute_process (COMMAND ${_glib_comple_schemas} ${GSETTINGS_DIR})")
    endif()
endmacro()
