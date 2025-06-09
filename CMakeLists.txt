cmake_minimum_required(VERSION 3.16)
project(NocoUI)

# C++20を使用
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# サブディレクトリを追加
add_subdirectory(editor/NocoEditor)
add_subdirectory(tests/NocoUITests)

# デフォルトターゲットの設定
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT NocoEditor)
