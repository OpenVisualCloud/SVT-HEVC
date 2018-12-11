cmake_minimum_required(VERSION 3.5)
set(project_name "svt-hevc")

project(${project_name} C ASM_NASM)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER gcc)
endif()
message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")

if(NOT MAKE_ASM_NASM_COMPILER)
    set(CMAKE_ASM_NASM_COMPILER yasm)
endif()
message(STATUS "CMAKE_ASM_NASM_COMPILER = ${CMAKE_ASM_NASM_COMPILER}")

SET(CMAKE_AR  "gcc-ar")
message(STATUS "CMAKE_AR = ${CMAKE_AR}")

SET(CMAKE_RANLIB  "gcc-ranlib")
message(STATUS "CMAKE_RANLIB = ${CMAKE_RANLIB}")

if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX STREQUAL "")
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "..." FORCE)
endif()
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

if(NOT DEFINED SVT_HEVC_VERSION OR SVT_HEVC_VERSION STREQUAL "")
    set(SVT_HEVC_VERSION "1.2.0")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_STANDARD 99)
set(CAN_USE_ASSEMBLER TRUE)
set(CMAKE_NASM_LINK_EXECUTABLE "ld -o <TARGET> <OBJECTS>")
set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS asm)


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_ASM_NASM_OBJECT_FORMAT macho64)
    set(CMAKE_ASM_NASM_FLAGS "-DUNIX64")
    set(CMAKE_MACOSX_RPATH 1)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_C_FLAGS "-fPIE -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2 -O2 -flto -Wall -Wextra -Wformat -Wformat-security -march=native -fvisibility=hidden")
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_EXE_LINKER_FLAGS "-z noexecstack -z relro -z now -pie ")
    set(CMAKE_SHARED_LINKER_FLAGS "-z noexecstack -z relro -z now -pie ")
    set(CMAKE_ASM_NASM_FLAGS "-DUNIX64")
    set(CMAKE_ASM_NASM_FLAGS_DEBUG "-g null")
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_EXE_LINKER_FLAGS " ")
    set(CMAKE_SHARED_LINKER_FLAGS " ")
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_ASM_NASM_FLAGS "-DWIN64")
    set(CMAKE_ASM_NASM_FLAGS_DEBUG "")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


# Add Subdirectories
add_subdirectory (Source/Lib)
add_subdirectory (Source/App)
add_subdirectory (Source/SimpleApp)

# install libsvt.pc
if(UNIX)
    include(FindPkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        configure_file("libsvt.pc.in" "libsvt.pc" @ONLY)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libsvt.pc" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
    endif()
endif()

# install headers and libs
install(FILES ${CMAKE_SOURCE_DIR}/Source/API/EbApi.h DESTINATION include)
install(FILES ${CMAKE_SOURCE_DIR}/Source/API/EbErrorCodes.h DESTINATION include)
install(FILES ${CMAKE_SOURCE_DIR}/Source/API/EbTime.h DESTINATION include)
install(FILES "${CMAKE_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/libSvtHevcEnc.so" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/")
