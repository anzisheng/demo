cmake_minimum_required(VERSION 3.16)
project(ParticleFountain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets Multimedia)

qt_add_executable(ParticleFountain
    main.cpp
    MainWindow.cpp
    GLWidget.cpp
    ConfigManager.cpp
    FountainDesigner.cpp
    FountainFile.cpp
    MusicPlayer.cpp
)

target_link_libraries(ParticleFountain
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Multimedia
)

# 复制配置文件
set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/FountainConfig.json")
if(EXISTS "${CONFIG_FILE}")
    add_custom_command(
        TARGET ParticleFountain POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CONFIG_FILE}"
            "$<TARGET_FILE_DIR:ParticleFountain>/FountainConfig.json"
        COMMENT "Copying FountainConfig.json to output directory"
    )
endif()

# 复制水帘图片目录（可选，运行时从指定路径读取）
# 注意：需要在运行时确保 resources 目录在可执行文件所在路径或使用绝对路径