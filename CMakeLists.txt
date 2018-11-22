cmake_minimum_required(VERSION 3.5)
set(project_name "svt-hevc")

project(${project_name} C ASM_NASM)

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
