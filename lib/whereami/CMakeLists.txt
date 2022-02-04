project(whereami)

set(LIBARY_NAME ${PROJECT_NAME})

# Create static library
add_library(${LIBARY_NAME} STATIC whereami.c whereami.h)

# Add X/Open definition for realpath
target_compile_definitions(${LIBARY_NAME} PUBLIC _XOPEN_SOURCE=500)

# Use PUBLIC iso INTERFACE, so the library itself also finds it's own header file
target_include_directories(
    ${LIBARY_NAME}
    SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
