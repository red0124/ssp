
if(NOT "/home/ado/git/ssp/hbuild/_deps/fast_float-subbuild/fast_float-populate-prefix/src/fast_float-populate-stamp/fast_float-populate-gitinfo.txt" IS_NEWER_THAN "/home/ado/git/ssp/hbuild/_deps/fast_float-subbuild/fast_float-populate-prefix/src/fast_float-populate-stamp/fast_float-populate-gitclone-lastrun.txt")
  message(STATUS "Avoiding repeated git clone, stamp file is up to date: '/home/ado/git/ssp/hbuild/_deps/fast_float-subbuild/fast_float-populate-prefix/src/fast_float-populate-stamp/fast_float-populate-gitclone-lastrun.txt'")
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/home/ado/git/ssp/hbuild/_deps/fast_float-src"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/home/ado/git/ssp/hbuild/_deps/fast_float-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git"  clone --no-checkout --depth 1 --no-single-branch --config "advice.detachedHead=false" "https://github.com/red0124/fast_float.git" "fast_float-src"
    WORKING_DIRECTORY "/home/ado/git/ssp/hbuild/_deps"
    RESULT_VARIABLE error_code
    )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once:
          ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/red0124/fast_float.git'")
endif()

execute_process(
  COMMAND "/usr/bin/git"  checkout origin/meson --
  WORKING_DIRECTORY "/home/ado/git/ssp/hbuild/_deps/fast_float-src"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'origin/meson'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git"  submodule update --recursive --init 
    WORKING_DIRECTORY "/home/ado/git/ssp/hbuild/_deps/fast_float-src"
    RESULT_VARIABLE error_code
    )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/home/ado/git/ssp/hbuild/_deps/fast_float-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy
    "/home/ado/git/ssp/hbuild/_deps/fast_float-subbuild/fast_float-populate-prefix/src/fast_float-populate-stamp/fast_float-populate-gitinfo.txt"
    "/home/ado/git/ssp/hbuild/_deps/fast_float-subbuild/fast_float-populate-prefix/src/fast_float-populate-stamp/fast_float-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/home/ado/git/ssp/hbuild/_deps/fast_float-subbuild/fast_float-populate-prefix/src/fast_float-populate-stamp/fast_float-populate-gitclone-lastrun.txt'")
endif()

