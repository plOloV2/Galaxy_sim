# @author Aleksander Dziągwa

cmake_minimum_required(VERSION 4.1)

project(Galaxy_sim C)
message("Build type: " ${CMAKE_BUILD_TYPE})


# Setup C compiler settings
set(CMAKE_C_STANDARD                23)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)


# Common C flags
set(COMMON_FLAGS -Wall -Wextra -Werror -pipe -march=native)


# Sources (OMP_GPU_SRCS is automatically populated only by toolchain-icx.cmake)
set(sources_SRCS
    src/main.c
    src/create_StarData.c
    src/basic_CPU/gravity_vector.c
    src/basic_CPU/multi_core_b.c
    src/basic_CPU/single_core_b.c
    src/SIMD_CPU/multi_core_s.c
    src/SIMD_CPU/single_core_s.c
    src/test_run.c
    ${OMP_GPU_SRCS}
)


# Include directories
set(include_DIRS
    inc
)


# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME} ${sources_SRCS})


# LTO support check for release version 
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    message(STATUS "IPO/LTO is supported. Enabling for Release build.")
    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
    message(WARNING "IPO/LTO is not supported: ${ipo_error}")
endif()


# Compilation flags, definitions, and link flags (OMP_* expand only when set by toolchain)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ${OMP_GPU_DEFINITIONS})

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE 
    ${COMMON_FLAGS}
    ${OMP_OFFLOAD_COMPILE_FLAGS}
    $<$<CONFIG:Debug>:-Og -ggdb3 -Wno-error=pass-failed> 
    $<$<CONFIG:Release>:-O3 -DNDEBUG -fomit-frame-pointer>
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE ${OMP_OFFLOAD_LINK_FLAGS})


# Fetch PRNG lib
include(FetchContent)
FetchContent_Declare(
    nXoshiro256
    GIT_REPOSITORY https://github.com/plOloV2/nXoshiro256
    GIT_TAG        main
)
FetchContent_MakeAvailable(nXoshiro256)


# Include OpenMP
find_package(OpenMP REQUIRED)


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


# Ensures the version header is generated before compilation
add_dependencies(${CMAKE_PROJECT_NAME} GenerateVersionHeader)


# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${include_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR} 
)


# Link libraries to executable
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE nXoshiro256 OpenMP::OpenMP_C m)


# Set final binary dir to /
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
