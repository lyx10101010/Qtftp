# FindSystemd
# -------
#
# Find Systemd library
#
# Try to find Systemd library on UNIX systems. 
# 
# This module defines `IMPORTED` target ``Systemd::systemd``, if
# systemd has been found.
#
# The following variables are also defined
#
# ::
#
#   SYSTEMD_FOUND         - True if Systemd is available
#   SYSTEMD_INCLUDE_DIRS  - Include directories for Systemd
#   SYSTEMD_LIBRARIES     - List of libraries for Systemd
#   SYSTEMD_DEFINITIONS   - List of definitions for Systemd
#   SYSTEMD_VERSION       - Version of Systemd library that was found
#
#   
#   Preferred usage is to link against library Systemd::systemd, which will
#   automatically set include directories and compile options for systemd.
#
#=============================================================================
# Copyright (c) 2017 Jari Vetoniemi / W. Dobbe
#
# Distributed under the OSI-approved BSD License (the "License");
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

include(FeatureSummary)
set_package_properties(Systemd PROPERTIES
   URL "http://freedesktop.org/wiki/Software/systemd/"
   DESCRIPTION "System and Service Manager")

find_package(PkgConfig)
pkg_check_modules(PC_SYSTEMD libsystemd)
find_library(SYSTEMD_LIBRARIES NAMES systemd ${PC_SYSTEMD_LIBRARY_DIRS})
find_path(SYSTEMD_INCLUDE_DIRS systemd/sd-login.h HINTS ${PC_SYSTEMD_INCLUDE_DIRS})
set(SYSTEMD_VERSION ${PC_SYSTEMD_VERSION})

set(SYSTEMD_DEFINITIONS ${PC_SYSTEMD_CFLAGS_OTHER})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SYSTEMD
                                  REQUIRED_VARS SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES
                                  VERSION_VAR SYSTEMD_VERSION)
mark_as_advanced(SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES SYSTEMD_DEFINITIONS) 

if (SYSTEMD_FOUND)
    if(NOT TARGET Systemd::systemd)
        add_library(Systemd::systemd UNKNOWN IMPORTED)
        set_property(TARGET Systemd::systemd PROPERTY IMPORTED_LOCATION ${SYSTEMD_LIBRARIES})
        set_target_properties(Systemd::systemd PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SYSTEMD_INCLUDE_DIRS}")
        set_target_properties(Systemd::systemd PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_SYSTEMD_CFLAGS_OTHER}")
        set_target_properties(Systemd::systemd PROPERTIES
            VERSION "${PC_SYSTEMD_VERSION}")
    endif()
endif()
