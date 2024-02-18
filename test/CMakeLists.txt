cmake_minimum_required(VERSION 3.14)

project(ssp_tests CXX)

# ---- Dependencies ----

include(FetchContent)
FetchContent_Declare(ssp SOURCE_DIR "${PROJECT_SOURCE_DIR}/..")
FetchContent_MakeAvailable(ssp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ssp INTERFACE -Wall -Wextra)
endif()

if (MSVC)
  add_compile_options(/bigobj)
elseif (MINGW)
  add_compile_options(-Wa,-mbig-obj -mcmodel=medium)
endif ()

include(FetchContent)
FetchContent_Declare(
    DOCTEST
    GIT_REPOSITORY https://github.com/red0124/doctest
    GIT_TAG origin/master
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(DOCTEST)
set(DOCTEST "${FETCHCONTENT_BASE_DIR}/doctest-src")

# ---- Test ----

enable_testing()

foreach(name IN ITEMS test_splitter test_parser1_1 test_parser1_2
                      test_parser1_3 test_parser1_4 test_converter
                      test_extractions test_parser2_1 test_parser2_2
                      test_parser2_3 test_parser2_4
                      test_extractions_without_fast_float)
    add_executable("${name}" "${name}.cpp")
    target_link_libraries("${name}" PRIVATE ssp::ssp fast_float
                                            doctest::doctest)
    target_compile_definitions(
        "${name}" PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN CMAKE_GITHUB_CI)
    add_test(NAME "${name}" COMMAND "${name}")
endforeach()
