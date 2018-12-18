# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# ASM_SSE2 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/Source/API/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/Codec/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -static-intel -w")
endif()

# Utility Source Files
add_library(ASM_SSE2
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
    EbApplySaoLcu16bit_Intrinsic_SSE2.c
    EbAvcStyleMcp_Intrinsic_SSE2.c
    EbCombinedAveragingSAD_Intrinsic_SSE2.c
    EbComputeMean_Intrinsic_SSE2.c
    EbDeblockingFilter_Intrinsic_SSE2.c
    EbGatherSaoStatistics16bit_Intrinsic_SSE2.c
    EbGatherSaoStatistics16bit_SSE2.asm
    EbIntraPrediction16bit_Intrinsic_SSE2.c
    EbIntraPrediction_Intrinsic_SSE2.c
    EbInvTransform_Intrinsic_SSE2.c
    EbMcp16bit_Intrinsic_SSE2.c
    EbMcp_Intrinsic_SSE2.c
    EbMeSadCalculation_Intrinsic_SSE2.c
    EbPackUnPack_Intrinsic_SSE2.c
    EbPictureOperators_Intrinsic_SSE2.c
    EbPictureOperators_SSE2.asm
    EbSaoStatistics_Intrinsic_SSE2.c
    EbTransforms_Intrinsic_SSE2.c
    x64inc.asm
    x64Macro.asm
    x64RegisterUtil.asm
)

