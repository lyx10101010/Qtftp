project(libtftp)

cmake_minimum_required(VERSION 3.2)
cmake_policy(VERSION 3.2)

enable_testing()

if ( QT_PREFIX )
    SET(CMAKE_PREFIX_PATH "${QT_PREFIX}/lib${LIBSUFFIX}/cmake")
    message(STATUS "QT_PREFIX has been set, will look for Qt in directory ${CMAKE_PREFIX_PATH}")
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_RELAXED_MODE ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Test REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_HOST_UNIX)
    set( CMAKE_THREAD_PREFER_PTHREAD TRUE )
    find_package(Threads REQUIRED )
    set( GCC_EXTRA_COMPILE_OPTIONS "${GCC_EXTRA_COMPILE_OPTIONS} -pthread" )
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using Clang, AppleClang or g++
  if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0" )
  endif()
  if (DEFINED CLANG_MEM_CHECK AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-optimize-sibling-calls" )
  endif()
endif()

if (MSVC)
    #suppress suggestion to use checked iterators, because they are a Microsoft extension
    #also suppress suggestion to use strcpy_s because it is Microsoft specific
    add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
endif()


get_target_property(QtCore_location Qt5::Core LOCATION)
message( "libtftp will be linked against qtcore library: ${QtCore_location}" )

add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(doc)
