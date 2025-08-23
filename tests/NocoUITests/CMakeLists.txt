cmake_minimum_required(VERSION 3.16)
project(OpenSiv3D_Linux_App CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# FetchContentを使用してCatch2 v2.13.6を取得
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.6
)
FetchContent_MakeAvailable(Catch2)

find_package(PkgConfig)

pkg_check_modules(LIBGLES2 glesv2)
pkg_check_modules(LIBX11 x11)
pkg_check_modules(LIBXI xi)
pkg_check_modules(LIBXCURSOR xcursor)
pkg_check_modules(LIBXRANDR xrandr)
pkg_check_modules(LIBXXF86VM xxf86vm)

set(TARGET_NAME NocoUITest)

add_executable(${TARGET_NAME} 
    Main.cpp
    TestNode.cpp
    TestHitTest.cpp
    TestTransformHitTest.cpp
    TestComponent.cpp
    TestCanvas.cpp
    TestRegion.cpp
    TestLayout.cpp
    TestSerialization.cpp
    TestProperty.cpp
    TestPropertyValue.cpp
    TestEventHandling.cpp
    TestComplexLayout.cpp
    TestErrorCases.cpp
    TestSerializationDetails.cpp
    TestStyleState.cpp
    TestParam.cpp
    TestParent.cpp
    TestTextBox.cpp
    TestTextArea.cpp
    TestComponentFactory.cpp
    TestPlaceholderComponent.cpp
)

find_package(Siv3D REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE NocoUI Siv3D::Siv3D)

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
# target_precompile_headers(${TARGET_NAME} PRIVATE <Siv3D.hpp>)

if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${TARGET_NAME} PRIVATE -fpie)
    target_link_options(${TARGET_NAME} PRIVATE -lpthread -ldl -pie)
endif()

# Catch2をリンク
target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2)

# CTestサポートを有効化
enable_testing()
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})

# 実行時の出力ディレクトリを設定
set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)