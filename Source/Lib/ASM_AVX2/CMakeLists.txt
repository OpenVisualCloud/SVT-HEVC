# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# ASM_AVX2 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/Codec/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/)
include_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

link_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)

if(UNIX)
	# Intel Linux
	if("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
		SET(CMAKE_C_FLAGS "-fPIC -static-intel -w")
	else()
		SET(CMAKE_C_FLAGS "-march=core-avx2")
	endif()

else()
	# Intel Windows (*Note - The Warning level /W0 should be made to /W4 at some point)
	if("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
		SET(CMAKE_C_FLAGS "/W0 /Qwd10148 /Qwd10010 /Qwd10157")
	else()
		SET(CMAKE_C_FLAGS "/arch:AVX2")	
	endif()

endif()

add_library(ASM_AVX2
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
    EbTransforms_Intrinsic_AVX2.c
)

