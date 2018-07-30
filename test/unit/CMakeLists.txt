project(qtftp_test_unit)


include_directories( ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/lib/src )

add_subdirectory(test_files)
add_subdirectory(stubs)
add_subdirectory(classes)
