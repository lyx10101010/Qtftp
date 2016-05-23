project(libftp_test)

if (CMAKE_HOST_WIN32)
    #winsock2 lib needed for htonl family of functions
    set(PLATFORM_LIBS "Ws2_32.lib")
else()
    set(PLATFORM_LIBS "")
endif()

add_subdirectory(unit)
add_subdirectory(proofofconcept)
