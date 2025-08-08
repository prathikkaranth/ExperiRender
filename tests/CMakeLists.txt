# Test sources
file(GLOB TEST_SOURCES "*.cpp")

add_executable(RendererTests ${TEST_SOURCES})

target_link_libraries(RendererTests PRIVATE
    gtest
    gtest_main
    gmock
    gmock_main
    RendererLib
)

target_include_directories(RendererTests PRIVATE
    ${gtest_SOURCE_DIR}/include
    ${gmock_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/VkRenderer
)

include(GoogleTest)
gtest_discover_tests(RendererTests)