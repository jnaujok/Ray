# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/iaslib-populate-gitclone-lastrun.txt" AND EXISTS "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/iaslib-populate-gitinfo.txt" AND
  "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/iaslib-populate-gitclone-lastrun.txt" IS_NEWER_THAN "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/iaslib-populate-gitinfo.txt")
  message(STATUS
    "Avoiding repeated git clone, stamp file is up to date: "
    "'/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/iaslib-populate-gitclone-lastrun.txt'"
  )
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git"
            clone --no-checkout --config "advice.detachedHead=false" "https://github.com/jnaujok/IASLib.git" "iaslib-src"
    WORKING_DIRECTORY "/home/jnaujok/projects/Ray/build-prof/_deps"
    RESULT_VARIABLE error_code
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/jnaujok/IASLib.git'")
endif()

execute_process(
  COMMAND "/usr/bin/git"
          checkout "de038c077de025696015f3a3b70230f4380bb8bb" --
  WORKING_DIRECTORY "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'de038c077de025696015f3a3b70230f4380bb8bb'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-src"
    RESULT_VARIABLE error_code
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/iaslib-populate-gitinfo.txt" "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/iaslib-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/iaslib-populate-gitclone-lastrun.txt'")
endif()
