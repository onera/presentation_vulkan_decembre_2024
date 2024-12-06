cmake_minimum_required(VERSION 3.20)
project(kompute_mat_mat_mul)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -march=native")


# Options
option(KOMPUTE_OPT_GIT_TAG "The tag of the repo to use for the example" v0.9.0)
option(KOMPUTE_OPT_FROM_SOURCE "Whether to build example from source or from git fetch repo" ON)

if(KOMPUTE_OPT_FROM_SOURCE)
    add_subdirectory(../../ ${CMAKE_CURRENT_BINARY_DIR}/kompute_build)
else()
    include(FetchContent)
    FetchContent_Declare(kompute GIT_REPOSITORY https://github.com/KomputeProject/kompute.git
        GIT_TAG ${KOMPUTE_OPT_GIT_TAG})
    FetchContent_MakeAvailable(kompute)
    include_directories(${kompute_SOURCE_DIR}/src/include)
endif()

# Compiling shader
# To add more shaders simply copy the vulkan_compile_shader command and replace it with your new shader
vulkan_compile_shader(
    INFILE shader/mulmatmat.comp
    OUTFILE shader/mulmatmat_h.hpp
    NAMESPACE "shader")

# Then add it to the library, so you can access it later in your code
add_library(shader INTERFACE "shader/mulmatmat_h.hpp")
target_include_directories(shader INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

# Setting up main example code
add_executable(kompute_mat_mat_mul src/main.cpp)
target_link_libraries(kompute_mat_mat_mul PRIVATE shader kompute::kompute)
target_link_libraries(kompute_mat_mat_mul PRIVATE shader openblas)

