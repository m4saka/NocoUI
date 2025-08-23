cmake_minimum_required(VERSION 3.12)
project(NocoEditor CXX)

if (NOT CMAKE_CONFIGURATION_TYPES AND 
    NOT CMAKE_NO_BUILD_TYPE AND
    NOT CMAKE_BUILD_TYPE AND
    CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "[!] Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

file(GLOB EDITOR_SRC_FILES *.cpp)

add_executable(NocoEditor
  ${EDITOR_SRC_FILES}
  )

if(APPLE)
    set_target_properties(NocoEditor PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "NocoEditor"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
    
    set(APP_BUNDLE_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/NocoEditor.app)
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/engine")
        add_custom_command(TARGET NocoEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/engine
            ${APP_BUNDLE_PATH}/Contents/Resources/engine
        )
    endif()
    
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns)
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icon.icns PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(NocoEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns)
    endif()
endif()

target_link_libraries(NocoEditor PRIVATE NocoUI)

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
