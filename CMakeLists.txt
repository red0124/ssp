cmake_minimum_required(VERSION 3.14)

project(
    ssp
    VERSION 0.0.1
    DESCRIPTION "Static split parser"
    HOMEPAGE_URL "https://github.com/red0124/ssp"
    LANGUAGES CXX
)

# ---- Warning guard ----

# Protect dependents from this project's warnings if the guard isn't disabled
set(ssp_warning_guard SYSTEM)
if(ssp_INCLUDE_WITHOUT_SYSTEM)
  set(ssp_warning_guard "")
endif()

# ---- Dependencies ----

FetchContent_Declare(
  fast_float
  GIT_REPOSITORY https://github.com/red0124/fast_float.git
  GIT_TAG origin/meson
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(fast_float)

# ---- Declare library ----

add_library(ssp INTERFACE)
add_library(ssp::ssp ALIAS ssp)

target_include_directories(
    ssp
    ${ssp_warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    fast_float
)

target_compile_features(ssp INTERFACE cxx_std_17)

target_link_libraries(
    ssp
    INTERFACE
    "$<$<AND:$<CXX_COMPILER_ID:AppleClang,Clang>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:c++fs>"
    "$<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.1>>:stdc++fs>"
)
