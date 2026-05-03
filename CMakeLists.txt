cmake_minimum_required(VERSION 3.19)
project(music LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)

qt_standard_project_setup()

qt_add_executable(music
    WIN32 MACOSX_BUNDLE
    main.cpp
    widget.cpp
    widget.h
    gamewidget.cpp
    gamewidget.h
    editorwidget.cpp
    editorwidget.h
    chart.cpp
    chart.h
    note.h
)

target_link_libraries(music
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
)

install(TARGETS music
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET music
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
