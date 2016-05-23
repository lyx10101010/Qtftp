# - Run Doxygen
#
# Adds a doxygen target that runs doxygen to generate the html
# and optionally the LaTeX API documentation.
# The doxygen target is added to the doc target as dependency.
# i.e.: the API documentation is built with:
#  make doc
#
# USAGE: INCLUDE IN PROJECT
#
#  set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
#  include(UseDoxygen)
# Add the Doxyfile.in and UseDoxygen.cmake files to the projects source directory.
#
#
# Variables you may define are:
#  DOXYFILE_OUTPUT_DIR - Path where the Doxygen output is stored. Defaults to "doc".
#
#  DOXYFILE_LATEX_DIR - Directory where the Doxygen LaTeX output is stored. Defaults to "latex".
#
#  DOXYFILE_HTML_DIR - Directory where the Doxygen html output is stored. Defaults to "html".
#

#
#  Copyright (c) 2009, 2010 Tobias Rautenkranz <tobias@rautenkranz.ch>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
#  Modified by W. Dobbe

macro(usedoxygen_set_default name value)
    if(NOT DEFINED "${name}")
        set("${name}" "${value}")
    endif()
endmacro()


find_package(Doxygen)


if(DOXYGEN_FOUND)
    find_file(DOXYFILE_IN "btdoxygen.in"
        PATHS "${CMAKE_CURRENT_SOURCE_DIR}"
        NO_DEFAULT_PATH)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(DOXYFILE_IN DEFAULT_MSG "DOXYFILE_IN")
endif()


if(DOXYGEN_FOUND AND DOXYFILE_IN_FOUND)
    configure_file(${DOXYFILE_IN} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.config ESCAPE_QUOTES IMMEDIATE @ONLY)
    if (CMAKE_HOST_WIN32)
        string(REPLACE "/" "\\" CMAKE_CURRENT_BINARY_DIR_WINDOWS ${CMAKE_CURRENT_BINARY_DIR})
        add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR_WINDOWS}\\doxygen.config)
    else ()
        add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.config)
    endif ()

    usedoxygen_set_default(DOXYFILE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc")
    usedoxygen_set_default(DOXYFILE_HTML_DIR "html")

    set_property(DIRECTORY APPEND PROPERTY
    ADDITIONAL_MAKE_CLEAN_FILES "${DOXYFILE_OUTPUT_DIR}/${DOXYFILE_HTML_DIR}")

    set(DOXYFILE_LATEX "NO")
    set(DOXYFILE_PDFLATEX "NO")
    set(DOXYFILE_DOT "NO")

    find_package(LATEX)
    if(LATEX_COMPILER AND MAKEINDEX_COMPILER)
        set(DOXYFILE_LATEX "YES")
        usedoxygen_set_default(DOXYFILE_LATEX_DIR "latex")

        set_property(DIRECTORY APPEND PROPERTY
            ADDITIONAL_MAKE_CLEAN_FILES
            "${DOXYFILE_OUTPUT_DIR}/${DOXYFILE_LATEX_DIR}")

        if(PDFLATEX_COMPILER)
            set(DOXYFILE_PDFLATEX "YES")
        endif()
        if(DOXYGEN_DOT_EXECUTABLE)
            set(DOXYFILE_DOT "YES")
        endif()

        add_custom_command(TARGET doc
            POST_BUILD
            COMMAND ${CMAKE_MAKE_PROGRAM}
            WORKING_DIRECTORY "${DOXYFILE_OUTPUT_DIR}/${DOXYFILE_LATEX_DIR}")

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/doc/latex/refman.pdf
            DESTINATION doc/pdf
            RENAME ${CMAKE_PROJECT_NAME}_reference.pdf )
    endif()

    install( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html
        DESTINATION doc
        OPTIONAL )

endif()

