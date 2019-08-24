#
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
#

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/
    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec/
    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

link_directories(${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/
    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

# Encoder Lib Source Files
set(Codec_Source
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
    EbUtility.c)

if(COMPILE_AS_CPP)
    set_source_files_properties(${Codec_Source}
        PROPERTIES LANGUAGE CXX)
endif()

add_library(SvtHevcEnc
    ${Codec_Source}
    $<TARGET_OBJECTS:HEVC_C_DEFAULT>
    $<TARGET_OBJECTS:HEVC_ASM_SSE2>
    $<TARGET_OBJECTS:HEVC_ASM_SSSE3>
    $<TARGET_OBJECTS:HEVC_ASM_SSE4_1>
    $<TARGET_OBJECTS:HEVC_ASM_AVX2>
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
    EbUtility.h)

if(UNIX)
    set(LIBS "-lpthread")
    # Set version number for SONAME.
    set_target_properties(SvtHevcEnc
        PROPERTIES SOVERSION ${SVT_HEVC_SOVERSION})
    # Pass version script to ensure that only global-namespace symbols
    # are exported.
    set(VERSION_SCRIPT ${PROJECT_SOURCE_DIR}/Source/API/EbApi.version)
    if(NOT APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script=${VERSION_SCRIPT}")
    endif()
    set_target_properties(SvtHevcEnc
        PROPERTIES LINK_DEPENDS ${VERSION_SCRIPT})
endif()

if(COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU")
        set(LIBS "-lgcov ${LIBS}")
    else(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(LIBS "--coverage ${LIBS}")
    endif()
endif()

add_dependencies(SvtHevcEnc
    HEVC_C_DEFAULT
    HEVC_ASM_SSE2
    HEVC_ASM_SSSE3
    HEVC_ASM_SSE4_1
    HEVC_ASM_AVX2)

configure_file(../pkg-config.pc.in ${CMAKE_BINARY_DIR}/SvtHevcEnc.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/SvtHevcEnc.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(TARGETS SvtHevcEnc
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
