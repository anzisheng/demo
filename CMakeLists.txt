cmake_minimum_required(VERSION 3.16)
project(SimpleFountain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)

qt_add_executable(SimpleFountain
    main.cpp
    GLWidget.cpp
    ConfigManager.cpp
)

target_link_libraries(SimpleFountain
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)