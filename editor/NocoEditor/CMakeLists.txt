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

file(GLOB_RECURSE NOCO_SRC_FILES ../../src/*.cpp)

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

target_compile_definitions(NocoEditor PRIVATE NOCO_EDITOR)

if(BUILD_TESTING)
enable_testing()
add_test(
  NAME Test
  COMMAND NocoEditor
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endif()
