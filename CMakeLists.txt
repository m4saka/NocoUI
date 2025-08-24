cmake_minimum_required(VERSION 3.16)
project(NocoUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# macOSのみカスタムFindSiv3D.cmakeを使用
if(APPLE)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos")
endif()

# macOS: x86_64 build for Siv3D 0.6.x compatibility
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++2a")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)
    
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19.2")
        set(CMAKE_APPLE_SILICON_PROCESSOR "x86_64")
    endif()
    
    message(STATUS "Building for x86_64 architecture (Rosetta 2 will be used on Apple Silicon)")
endif()

if(NOT DEFINED Siv3D_ROOT)
    set(Siv3D_ROOT "$ENV{HOME}/Library/Siv3D" CACHE PATH "Path to Siv3D installation")
endif()

file(GLOB_RECURSE NOCOUI_SOURCES src/*.cpp)
add_library(NocoUI STATIC ${NOCOUI_SOURCES})

target_include_directories(NocoUI PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Siv3D REQUIRED)
target_link_libraries(NocoUI PUBLIC Siv3D::Siv3D)

if(APPLE)
    target_compile_options(NocoUI PRIVATE
        -Wall
        -Wextra
        -Wno-unknown-pragmas
        -fvisibility=hidden
    )
endif()

target_compile_features(NocoUI PUBLIC cxx_std_20)

add_subdirectory(editor/NocoEditor)
add_subdirectory(tests/NocoUITests)

if(WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT NocoEditor)
elseif(APPLE AND CMAKE_GENERATOR MATCHES "Xcode")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY XCODE_STARTUP_PROJECT NocoEditor)
endif()
