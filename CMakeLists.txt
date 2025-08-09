cmake_minimum_required(VERSION 3.16)
project(NocoUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE NOCOUI_SOURCES src/*.cpp)
add_library(NocoUI STATIC ${NOCOUI_SOURCES})

target_include_directories(NocoUI PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Siv3D REQUIRED)
target_link_libraries(NocoUI PUBLIC Siv3D::Siv3D)
target_compile_features(NocoUI PUBLIC cxx_std_20)

add_subdirectory(editor/NocoEditor)
add_subdirectory(tests/NocoUITests)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT NocoEditor)
