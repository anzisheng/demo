cmake_minimum_required(VERSION 3.16)
project(ParticleFountain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # 自动处理 Qt 的 MOC

# 查找 Qt6 组件 - 添加 OpenGLWidgets
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    OpenGL
    OpenGLWidgets  # 新增！
)

# 设置源文件
set(SOURCES
    main.cpp
    MainWindow.cpp
    GLWidget.cpp
)

set(HEADERS
    MainWindow.h
    GLWidget.h
)

# 创建可执行文件
qt_add_executable(ParticleFountain ${SOURCES} ${HEADERS})

# 链接 Qt 库 - 添加 OpenGLWidgets
target_link_libraries(ParticleFountain
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets  # 新增！
)