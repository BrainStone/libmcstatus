find_package(GTest REQUIRED)

# Test executable
file(GLOB_RECURSE TEST_SOURCES "*.cpp")
file(GLOB_RECURSE TEST_HEADERS "*.hpp")

add_executable(libmcstatus_tests ${TEST_SOURCES} ${TEST_HEADERS})

target_link_libraries(libmcstatus_tests
        PRIVATE
        mcstatus
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

# Pedantic Warnings
target_compile_options(mcstatus PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wall -Wextra -Wpedantic -Werror>
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W4 /WX>
)

target_include_directories(libmcstatus_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(libmcstatus_tests)

# Custom test target
add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS libmcstatus_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
