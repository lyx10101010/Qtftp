project(qtftp_test_proofofconcept_qtftpd)


include_directories(${CMAKE_SOURCE_DIR}/lib/include)

add_executable(qtftpd src/main.cpp)
target_link_libraries(qtftpd qtftp Qt5::Network Qt5::Core ${PLATFORM_LIBS})

