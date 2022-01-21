find_package(Git QUIET REQUIRED)
if(GIT_FOUND)
    # Get last tag from git
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=0 --tags
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    message(STATUS "GIT_TAG: ${GIT_TAG}")
    if("${GIT_TAG}" MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
        set(GIT_TAG_VERSION "${GIT_TAG}")
    else()
        set(GIT_TAG_VERSION "0.0.0")
    endif()
else(GIT_FOUND)
    message("GIT needs to be installed to generate GIT versioning.")
endif(GIT_FOUND)
