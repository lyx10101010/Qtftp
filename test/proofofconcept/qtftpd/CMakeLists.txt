project(qtftp_test_proofofconcept_qtftpd)


include_directories(${qtftp_SOURCE_DIR}/lib/include)

add_executable(qtftpd src/main.cpp)
target_link_libraries(qtftpd Qtftp Qt5::Network Qt5::Core )

