#
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
#

# ASM_AVX2 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/
    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec/
    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)
link_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)

set(flags_to_test
    -mavx2
    -static-intel
    /Qwd10148
    /Qwd10010
    /Qwd10157)

foreach(cflag ${flags_to_test})
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" cflag_var "${cflag}")
    set(test_c_flag "C_FLAG${cflag_var}")
    check_c_compiler_flag(${cflag} "${test_c_flag}")
    if(${test_c_flag})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
    endif()
endforeach()

if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2")
elseif(CMAKE_C_COMPILER_ID STREQUAL "Intel")
    if(WIN32)
        # Intel Windows (*Note - The Warning level /W0 should be made to /W4 at some point)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
    endif()
endif()

if(CMAKE_C_COMPILER_VERSION VERSION_EQUAL 5.4)
    string(REPLACE "-flto" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif()

add_library(HEVC_ASM_AVX2 OBJECT
    EbCombinedAveragingSAD_Intrinsic_AVX2.h
    EbCombinedAveragingSAD_Intrinsic_AVX512.h
    EbComputeSAD_AVX2.h
    EbComputeSAD_SadLoopKernel_AVX512.h
    EbIntraPrediction_AVX2.h
    EbMcp_AVX2.h
    EbNoiseExtractAVX2.h
    EbPackUnPack_Intrinsic_AVX2.h
    EbPictureOperators_AVX2.h
    EbTransforms_AVX2.h
    EbCombinedAveragingSAD_Intrinsic_AVX2.c
    EbCombinedAveragingSAD_Intrinsic_AVX512.c
    EbComputeSAD_Intrinsic_AVX2.c
    EbComputeSAD_SadLoopKernel_AVX512.c
    EbIntraPrediction_Intrinsic_AVX2.c
    EbMCP16bit_Intrinsic_AVX2.c
    EbNoiseExtractAVX2.c
    EbPackUnPack_Intrinsic_AVX2.c
    EbPictureOperators_Intrinsic_AVX2.c
    EbTransforms_Intrinsic_AVX2.c)
