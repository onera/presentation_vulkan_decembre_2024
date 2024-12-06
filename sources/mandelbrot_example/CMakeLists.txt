cmake_minimum_required (VERSION 3.2)
project (vulkan_compute_example)

find_package(Vulkan)

# get rid of annoying MSVC warnings.
#add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-O3 -march=native -fomit-frame-pointer -fopenmp")# -mkl -tbb")

include_directories(${Vulkan_INCLUDE_DIR})

set(ALL_LIBS  ${Vulkan_LIBRARY} )

add_executable(vulkan_compute_example src/lodepng.cpp src/debug_utils.cpp src/vk_computing.cpp src/saving_png.cpp src/application.cpp)

set_target_properties(vulkan_compute_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(vulkan_compute_example ${ALL_LIBS} )
