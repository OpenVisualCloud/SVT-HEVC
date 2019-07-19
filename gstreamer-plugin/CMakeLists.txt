cmake_minimum_required(VERSION 2.8)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
project(gst-svt-hevc C)

include(CheckCCompilerFlag)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=1.8)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0>=1.8)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.8)
pkg_check_modules(SVT_HEVC REQUIRED SvtHevcEnc>=1.3)
include_directories(${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_BASE_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    ${SVT_HEVC_INCLUDE_DIRS})

set(flags_to_test
    -Wextra
    -Wformat
    -Wformat-security
    -fstack-protector-strong
    -fPIE
    -fPIC
    -flto
    -D_FORTIFY_SOURCE=2)
if(NOT MSVC)
    list(INSERT flags_to_test 0 -Wall)
else()
    list(APPEND flags_to_test /MP)
endif()
set(release_flags_to_test
    -O2
    -O3)
set(debug_flags_to_test
    -g
    -O0
    -Og)

foreach(flag ${flags_to_test};${release_flags_to_test};${debug_flags_to_test})
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" flag_var "${flag}")
    set(test_c_flag "C_FLAG${flag_var}")
    check_c_compiler_flag(${flag} "${test_c_flag}")
    list(FIND release_flags_to_test ${flag} r_flag)
    list(FIND debug_flags_to_test ${flag} d_flag)
    if(${test_c_flag})
        if(NOT r_flag EQUAL -1)
            set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${flag}")
        elseif(NOT d_flag EQUAL -1)
            set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${flag}")
        else()
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
        endif()
    endif()
endforeach()

if(UNIX AND NOT APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -z noexecstack -z relro -z now -pie")
endif()

add_library(gstsvthevcenc SHARED gstsvthevcenc.c)
target_link_libraries(gstsvthevcenc
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${SVT_HEVC_LIBRARIES})

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${GSTREAMER_LIBDIR}")
    message("Install to: " ${GSTREAMER_LIBDIR})
endif()

install(TARGETS gstsvthevcenc
    LIBRARY DESTINATION gstreamer-1.0)

message("Build: " ${CMAKE_BUILD_TYPE})
