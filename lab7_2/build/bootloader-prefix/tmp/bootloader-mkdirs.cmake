# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/pj/Documents/cse121/esp/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "/home/pj/Documents/cse121/esp/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "/home/pj/Documents/cse121/Lab7-CSE121/lab7_2/build/bootloader"
  "/home/pj/Documents/cse121/Lab7-CSE121/lab7_2/build/bootloader-prefix"
  "/home/pj/Documents/cse121/Lab7-CSE121/lab7_2/build/bootloader-prefix/tmp"
  "/home/pj/Documents/cse121/Lab7-CSE121/lab7_2/build/bootloader-prefix/src/bootloader-stamp"
  "/home/pj/Documents/cse121/Lab7-CSE121/lab7_2/build/bootloader-prefix/src"
  "/home/pj/Documents/cse121/Lab7-CSE121/lab7_2/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/pj/Documents/cse121/Lab7-CSE121/lab7_2/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/pj/Documents/cse121/Lab7-CSE121/lab7_2/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
