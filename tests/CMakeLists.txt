find_package(GTest REQUIRED)

# Test executable
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(libmcstatus_tests ${TEST_SOURCES})

target_link_libraries(libmcstatus_tests
        PRIVATE
        mcstatus
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(libmcstatus_tests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

if (ENABLE_COVERAGE)
    target_compile_options(libmcstatus_tests PRIVATE --coverage)
    target_link_libraries(libmcstatus_tests PRIVATE --coverage)
endif ()

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(libmcstatus_tests)

# Custom test target
add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS libmcstatus_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
