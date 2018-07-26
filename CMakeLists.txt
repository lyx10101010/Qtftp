cmake_minimum_required(VERSION 3.4)
cmake_policy(SET CMP0048 NEW) #allows to set version in project command
#cmake_policy(VERSION 3.2)
project(QTFTP VERSION 1.2.0)

OPTION(BUILD_SHARED_LIBS   "Build all libraries as shared library [default: ON]" ON)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

enable_testing()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_RELAXED_MODE ON)

#If you want to build with a Qt installation in a non-standard location, set CMAKE variable 
# QT_PREFIX to the location of your Qt installation (for example QT_PREFIX=/home/joe/installdir/qt)
if ( QT_PREFIX )
    SET(CMAKE_PREFIX_PATH "${QT_PREFIX}/lib${LIBSUFFIX}/cmake" ${CMAKE_PREFIX_PATH})
    message(STATUS "QT_PREFIX has been set, will look for external Qt in directory ${QT_PREFIX}")
endif()
SET (QT_MIN_VERSION "5.6.0")
find_package(Qt5Core ${QT_MIN_VERSION} REQUIRED)
find_package(Qt5Network ${QT_MIN_VERSION} REQUIRED)
find_package(Qt5Test ${QT_MIN_VERSION} REQUIRED)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    find_package(Systemd 228 REQUIRED)
endif()


get_target_property(QtCore_location Qt5::Core LOCATION)
message(STATUS "Qt at location: ${QtCore_location} will be used." )

add_subdirectory(lib)
add_subdirectory(qtftpd)
add_subdirectory(test)
add_subdirectory(doc)
