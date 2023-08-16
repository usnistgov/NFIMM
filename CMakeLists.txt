
cmake_minimum_required(VERSION 3.10)

# If this error is encountered:
#  "CMake Error in /.../NFIMM/build/CMakeFiles/CMakeTmp/CMakeLists.txt:
#   Target "cmTC_32aa2" requires the language dialect "CXX17" (with compiler
#   extensions), but CMake does not know the compile flags to use to enable it."
# then most likely a later version of GNU is required.
# In this event that GNU is located elsewhere, prior to running cmake,
# run the following (your location may vary):
# $ export CXX=/usr/local/bin/g++
# $ export CC=/usr/local/bin/gcc (optional)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(NFIMM VERSION 0.1.0)

message(STATUS "PROJECT_NAME: ${PROJECT_NAME}")

# Pick up the library
add_subdirectory(src/lib)
add_subdirectory(src/bin)
