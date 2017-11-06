project(qtftp_test_unit_testfiles)

# one of the test files should not be readable while running unit tests, to provoke a "permission denied" error
if ( CMAKE_HOST_UNIX )
    execute_process(COMMAND chmod 000 no_permission.txt
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    ERROR_VARIABLE CHMOD_ERROR )
else()
    if (CMAKE_HOST_WIN32)
        execute_process(COMMAND icacls no_permission.txt /deny Users:R Administrators:R SYSTEM:R
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        ERROR_VARIABLE CHMOD_ERROR
						OUTPUT_QUIET)
    endif()
endif()
if (CHMOD_ERROR)
    message( "Configure error: could not remove read permission from test file 'no_permission.txt' (${CHMOD_ERROR}). At least one unit test will fail" )
endif()

if ( CHMOD_ERROR )
    message( "Could not change permission of test file ${CMAKE_CURRENT_SOURCE_DIR}/no_permission.txt (${CHMOD_ERROR})" )
    message( "One or more unit tests will fail." )
endif()


# one of the tests needs a large file. To keep the repository size small, we generate it.
file(REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/large_file.txt)
file( READ ${CMAKE_CURRENT_SOURCE_DIR}/1024_byte_file.txt ONE_KB_FILE_CONTENTS LIMIT 1024 )

set(LOOP_INDEX 0)
while( LOOP_INDEX LESS 5)
    file(APPEND ${CMAKE_CURRENT_SOURCE_DIR}/large_file.txt ${ONE_KB_FILE_CONTENTS})
    math( EXPR LOOP_INDEX ${LOOP_INDEX}+1 )
endwhile()
