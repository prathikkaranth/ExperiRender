# GPU Crash Monitor library - requires Nsight Aftermath
cmake_minimum_required(VERSION 3.11)

# Collect source files
file(GLOB GPU_CRASH_MONITOR_SOURCES
    "*.cpp"
)

file(GLOB GPU_CRASH_MONITOR_HEADERS
    "*.h"
)

# Create the GPU Crash Monitor library
add_library(GPUCrashMonitor STATIC 
    ${GPU_CRASH_MONITOR_SOURCES} 
    ${GPU_CRASH_MONITOR_HEADERS}
)

# Include directories
target_include_directories(GPUCrashMonitor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link required libraries
target_link_libraries(GPUCrashMonitor PUBLIC
    Vulkan::Vulkan
    NsightAftermath::NsightAftermath
    spdlog::spdlog
)

# Ensure Nsight Aftermath is enabled for this target
target_compile_definitions(GPUCrashMonitor PUBLIC NSIGHT_AFTERMATH_ENABLED)

# Set target properties
set_target_properties(GPUCrashMonitor PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "GPU Crash Monitor library configured with Nsight Aftermath support")