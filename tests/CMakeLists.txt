find_package(Audition REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..) # For finding confetti.h

if (CONFETTI_UNDEFINED_BEHAVIOR_SANITIZER)
    target_compile_options(confetti PRIVATE -g -fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
elseif (CONFETTI_ADDRESS_SANITIZER)
    target_compile_options(confetti PRIVATE -g -fsanitize=address)
    add_link_options(-fsanitize=address)
elseif (CONFETTI_MEMORY_SANITIZER)
    target_compile_options(confetti PRIVATE -O0 -g -fsanitize=memory -fsanitize-memory-track-origins)
    add_link_options(-fsanitize=memory -fsanitize-memory-track-origins)
elseif (CONFETTI_CODE_COVERAGE)
    if (NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Code coverage generation requires GCC.")
    endif ()

    target_compile_options(confetti PRIVATE -g -fprofile-arcs -ftest-coverage -O0)
    add_link_options(-fprofile-arcs -ftest-coverage -fbranch-probabilities -O0)

    # Generate an HTML code coverage report for review.
    add_custom_target(coverage)
    add_custom_command(
        TARGET coverage
        POST_BUILD
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_coverage.sh ${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report: ${CMAKE_BINARY_DIR}/coverage")

    # Check code coverage for local review.
    add_custom_target(check-coverage)
    add_custom_command(
        TARGET check-coverage
        POST_BUILD
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/check_coverage.sh ${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Checking code coverage...")
endif ()

add_executable(tests_confetti
    test_parse.c
    test_walk.c
    test_bidi.c
    test_custom_allocator.c
    test_out_of_memory.c
    test_extensions.c
    test_parse_api.c
    test_walk_api.c
    test_abort.c
    test_utils.c
    test_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../confetti.h
)
set_property(TARGET tests_confetti PROPERTY C_STANDARD 11)
target_compile_definitions(tests_confetti PRIVATE -DPATH_TO_SNAPSHOTS="${CMAKE_CURRENT_SOURCE_DIR}/snapshots")
target_compile_definitions(tests_confetti PRIVATE -DPATH_TO_TESTDATA="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_compile_definitions(tests_confetti PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_include_directories(tests_confetti PRIVATE ${AUDITION_INCLUDE_DIR})
target_link_libraries(tests_confetti confetti)
target_link_libraries(tests_confetti ${AUDITION_LIBRARY})
if (CODE_COVERAGE)
    target_link_options(tests_confetti PRIVATE -g -fprofile-instr-generate -fcoverage-mapping)
endif ()

add_test(confetti tests_confetti)

# Check if Clang is available as well as its fuzzer.
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    include(CheckCSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS -fsanitize=address,fuzzer)
    set(CMAKE_REQUIRED_LINK_OPTIONS -fsanitize=address,fuzzer)
    check_c_source_compiles(
        "
#include <stddef.h>
#include <stdint.h>
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) { return 0; }
        "
        HAVE_FUZZER
    )
    if (HAVE_FUZZER)
        # Fuzzing is only compatible with the ADDRESS sanitizer.
        if (NOT CONFETTI_UNDEFINED_BEHAVIOR_SANITIZER AND NOT CONFETTI_MEMORY_SANITIZER)
            # These options are common to all fuzz tests.
            add_compile_options(-g -O0 -fsanitize=address,fuzzer)
            add_link_options(-fsanitize=address,fuzzer)
            add_link_options(-fprofile-instr-generate)
            link_libraries(confetti)

            # Add the fuzz tests.
            add_executable(fuzz_parse fuzz_parse.c)
            add_executable(fuzz_walk fuzz_walk.c)

            # Register tests.
            add_test(NAME FuzzParse COMMAND fuzz_parse "${CMAKE_CURRENT_SOURCE_DIR}/corpus" -runs=10000)
            add_test(NAME FuzzWalk COMMAND fuzz_walk "${CMAKE_CURRENT_SOURCE_DIR}/corpus" -runs=10000)
        endif ()
    endif ()
endif ()
