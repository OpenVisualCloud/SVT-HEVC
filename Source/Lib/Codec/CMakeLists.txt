# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/API/)
include_directories (${PROJECT_SOURCE_DIR}/API/OpenMAX/IL/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/SRM/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/System/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/Codec/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/Utility/)

include_directories (${PROJECT_SOURCE_DIR}/Lib/C_DEFAULT/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_SSE2/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_SSSE3/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_SSE4_1/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_AVX2/)

link_directories (${PROJECT_SOURCE_DIR}/Lib/SRM/)

if(${COMPILE_AS_CPP} EQUAL 1) 

	file(GLOB SourceFiles
	    "*.c"
	)

	set_source_files_properties( 
		${SourceFiles} 
		PROPERTIES LANGUAGE CXX )
endif(${COMPILE_AS_CPP} EQUAL 1)

# Encoder Lib Source Files
add_library(EncoderLib 
    EbAdaptiveMotionVectorPrediction.c
    EbAvailability.c
    EbAvcStyleMcp.c
    EbBitstreamUnit.c
    EbCabacContextModel.c
    EbCodingLoop.c
    EbCodingUnit.c
    EbCoeffEstimation_Intrinsic.c
    EbDeblockingFilter.c
    EbEntropyCoding.c
    EbEntropyCodingUtil.c
    EbFullLoop.c
    EbHmCode.c
    EbInterPrediction.c
    EbIntraPrediction.c
    EbMcp.c
    EbMcpTables.c
    EbMdRateEstimation.c
    EbModeDecision.c
    EbModeDecisionConfiguration.c
    EbMotionEstimation.c
    EbNeighborArrays.c
    EbPictureOperators.c
    EbPredictionStructure.c
    EbProductCodingLoop.c
    EbRateControlTables.c
    EbRateDistortionCost.c
    EbSampleAdaptiveOffsetGenerationDecision.c
    EbSei.c
    EbTransforms.c
    EbTransQuantBuffers.c
)
	
target_link_libraries(EncoderLib
	Utility
	C_DEFAULT
	ASM_SSE2
	ASM_SSSE3
	ASM_SSE4_1
	ASM_AVX2)



