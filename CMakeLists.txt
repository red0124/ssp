cmake_minimum_required(VERSION 3.14)

project(
    ssp
    VERSION 1.6.1
    DESCRIPTION "csv parser"
    HOMEPAGE_URL "https://github.com/red0124/ssp"
    LANGUAGES CXX
)

# ---- Warning guard ----

# Protect dependents from this project's warnings if the guard isn't disabled
set(SSP_WARNING_GUARD SYSTEM)
if(SSP_INCLUDE_WITHOUT_SYSTEM)
    set(SSP_WARNING_GUARD "")
endif()

# ---- Dependencies ----

include(FetchContent)
fetchcontent_declare(
    fast_float
    GIT_REPOSITORY https://github.com/red0124/fast_float.git
    GIT_TAG origin/meson
    GIT_SHALLOW TRUE
)

fetchcontent_makeavailable(fast_float)
set(FAST_FLOAT_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/fast_float-src")

# ---- Declare library ----

add_library(ssp INTERFACE)
add_library(ssp::ssp ALIAS ssp)

target_include_directories(
    ssp
    ${SSP_WARNING_GUARD}
    INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/fast_float>"
)

target_compile_features(ssp INTERFACE cxx_std_17)

target_link_libraries(
    ssp
    INTERFACE
        "$<$<AND:$<CXX_COMPILER_ID:AppleClang,Clang>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:c++fs>"
        "$<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.1>>:stdc++fs>"
)

# ---- Install ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(SSP_DIRECTORY "ssp-${PROJECT_VERSION}")
set(SSP_INCLUDE_DIRECTORY "${CMAKE_INSTALL_INCLUDEDIR}")

install(
    DIRECTORY
        "${PROJECT_SOURCE_DIR}/include/"
        "${FAST_FLOAT_SOURCE_DIR}/include/"
    DESTINATION "${SSP_INCLUDE_DIRECTORY}"
    COMPONENT ssp_Development
)

install(
    TARGETS ssp
    EXPORT sspTargets
    INCLUDES DESTINATION "${SSP_INCLUDE_DIRECTORY}"
)

write_basic_package_version_file(
    ssp-config-version.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

set(SSP_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${SSP_DIRECTORY}")

install(
    FILES "${PROJECT_BINARY_DIR}/ssp-config-version.cmake"
    DESTINATION "${SSP_INSTALL_CMAKEDIR}"
    COMPONENT ssp_Development
)

install(
    EXPORT sspTargets
    FILE ssp-config.cmake
    NAMESPACE ssp::
    DESTINATION "${SSP_INSTALL_CMAKEDIR}"
    COMPONENT ssp_Development
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CPack)
endif()
