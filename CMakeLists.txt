cmake_minimum_required(VERSION 3.16)
project(ParticleFountain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)

qt_add_executable(ParticleFountain
    main.cpp
    MainWindow.cpp
    GLWidget.cpp
    ConfigManager.cpp
)

target_link_libraries(ParticleFountain
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# 简单的文件复制（不使用 add_custom_command）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/FountainConfig.json")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/FountainConfig.json"
        "${CMAKE_CURRENT_BINARY_DIR}/FountainConfig.json"
        COPYONLY
    )
endif()