project(libtftp_test_unit)


include_directories( ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/lib/src )

add_definitions(-DTFTP_TEST_FILES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/test_files\")

add_subdirectory(test_files)
add_subdirectory(stubs)
add_subdirectory(classes)
