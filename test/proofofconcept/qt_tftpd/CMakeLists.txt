project(libtftp_test_proofofconcept_qt_tftpd)


include_directories(${CMAKE_SOURCE_DIR}/lib/include)

add_executable(qt_tftpd src/main.cpp)
target_link_libraries(qt_tftpd libtftp Qt5::Network Qt5::Core ${PLATFORM_LIBS})

