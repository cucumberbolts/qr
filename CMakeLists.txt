cmake_minimum_required(VERSION 3.5)

project(qr C)

# For IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCES
    "include/qr.h"

    "src/qr.c"
    "src/qr_write.c"

    "lib/stb_image_write.h"
)

add_library(${PROJECT_NAME} ${SOURCES})

# Set folder for IDEs
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "QR/qr")

# Produce json file for nvim
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    "include/"
    PRIVATE
    "lib/"
)

add_subdirectory(test)
