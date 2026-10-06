# @author Aleksander Dziągwa

cmake_minimum_required(VERSION 4.1)

project(Galaxy_sim C)
message("Build type: " ${CMAKE_BUILD_TYPE})

# Setup C compiler settings
set(CMAKE_C_STANDARD                23)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)

# Sources
set(sources_SRCS
    src/main.c
)

# Include directories
set(include_DIRS
    inc
)

# App version check
add_custom_target(
    GenerateVersionHeader
    COMMAND ${CMAKE_COMMAND} 
        -D PROJ_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        -D TEMPLATE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_version/version.h.in
        -D OUTPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/inc/version.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_version/git_version.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/inc/version.h
    COMMENT "Checking Git version..."
    VERBATIM
)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME} ${sources_SRCS})

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${include_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR} 
)

# Error flags
set(COMMON_FLAGS -Wall -Wextra -Werror -pipe)

# Ensures the version header is generated before compilation
add_dependencies(${CMAKE_PROJECT_NAME} GenerateVersionHeader)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    message(STATUS "IPO/LTO is supported. Enabling for Release build.")
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
    message(WARNING "IPO/LTO is not supported: ${ipo_error}")
endif()

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE 
    ${COMMON_FLAGS}
    $<$<CONFIG:Debug>:-Og -ggdb3> 
    $<$<CONFIG:Release>:-O3 -DNDEBUG -fomit-frame-pointer>
)
