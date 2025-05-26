# Test sources
file(GLOB TEST_SOURCES "*.cpp")

# Create test executable
add_executable(RendererTests ${TEST_SOURCES})

# Link with GTest, GMock, and your renderer library
target_link_libraries(RendererTests PRIVATE
    gtest
    gtest_main
    gmock
    gmock_main
    RendererLib
)

# Include GTest directories and VkRenderer directory
target_include_directories(RendererTests PRIVATE
    ${gtest_SOURCE_DIR}/include
    ${gmock_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/VkRenderer
)

# Discover and register tests
include(GoogleTest)
gtest_discover_tests(RendererTests)