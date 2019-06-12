#
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
#

# ASM_SSSE3 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/
    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec/
    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

set(flags_to_test
    -mssse3
    -msse4.1
    -static-intel)

foreach(cflag ${flags_to_test})
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" cflag_var "${cflag}")
    set(test_c_flag "C_FLAG${cflag_var}")
    check_c_compiler_flag(${cflag} "${test_c_flag}")
    if(${test_c_flag})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
    endif()
endforeach()

if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
endif()

# Utility Source Files
add_library(HEVC_ASM_SSSE3 OBJECT
    EbAvcStyleMcp_SSSE3.h
    EbDeblockingFilter_SSSE3.h
    EbIntraPrediction_SSSE3.h
    EbMcp_SSSE3.h
    EbSaoApplication_SSSE3.h
    EbTransforms_SSSE3.h
    EbAvcStyleMcp_Intrinsic_SSSE3.c
    EbDeblockingFilter16bit_Intrinsic_SSSE3.c
    EbDeblockingFilter_Intrinsic_SSSE3.c
    EbIntraPrediction16bit_Intrinsic_SSSE3.c
    EbMcp_Intrinsic_SSSE3.c
    EbSaoApplication_Intrinsic_SSSE3.c
    EbTransforms_Intrinsic_SSSE3.c)
