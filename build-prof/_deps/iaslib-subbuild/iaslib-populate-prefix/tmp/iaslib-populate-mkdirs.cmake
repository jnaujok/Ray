# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-src"
  "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-build"
  "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix"
  "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/tmp"
  "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp"
  "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src"
  "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/jnaujok/projects/Ray/build-prof/_deps/iaslib-subbuild/iaslib-populate-prefix/src/iaslib-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
