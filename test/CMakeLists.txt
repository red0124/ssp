cmake_minimum_required(VERSION 3.14)

project(ssp_tests CXX)

# ---- Dependencies ----

set(
    ssp_INCLUDE_WITHOUT_SYSTEM
    YES
    CACHE
    INTERNAL
    "Turn the warning guard off to have errors appear in test builds"
)

include(FetchContent)
FetchContent_Declare(ssp SOURCE_DIR "${PROJECT_SOURCE_DIR}/..")
FetchContent_MakeAvailable(ssp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(ssp INTERFACE -Wall -Wextra)
endif()

find_package(doctest 2.4.4 CONFIG REQUIRED)
# for doctest_discover_tests
include(doctest)

# ---- Test ----

enable_testing()

foreach(name IN ITEMS test_parser test_converter test_extractions)
  add_executable("${name}" "${name}.cpp")
  target_link_libraries("${name}" PRIVATE ssp::ssp doctest::doctest)
  target_compile_definitions("${name}" PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
  doctest_discover_tests("${name}")
endforeach()
