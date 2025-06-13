cmake_minimum_required(VERSION 3.12)
project(OpenSiv3D_Linux_App CXX)

if (NOT CMAKE_CONFIGURATION_TYPES AND 
    NOT CMAKE_NO_BUILD_TYPE AND
    NOT CMAKE_BUILD_TYPE AND
    CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "[!] Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

# ソースファイルを明示的にリストアップしてキャッシュ効率を向上
file(GLOB_RECURSE NOCO_SRC_FILES ../../src/*.cpp)

# 並列ビルドの設定
if(CMAKE_GENERATOR MATCHES "Make")
    include(ProcessorCount)
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j${N}")
    endif()
endif()

add_executable(NocoEditor
  Main.cpp
  HistorySystem.cpp
  ResizableHandle.cpp
  ${NOCO_SRC_FILES}
  )

target_include_directories(NocoEditor PRIVATE
  ../../include
  )

find_package(Siv3D)
target_link_libraries(NocoEditor PUBLIC Siv3D::Siv3D)

target_compile_features(NocoEditor PRIVATE cxx_std_20)

# プリコンパイルヘッダの設定
target_precompile_headers(NocoEditor PRIVATE ../../include/NocoUI/pch.hpp)

# ビルド最適化フラグ
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # デバッグビルドでもある程度の最適化
    target_compile_options(NocoEditor PRIVATE -O1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    # リリースビルドでの最適化
    target_compile_options(NocoEditor PRIVATE -O2 -DNDEBUG)
endif()

target_compile_definitions(NocoEditor PRIVATE NOCO_EDITOR)

if(BUILD_TESTING)
enable_testing()
add_test(
  NAME Test
  COMMAND NocoEditor
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endif()
