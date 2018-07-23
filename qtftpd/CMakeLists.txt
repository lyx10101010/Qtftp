project(qtftp_test_proofofconcept_qtftpd)




add_executable(qtftpd src/main.cpp)
target_link_libraries(qtftpd Qtftp)

