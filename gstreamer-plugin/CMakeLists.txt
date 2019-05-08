cmake_minimum_required(VERSION 2.8)
project(gst-svt-hevc C)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

FIND_PACKAGE(PkgConfig REQUIRED)
pkg_check_modules (GSTREAMER REQUIRED gstreamer-1.0>=1.8)
include_directories (${GSTREAMER_INCLUDE_DIRS})

pkg_check_modules (GSTREAMER_BASE REQUIRED gstreamer-base-1.0>=1.8)
include_directories (${GSTREAMER_BASE_INCLUDE_DIRS})

pkg_check_modules (GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.8)
include_directories (${GSTREAMER_VIDEO_INCLUDE_DIRS})

pkg_check_modules (SVT_HEVC REQUIRED SvtHevcEnc>=1.3)
include_directories (${SVT_HEVC_INCLUDE_DIRS})

set(CMAKE_C_FLAGS "-fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2 -O2 -flto -Wall -Wextra -Wformat -Wformat-security")
set(CMAKE_SHARED_LINKER_FLAGS "-z noexecstack -z relro -z now -pie ")

add_library(gstsvthevcenc SHARED gstsvthevcenc.c)
target_link_libraries(gstsvthevcenc
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${SVT_HEVC_LIBRARIES})

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${GSTREAMER_LIBDIR}")
    message("Install to: " ${GSTREAMER_LIBDIR})
endif()

install (TARGETS gstsvthevcenc
    LIBRARY DESTINATION gstreamer-1.0)

message("Build: " ${CMAKE_BUILD_TYPE})

