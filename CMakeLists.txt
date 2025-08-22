cmake_minimum_required(VERSION 3.15)

project(libmcstatus
        VERSION 0.0.0
        DESCRIPTION "A C++ library for Minecraft server status checking"
        LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Conan integration
find_package(Boost REQUIRED)

# Library options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(LIBMCSTATUS_BUILD_TESTS "Build tests" ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Library source files
file(GLOB_RECURSE LIBMCSTATUS_SOURCES "src/*.cpp")
file(GLOB_RECURSE LIBMCSTATUS_HEADERS "include/*.hpp")

# Create the library
add_library(mcstatus ${LIBMCSTATUS_SOURCES} ${LIBMCSTATUS_HEADERS})

# Set target properties
set_target_properties(mcstatus PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME "mcstatus"
)

# Link dependencies
target_link_libraries(mcstatus
        PUBLIC
        Boost::boost
        PRIVATE
        resolv
)

# Include directories for the library
target_include_directories(mcstatus
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler features
target_compile_features(mcstatus PUBLIC cxx_std_20)

# Install rules
install(TARGETS mcstatus
        EXPORT libmcstatusTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

install(DIRECTORY include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT libmcstatusTargets
        FILE libmcstatusTargets.cmake
        NAMESPACE libmcstatus::
        DESTINATION lib/cmake/libmcstatus
)

# Create config files
include(CMakePackageConfigHelpers)

configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/cmake/libmcstatusConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/libmcstatusConfig.cmake"
        INSTALL_DESTINATION lib/cmake/libmcstatus
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/libmcstatusConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/libmcstatusConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/libmcstatusConfigVersion.cmake"
        DESTINATION lib/cmake/libmcstatus
)

# Testing
if(LIBMCSTATUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
