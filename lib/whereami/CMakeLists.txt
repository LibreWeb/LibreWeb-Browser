project(whereami)

set(LIBARY_NAME ${PROJECT_NAME})

# Create static library
add_library(${LIBARY_NAME} STATIC whereami.c whereami.h)

# Use PUBLIC iso INTERFACE, so the library itself also finds it's own header file
target_include_directories(
    ${LIBARY_NAME}
    SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
