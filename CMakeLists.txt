cmake_minimum_required(VERSION 2.8)
set(project_name "svt-hevc")

project(${project_name} C ASM_NASM)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()
    message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
endif()

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
set(SVT_HEVC_SOVERSION 1)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_STANDARD 99)
set(CAN_USE_ASSEMBLER TRUE)
set(CMAKE_NASM_LINK_EXECUTABLE "ld -o <TARGET> <OBJECTS>")
set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS asm)


if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_ASM_NASM_OBJECT_FORMAT macho64)
    set(CMAKE_ASM_NASM_FLAGS "-DUNIX64")
    set(CMAKE_MACOSX_RPATH 1)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_ASM_NASM_FLAGS "-DUNIX64")
    set(CMAKE_ASM_NASM_FLAGS_DEBUG "-g null")
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
    set(CMAKE_EXE_LINKER_FLAGS "-z noexecstack -z relro -z now -pie ")
    set(CMAKE_SHARED_LINKER_FLAGS "-z noexecstack -z relro -z now -pie ")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_ASM_NASM_FLAGS "-DWIN64")
    set(CMAKE_ASM_NASM_FLAGS_DEBUG "")
    if (MSVC)
        set(CMAKE_C_FLAGS " /GS /sdl ")
        set(CMAKE_CXX_FLAGS " /GS /sdl ")
        set(CMAKE_EXE_LINKER_FLAGS " /NXCompat /DynamicBase ")
        set(CMAKE_SHARED_LINKER_FLAGS " /NXCompat /DynamicBase ")
    endif()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2 -O2 -flto -Wall -Wextra -Wformat -Wformat-security -fvisibility=hidden -march=native")
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows") #for Mingw64 support, see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65782
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-asynchronous-unwind-tables")
    endif()
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
endif()

# Prepare for Coveralls
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
if(CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    setup_target_for_coverage(${PROJECT_NAME}_coverage ${PROJECT_TEST_NAME} coverage)
endif()

# Add Subdirectories
add_subdirectory (Source/Lib)
add_subdirectory (Source/App)