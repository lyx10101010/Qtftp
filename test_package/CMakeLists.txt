project(PackageTest CXX)
cmake_minimum_required(VERSION 3.8)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qtftp)

#include_directories(${CONAN_INCLUDE_DIRS})
#link_directories(${CONAN_LIB_DIRS})
add_executable(example example.cpp)
#target_link_libraries(example ${CONAN_LIBS})
target_link_libraries(example Qtftp::Qtftp)


