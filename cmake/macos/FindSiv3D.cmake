# FindSiv3D.cmake - Siv3D library detection for CMake
#
# This module defines:
#  Siv3D_FOUND - System has Siv3D
#  Siv3D_INCLUDE_DIRS - The Siv3D include directories
#  Siv3D_LIBRARIES - The libraries needed to use Siv3D
#  Siv3D_DEFINITIONS - Compiler switches required for using Siv3D

set(Siv3D_SEARCH_PATHS
    ~/Library/Siv3D
    ~/.siv3d
    /usr/local/opt/siv3d
    /usr/local
    /opt/siv3d
    ${Siv3D_ROOT}
)

find_path(Siv3D_INCLUDE_DIR
    NAMES Siv3D.hpp
    PATHS ${Siv3D_SEARCH_PATHS}
    PATH_SUFFIXES include
)

find_library(Siv3D_LIBRARY
    NAMES Siv3D libSiv3D libSiv3D.a
    PATHS ${Siv3D_SEARCH_PATHS}
    PATH_SUFFIXES lib lib/macOS
)

# Find dependencies
set(Siv3D_DEPENDENCIES)

set(SIV3D_STATIC_LIBS
    boost_filesystem:boost
    freetype:freetype
    harfbuzz:harfbuzz
    libgif:libgif
    turbojpeg:libjpeg-turbo
    ogg:libogg
    png16:libpng
    tiff:libtiff
    vorbis:libvorbis
    vorbisenc:libvorbis
    vorbisfile:libvorbis
    webp:libwebp
    opencv_core:opencv
    opencv_imgcodecs:opencv
    opencv_imgproc:opencv
    opencv_objdetect:opencv
    opencv_photo:opencv
    opencv_videoio:opencv
    opus:opus
    opusfile:opus
    zlib:zlib
)

foreach(lib_pair IN LISTS SIV3D_STATIC_LIBS)
    string(REPLACE ":" ";" lib_info ${lib_pair})
    list(GET lib_info 0 lib_name)
    list(GET lib_info 1 lib_dir)
    string(TOUPPER ${lib_name} LIB_UPPER)
    
    find_library(Siv3D_${LIB_UPPER}_LIBRARY
        NAMES lib${lib_name}.a
        PATHS ${Siv3D_SEARCH_PATHS}
        PATH_SUFFIXES lib/macOS/${lib_dir}
        NO_DEFAULT_PATH
    )
    
    if(Siv3D_${LIB_UPPER}_LIBRARY)
        list(APPEND Siv3D_DEPENDENCIES ${Siv3D_${LIB_UPPER}_LIBRARY})
    endif()
endforeach()

# System frameworks only
list(APPEND Siv3D_DEPENDENCIES
    "-framework AudioToolbox"
    "-framework AVFoundation"
    "-framework CoreAudio"
    "-framework CoreMedia"
    "-framework OpenGL"
    "-framework IOKit"
    "-framework Cocoa"
    "-framework CoreVideo"
    "-framework Metal"
    "-framework MetalKit"
    "-framework GameController"
    "-lcurl"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Siv3D DEFAULT_MSG
    Siv3D_LIBRARY Siv3D_INCLUDE_DIR)

if(Siv3D_FOUND)
    set(Siv3D_LIBRARIES ${Siv3D_LIBRARY} ${Siv3D_DEPENDENCIES})
    set(Siv3D_INCLUDE_DIRS ${Siv3D_INCLUDE_DIR} ${Siv3D_INCLUDE_DIR}/ThirdParty)
    
    if(NOT TARGET Siv3D::Siv3D)
        add_library(Siv3D::Siv3D UNKNOWN IMPORTED)
        set_target_properties(Siv3D::Siv3D PROPERTIES
            IMPORTED_LOCATION "${Siv3D_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Siv3D_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${Siv3D_DEPENDENCIES}"
        )
    endif()
endif()

mark_as_advanced(Siv3D_INCLUDE_DIR Siv3D_LIBRARY)