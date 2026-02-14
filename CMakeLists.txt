cmake_minimum_required(VERSION 3.16)
project(CppShell LANGUAGES CXX)

# CMake 4.x no longer supports projects requiring policies older than 3.5.
# Some third-party dependencies still declare very old minimum versions.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tooling versions (for CI/docs):
# - clang-format-20
# - clang-tidy-20

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
foreach(config Debug Release RelWithDebInfo MinSizeRel)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_SOURCE_DIR}/bin)
endforeach()

add_library(cppshell_core
    src/cppshell/tokenizer.cpp
    src/cppshell/parser.cpp
    src/cppshell/environment.cpp
    src/cppshell/builtins.cpp
    src/cppshell/external_command.cpp
    src/cppshell/command.cpp
    src/cppshell/shell.cpp
)

target_include_directories(cppshell_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(cppshell_core PUBLIC cxx_std_23)

add_executable(cppshell src/cli/cli.cpp)
target_link_libraries(cppshell PRIVATE cppshell_core)

include(CTest)
if (BUILD_TESTING)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        doctest
        URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(doctest)

    add_executable(cppshell_test_helper
        tests/test_helper.cpp
    )
    target_compile_features(cppshell_test_helper PRIVATE cxx_std_23)

    add_executable(cppshell_tests
        tests/test_main.cpp
        tests/test_tokenizer.cpp
        tests/test_parser.cpp
        tests/test_builtins.cpp
        tests/test_external.cpp
    )

    target_link_libraries(cppshell_tests PRIVATE cppshell_core doctest::doctest)
    target_compile_features(cppshell_tests PRIVATE cxx_std_23)
    target_compile_definitions(cppshell_tests PRIVATE
        CPPSHELL_TEST_HELPER_PATH="$<TARGET_FILE:cppshell_test_helper>"
    )

    add_test(NAME cppshell_tests COMMAND cppshell_tests)
endif()

