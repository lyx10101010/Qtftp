project(qtftp_test_unit_qtftpserver)


INCLUDE_DIRECTORIES( ${qtftp_SOURCE_DIR}/test/unit/stubs/include
                     ${qtftp_SOURCE_DIR}/lib/include )

set( AUTOMOC_MOC_OPTIONS "-I${qtftp_SOURCE_DIR}/lib/include" )


add_executable(tftpserver_ut  tftpserver_ut.cpp )
add_executable(readsession_ut readsession_ut.cpp)

set( UNIT_TEST_REQUIRED_LIBS qtftp_unit_stub Qtftp Qt5::Network Qt5::Test ${CMAKE_THREAD_LIBS_INIT} )

target_link_libraries(tftpserver_ut  ${UNIT_TEST_REQUIRED_LIBS} )
target_link_libraries(readsession_ut ${UNIT_TEST_REQUIRED_LIBS} )

target_compile_features( tftpserver_ut
    PUBLIC
        cxx_override
    
    PRIVATE
        cxx_auto_type
        cxx_constexpr
        cxx_deleted_functions
        cxx_lambdas
        cxx_noexcept
        cxx_strong_enums
        cxx_uniform_initialization
        cxx_user_literals
        cxx_raw_string_literals
        cxx_std_14
)

target_compile_features( readsession_ut
    PUBLIC
        cxx_override
    
    PRIVATE
        cxx_auto_type
        cxx_constexpr
        cxx_deleted_functions
        cxx_lambdas
        cxx_noexcept
        cxx_strong_enums
        cxx_uniform_initialization
        cxx_user_literals
        cxx_raw_string_literals
        cxx_std_14
)

add_test( tftpserver_unit_test tftpserver_ut )

# One of the test files should not be readable while running unit tests, to provoke a "permission denied" error.
# However some build systems (like Yocto) don't like files that they can't read, so restore permissions after test.
if ( CMAKE_HOST_UNIX )
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_readsession_unittest.cmake 
    "execute_process(COMMAND chmod 000 no_permission.txt
                    WORKING_DIRECTORY ${qtftp_test_unit_testfiles_SOURCE_DIR}
                    RESULT_VARIABLE CHMOD_RESULT
                    ERROR_VARIABLE CHMOD_ERROR )
    message(\"chmod result: \${CHMOD_RESULT}\")
    execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/readsession_ut
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    RESULT_VARIABLE UT_RESULT
    )
    message(\"unit test result: \${UT_RESULT}\")
    execute_process(COMMAND chmod 644 no_permission.txt
                    WORKING_DIRECTORY ${qtftp_test_unit_testfiles_SOURCE_DIR}
                    RESULT_VARIABLE CHMOD_RESULT
                    ERROR_VARIABLE CHMOD_ERROR )
    message(\"chmod result: \${CHMOD_RESULT}\")
    ")
else()
    if ( CMAKE_HOST_WIN32 )
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_readsession_unittest.cmake 
        "execute_process(COMMAND icacls no_permission.txt /deny Users:R Administrators:R SYSTEM:R
                        WORKING_DIRECTORY ${qtftp_test_unit_testfiles_SOURCE_DIR}
                        RESULT_VARIABLE CHMOD_RESULT
                        ERROR_VARIABLE CHMOD_ERROR )
        message(\"chmod result: \${CHMOD_RESULT}\")
        execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/readsession_ut
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        RESULT_VARIABLE UT_RESULT
        )
        message(\"unit test result: \${UT_RESULT}\")
        execute_process(COMMAND icacls no_permission.txt /grant Users:R Administrators:R SYSTEM:R
                        WORKING_DIRECTORY ${qtftp_test_unit_testfiles_SOURCE_DIR}
                        RESULT_VARIABLE CHMOD_RESULT
                        ERROR_VARIABLE CHMOD_ERROR )
        message(\"chmod result: \${CHMOD_RESULT}\")
        ")
    endif()
endif()


add_test(NAME readsession_unit_test COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/run_readsession_unittest.cmake )
#add_test(readsession_unit_test readsession_ut)
