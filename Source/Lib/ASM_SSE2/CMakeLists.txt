#
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
#

# ASM_SSE2 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/
    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec/
    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)

set(flags_to_test
    -msse2
    -msse4.1
    -static-intel)

foreach(cflag ${flags_to_test})
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" cflag_var "${cflag}")
    set(test_c_flag "C_FLAG${cflag_var}")
    if(COMPILE_AS_CPP)
        set(test_cxx_flag "CXX_FLAG${cflag_var}")
    endif()
    check_c_compiler_flag(${cflag} "${test_c_flag}")
    if(COMPILE_AS_CPP)
        check_cxx_compiler_flag(${cflag} "${test_cxx_flag}")
    endif()
    if(${test_c_flag})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
    endif()
    if(COMPILE_AS_CPP)
        if(${test_cxx_flag})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cflag}")
        endif()
    endif()
endforeach()

if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
    if(COMPILE_AS_CPP)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    endif()
endif()

set(ASM_SSE2_SOURCE
    EbApplySaoLcu16bit_Intrinsic_SSE2.c
    EbAvcStyleMcp_Intrinsic_SSE2.c
    EbCombinedAveragingSAD_Intrinsic_SSE2.c
    EbComputeMean_Intrinsic_SSE2.c
    EbDeblockingFilter_Intrinsic_SSE2.c
    EbGatherSaoStatistics16bit_Intrinsic_SSE2.c
    EbIntraPrediction16bit_Intrinsic_SSE2.c
    EbIntraPrediction_Intrinsic_SSE2.c
    EbInvTransform_Intrinsic_SSE2.c
    EbMcp16bit_Intrinsic_SSE2.c
    EbMcp_Intrinsic_SSE2.c
    EbMeSadCalculation_Intrinsic_SSE2.c
    EbPackUnPack_Intrinsic_SSE2.c
    EbPictureOperators_Intrinsic_SSE2.c
    EbSaoStatistics_Intrinsic_SSE2.c
    EbTransforms_Intrinsic_SSE2.c)

if(COMPILE_AS_CPP)
    set_source_files_properties(${ASM_SSE2_SOURCE}
        PROPERTIES LANGUAGE CXX)
endif()

# Utility Source Files
add_library(HEVC_ASM_SSE2 OBJECT
    ${ASM_SSE2_SOURCE}
    EbAvcStyleMcp_SSE2.h
    EbComputeMean_SSE2.h
    EbComputeSAD_SSE2.h
    EbDeblockingFilter_SSE2.h
    EbIntraPrediction_SSE2.h
    EbIntrinMacros16bit_SSE2.h
    EbIntrinMacros_SSE2.h
    EbMcp_SSE2.h
    EbMeSadCalculation_SSE2.h
    EbPackUnPack_SSE2.h
    EbPictureOperators_SSE2.h
    EbSampleAdaptiveOffset_SSE2.h
    EbTransforms_SSE2.h
    EbGatherSaoStatistics16bit_SSE2.asm
    EbPictureOperators_SSE2.asm
    x64inc.asm
    x64Macro.asm
    x64RegisterUtil.asm)
