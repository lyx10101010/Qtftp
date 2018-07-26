project(qtftp_test_proofofconcept_qtftpd)




add_executable(qtftpd src/main.cpp)
target_link_libraries(qtftpd
                      PUBLIC Qtftp
                      PRIVATE $<$<PLATFORM_ID:Linux>:Systemd::systemd>
)

target_compile_features( qtftpd

    PRIVATE
        cxx_override
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

target_compile_definitions( qtftpd PRIVATE $<$<CONFIG:Debug>:QTFTP_DEBUG_BUILD>)
target_compile_definitions( qtftpd PRIVATE QTFTP_DEBUG_BUILD)

install(TARGETS qtftpd
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
