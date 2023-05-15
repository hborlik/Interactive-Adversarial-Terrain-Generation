cmake_minimum_required(VERSION 3.15)

include(cmake/get_cpm.cmake)

project(dirtbox VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# application executable
add_subdirectory(dirtbox)