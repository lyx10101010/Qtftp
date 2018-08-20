from conans import ConanFile, CMake
import os

class Qtftp(ConanFile):
    name = "Qtftp"
    version = "1.2.0"
    license = "https://github.com/wdobbe/Qtftp/blob/master/COPYING.LIB"
    url = "https://github.com/wdobbe/Qtftp"
    description = "Tftp library and server implemented with Qt5"
    requires = ( "Qt/5.11.1@dynniq/stable")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=True"
    generators = "cmake_paths"
    exports_sources = ["*"]

    def build(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = os.path.join(os.getcwd(), 'conan_paths.cmake')
        cmake.definitions["BUILD_SHARED_LIBS"] = self.options.shared
        cmake.configure()
        cmake.build()
        cmake.install()
        

    def package(self):
        pass
        

