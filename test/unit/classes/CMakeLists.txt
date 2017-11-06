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
add_test( readsession_unit_test readsession_ut )
