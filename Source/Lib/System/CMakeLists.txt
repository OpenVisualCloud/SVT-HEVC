# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# EncoderPipeline Directory CMakeLists.txt

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

if(${COMPILE_AS_CPP} EQUAL 1) 

	file(GLOB SourceFiles
	    "*.c"
	)

	set_source_files_properties( 
		${SourceFiles} 
		PROPERTIES LANGUAGE CXX )
endif(${COMPILE_AS_CPP} EQUAL 1)

add_library(HevcEncoder SHARED 
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
)

target_link_libraries(HevcEncoder
	Utility 
	EncoderLib 
	SystemResourceManager
	C_DEFAULT
	ASM_SSE2
	ASM_SSSE3
	ASM_SSE4_1
	ASM_AVX2)

# Set the Lib PreCompiler Definitions
SET(EncoderPipelineCompileDefinitions "")
get_target_property(HevcEncoder COMPILE_DEFINITIONS EncoderPipelineCompileDefinitions)
# Sets OMX_API to export for the library 
FORCE_ADD_FLAGS(EncoderPipelineCompileDefinitions __OMX_EXPORTS)	
set_target_properties(HevcEncoder PROPERTIES COMPILE_DEFINITIONS ${EncoderPipelineCompileDefinitions})

#********* LINK_FLAGS ************

SET(EncoderPipelineLinkFlags " ")
get_target_property(HevcEncoder LINK_FLAGS EncoderPipelineLinkFlags)

if(UNIX)
	# Linux

	if("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
		FORCE_ADD_FLAGS(EncoderPipelineLinkFlags -static-intel -no-intel-extensions)
	else()
		
	endif()

else()
	if("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
		
	else()
		
	endif()
endif()

set_target_properties(HevcEncoder PROPERTIES LINK_FLAGS ${EncoderPipelineLinkFlags})



