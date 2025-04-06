cmake_minimum_required(VERSION 3.21)
project(confetti LANGUAGES C VERSION 0.4.0)

include(CMakePackageConfigHelpers)

option(CONFETTI_BUILD_EXAMPLES "Build Confetti C API examples" ON)
option(CONFETTI_BUILD_TESTS "Build Confetti tests" OFF)

option(CONFETTI_CODE_COVERAGE "Toggle code coverage" OFF)
option(CONFETTI_UNDEFINED_BEHAVIOR_SANITIZER "Toggle undefined behavior sanitizer" OFF)
option(CONFETTI_ADDRESS_SANITIZER "Toggle address sanitizer" OFF)
option(CONFETTI_MEMORY_SANITIZER "Toggle address sanitizer" OFF)

# Python is required to generate the Unicode data table source file.
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/confetti_unidata.c")
    message(FATAL_ERROR "Please run unicode.py before configuring the project!")
endif()

# Generate version information.
write_basic_package_version_file(${CMAKE_BINARY_DIR}/ConfettiConfigVersion.cmake COMPATIBILITY SameMajorVersion)

# Generate configuration for CMake's find package tools.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ConfettiConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/ConfettiConfig.cmake" @ONLY)

# Register the library.
add_library(confetti STATIC confetti.c confetti_unidata.c confetti.h)
set_target_properties(confetti PROPERTIES PUBLIC_HEADER confetti.h)
set_property(TARGET confetti PROPERTY C_STANDARD 11)
set_property(TARGET confetti PROPERTY C_STANDARD_REQUIRED TRUE)

# Enable compiler flags to detect common issues.
if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
    target_compile_options(confetti PRIVATE -pedantic)
    if (CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(confetti PRIVATE
            -Wall -Wextra -Wconversion -Wno-missing-field-initializers
            -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion)
    endif ()
endif ()

# Set installation files.
install(TARGETS confetti ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)
install(FILES ${PROJECT_BINARY_DIR}/ConfettiConfig.cmake DESTINATION cmake)
install(FILES ${PROJECT_BINARY_DIR}/ConfettiConfigVersion.cmake DESTINATION cmake)

# Add code examples.
if (CONFETTI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

# Register the tests if building from repo checkout.
# The release acrhive omits the unit test.
if (CONFETTI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
