cmake_minimum_required(VERSION 3.5)

project(chardevice LANGUAGES C)

add_executable(chardevice main.c)

include(GNUInstallDirs)
install(TARGETS chardevice
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
