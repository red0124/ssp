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

# ---- Declare library ----

add_library(ssp INTERFACE)
add_library(ssp::ssp ALIAS ssp)

target_include_directories(
    ssp
    ${ssp_warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(ssp INTERFACE cxx_std_17)

# ---- Install ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(ssp_directory "ssp-${PROJECT_VERSION}")
set(ssp_include_directory "${CMAKE_INSTALL_INCLUDEDIR}/${ssp_directory}")

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
    DESTINATION "${ssp_include_directory}"
    COMPONENT ssp_Development
)

install(
    TARGETS ssp
    EXPORT sspTargets
    INCLUDES DESTINATION "${ssp_include_directory}"
)

write_basic_package_version_file(
    ssp-config-version.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

set(ssp_install_cmakedir "${CMAKE_INSTALL_LIBDIR}/cmake/${ssp_directory}")

install(
    FILES "${PROJECT_BINARY_DIR}/ssp-config-version.cmake"
    DESTINATION "${ssp_install_cmakedir}"
    COMPONENT ssp_Development
)

install(
    EXPORT sspTargets
    FILE ssp-config.cmake
    NAMESPACE ssp::
    DESTINATION "${ssp_install_cmakedir}"
    COMPONENT ssp_Development
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  include(CPack)
endif()
