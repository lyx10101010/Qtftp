project(libtftp_test_unit_classes)


INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/test/unit/stubs/include )

set( AUTOMOC_MOC_OPTIONS "-I${CMAKE_SOURCE_DIR}/lib/include" )


add_executable(tftpserver_ut  tftpserver_ut.cpp )
add_executable(readsession_ut readsession_ut.cpp)

set( UNIT_TEST_REQUIRED_LIBS libtftp_unit_stub libtftp Qt5::Network Qt5::Test ${CMAKE_THREAD_LIBS_INIT} )

target_link_libraries(tftpserver_ut  ${UNIT_TEST_REQUIRED_LIBS} ${PLATFORM_LIBS})
target_link_libraries(readsession_ut ${UNIT_TEST_REQUIRED_LIBS} ${PLATFORM_LIBS})

add_test( tftpserver_unit_test tftpserver_ut )
add_test( readsession_unit_test readsession_ut )
