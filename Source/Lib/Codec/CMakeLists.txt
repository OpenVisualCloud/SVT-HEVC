# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/Source/API/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/Codec/)

include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/)
link_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/Bin/${CMAKE_BUILD_TYPE}/)

# Encoder Lib Source Files
add_library(SvtHevcEnc SHARED  
    EbAdaptiveMotionVectorPrediction.h
    EbAvailability.h
    EbAvcStyleMcp.h
    EbBitstreamUnit.h
    EbCabacContextModel.h
    EbCodingLoop.h
    EbCodingUnit.h
    EbComputeMean.h
    EbComputeSAD.h
    EbDeblockingFilter.h
    EbDefinitions.h
    EbEncDecProcess.h
    EbEncDecResults.h
    EbEncDecSegments.h
    EbEncDecTasks.h
    EbEncHandle.h
    EbEncodeContext.h
    EbEntropyCoding.h
    EbEntropyCodingObject.h
    EbEntropyCodingProcess.h
    EbEntropyCodingResults.h
    EbEntropyCodingUtil.h
    EbFullLoop.h
    EbHmCode.h
    EbInitialRateControlProcess.h
    EbInitialRateControlReorderQueue.h
    EbInitialRateControlResults.h
    EbInterPrediction.h
    EbIntraPrediction.h
    EbLambdaRateTables.h
    EbMcp.h
    EbMdRateEstimation.h
    EbMeSadCalculation.h
    EbModeDecision.h
    EbModeDecisionConfiguration.h
    EbModeDecisionConfigurationProcess.h
    EbModeDecisionProcess.h
    EbModeDecisionSegments.h
    EbMotionEstimation.h
    EbMotionEstimationContext.h
    EbMotionEstimationLcuResults.h
    EbMotionEstimationProcess.h
    EbMotionEstimationResults.h
    EbMotionVectorUnit.h
    EbMvMerge.h
    EbNeighborArrays.h
    EbPacketizationProcess.h
    EbPacketizationReorderQueue.h
    EbPackUnPack.h
    EbPictureAnalysisProcess.h
    EbPictureAnalysisResults.h
    EbPictureBufferDesc.h
    EbPictureControlSet.h
    EbPictureDecisionProcess.h
    EbPictureDecisionQueue.h
    EbPictureDecisionReorderQueue.h
    EbPictureDecisionResults.h
    EbPictureDemuxResults.h
    EbPictureManagerProcess.h
    EbPictureManagerQueue.h
    EbPictureManagerReorderQueue.h
    EbPictureOperators.h
    EbPredictionStructure.h
    EbPredictionUnit.h
    EbRateControlProcess.h
    EbRateControlResults.h
    EbRateControlTables.h
    EbRateControlTasks.h
    EbRateDistortionCost.h
    EbReferenceObject.h
    EbResourceCoordinationProcess.h
    EbResourceCoordinationResults.h
    EbSampleAdaptiveOffset.h
    EbSei.h
    EbSequenceControlSet.h
    EbSourceBasedOperationsProcess.h
    EbSyntaxElements.h
    EbSystemResourceManager.h
    EbThreads.h
    EbTransforms.h
    EbTransformUnit.h
    EbTransQuantBuffers.h
    EbUtility.h
    EbEncDecProcess.c
    EbEncDecResults.c
    EbEncDecSegments.c
    EbEncDecTasks.c
    EbEncHandle.c
    EbEncodeContext.c
    EbEntropyCodingProcess.c
    EbEntropyCodingResults.c
    EbInitialRateControlProcess.c
    EbInitialRateControlReorderQueue.c
    EbInitialRateControlResults.c
    EbModeDecisionConfigurationProcess.c
    EbModeDecisionProcess.c
    EbMotionEstimationContext.c
    EbMotionEstimationProcess.c
    EbMotionEstimationResults.c
    EbPacketizationProcess.c
    EbPacketizationReorderQueue.c
    EbPictureAnalysisProcess.c
    EbPictureAnalysisResults.c
    EbPictureBufferDesc.c
    EbPictureControlSet.c
    EbPictureDecisionProcess.c
    EbPictureDecisionQueue.c
    EbPictureDecisionReorderQueue.c
    EbPictureDecisionResults.c
    EbPictureDemuxResults.c
    EbPictureManagerProcess.c
    EbPictureManagerQueue.c
    EbPictureManagerReorderQueue.c
    EbRateControlProcess.c
    EbRateControlResults.c
    EbRateControlTasks.c
    EbReferenceObject.c
    EbResourceCoordinationProcess.c
    EbResourceCoordinationResults.c
    EbSequenceControlSet.c
    EbSourceBasedOperationsProcess.c
    EbSystemResourceManager.c
    EbThreads.c
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
    EbUtility.c
)
	
target_link_libraries(SvtHevcEnc 
	C_DEFAULT
	ASM_SSE2
	ASM_SSSE3
	ASM_SSE4_1
	ASM_AVX2)

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib)
endif()

configure_file(../pkg-config.pc.in ${CMAKE_BINARY_DIR}/SvtHevcEnc.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/SvtHevcEnc.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(TARGETS SvtHevcEnc DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY ../../API/ DESTINATION include
        FILES_MATCHING PATTERN "*.h")