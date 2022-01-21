project(whereami)

set(TARGET_NAME ${PROJECT_NAME})

# Create static library
add_library(${TARGET_NAME} STATIC whereami.c)

# Use PUBLIC iso INTERFACE, so the library itself also finds it's own header file
target_include_directories(
    ${TARGET_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
