cmake_minimum_required (VERSION 3.8)

project ("Test(clang)")

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
message(FATAL_ERROR "in-source-build is not allowed.")
endif()

set(CMAKE_CXX_FLAGS "-O0 -g -std=c++20 -Wno-constant-conversion -Wno-comment")
if($ENV{PROJECT_COPY})
add_subdirectory("./src" ./built)
else()
add_subdirectory("$ENV{PROJECT_BASE}" ./built)
endif()