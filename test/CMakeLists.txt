cmake_minimum_required(VERSION 3.14)

project(ssp_tests CXX)

# ---- Dependencies ----

include(FetchContent)
fetchcontent_declare(ssp SOURCE_DIR "${PROJECT_SOURCE_DIR}/..")
fetchcontent_makeavailable(ssp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ssp INTERFACE -Wall -Wextra)
endif()

include(FetchContent)
fetchcontent_declare(
    DOCTEST
    GIT_REPOSITORY https://github.com/red0124/doctest
    GIT_TAG origin/master
    GIT_SHALLOW TRUE
)

fetchcontent_makeavailable(DOCTEST)
set(DOCTEST "${FETCHCONTENT_BASE_DIR}/doctest-src")

# ---- Test ----

enable_testing()

foreach(name IN ITEMS test_splitter test_parser test_converter test_extractions)
    add_executable("${name}" "${name}.cpp")
    target_link_libraries(
        "${name}"
        PRIVATE ssp::ssp fast_float doctest::doctest
    )
    target_compile_definitions(
        "${name}"
        PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN CMAKE_GITHUB_CI
    )
    add_test(NAME "${name}" COMMAND "${name}")
endforeach()
