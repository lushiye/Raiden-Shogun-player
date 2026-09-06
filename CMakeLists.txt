cmake_minimum_required(VERSION 3.21)

project(RaidenShogunPlayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 要求 Qt 6.8 及以上；需要 Quick（界面）、Multimedia（播放引擎）、Sql（SQLite 输入模块）
find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia Sql)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(RaidenShogunPlayer
    src/main.cpp
    src/input/MusicLibrary.h
    src/input/MusicLibrary.cpp
    src/input/TrackListModel.h
    src/input/TrackListModel.cpp
    src/input/DecryptService.h
    src/input/DecryptService.cpp
    src/player/PlayerController.h
    src/player/PlayerController.cpp
)

# QML 模块：URI 作为 C++ 侧 loadFromModule 的模块名
qt_add_qml_module(RaidenShogunPlayer
    URI RaidenShogunPlayer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/PlaylistView.qml
        qml/PlayerControls.qml
        qml/DecryptDialog.qml
)

target_link_libraries(RaidenShogunPlayer PRIVATE
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Sql
)

# 以项目根目录为 include 基准，支持 #include "src/..." 的写法
target_include_directories(RaidenShogunPlayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 解密程序 qqmusic_des.exe 的绝对路径（供输入模块在运行时定位）
target_compile_definitions(RaidenShogunPlayer PRIVATE
    QQMUSIC_DES_EXE="${CMAKE_CURRENT_SOURCE_DIR}/qqmusic_des.exe"
)

# MSVC 下按 UTF-8 编译源文件，保证中文字符串/解析标记正确
if(MSVC)
    target_compile_options(RaidenShogunPlayer PRIVATE /utf-8)
endif()

set_target_properties(RaidenShogunPlayer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
