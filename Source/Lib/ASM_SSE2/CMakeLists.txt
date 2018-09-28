# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# ASM_SSE2 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/API/)
include_directories (${PROJECT_SOURCE_DIR}/API/OpenMAX/IL/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/System/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/Codec/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/Utility/)

include_directories (${PROJECT_SOURCE_DIR}/Lib/C_DEFAULT/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_SSE2/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/SRM/)


link_directories (${PROJECT_SOURCE_DIR}/Lib/SRM/)

if(${COMPILE_AS_CPP} EQUAL 1) 

	file(GLOB SourceFiles
	    "*.c"
	)

	set_source_files_properties( 
		${SourceFiles} 
		PROPERTIES LANGUAGE CXX )
endif(${COMPILE_AS_CPP} EQUAL 1)

# Utility Source Files
add_library(ASM_SSE2
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



