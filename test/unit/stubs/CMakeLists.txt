project(qtftp_test_unit_stublib)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/include
                     ${qtftp_SOURCE_DIR}/lib/include
)

file( GLOB QTFTP_STUBLIB_INCLUDE_FILES SOURCES include/*.h )

set( QTFTP_STUBLIB_SOURCE_FILES src/udpsocketstub.cpp
                                src/udpsocketstubfactory.cpp
                                src/simulatednetworkstream.cpp
)

#because the include files are in a different directory than the .cpp files we have to include them
#as source files for our library, to let cmake automoc process them.
add_library(qtftp_unit_stub STATIC ${QTFTP_STUBLIB_SOURCE_FILES} ${QTFTP_STUBLIB_INCLUDE_FILES} )
target_compile_features( qtftp_unit_stub
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


target_link_libraries(qtftp_unit_stub Qt5::Network)
