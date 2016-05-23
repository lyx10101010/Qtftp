project(qtftp_lib)

# GNUInstallDirs sets variable CMAKE_INSTALL_LIBDIR that we use to install library in correct lib dir
include("GNUInstallDirs")

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/include )

set( QTFTP_INCLUDE_FILES include/abstractsocket.h
                         include/udpsocket.h
                         include/udpsocketfactory.h
                         include/session.h
                         include/readsession.h
                         include/tftpserver.h
                         include/tftp_error.h
                         include/tftp_utils.h
                         include/tftp_constants.h
)

set( QTFTP_SOURCE_FILES src/udpsocket.cpp
                        src/udpsocketfactory.cpp
                        src/session.cpp
                        src/readsession.cpp
                        src/tftpserver.cpp
                        src/abstractsocket.cpp
                        src/tftp_utils.cpp
)

#because the include files are in a different directory than the .cpp files we have to include them
#as source files for our library, to let cmake automoc process them.
add_library(qtftp STATIC ${QTFTP_SOURCE_FILES} ${QTFTP_INCLUDE_FILES} )
target_link_libraries(qtftp Qt5::Network)
install(TARGETS qtftp ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${QTFTP_INCLUDE_FILES} DESTINATION include)

if (NOT MSVC)
    #below is a dummy target to let the header files show up in QtCreator. It can safely be removed if needed.
    add_custom_target(qtftpLibHeaderDummy SOURCES ${QTFTP_INCLUDE_FILES} )
endif()
