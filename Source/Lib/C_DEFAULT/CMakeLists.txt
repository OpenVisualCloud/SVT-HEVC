# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# C_DEFAULT Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/API/)
include_directories (${PROJECT_SOURCE_DIR}/API/OpenMAX/IL/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/System/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/Codec/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/Utility/)

include_directories (${PROJECT_SOURCE_DIR}/Lib/C_DEFAULT/)
#include_directories (${PROJECT_SOURCE_DIR}/Lib/SRM/)

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
add_library(C_DEFAULT
    EbAvcStyleMcp_C.c
    EbComputeMean_C.c
    EbComputeSAD_C.c
    EbDeblockingFilter_C.c
    EbIntraPrediction_C.c
    EbMcp_C.c
    EbMeSadCalculation_C.c
    EbPackUnPack_C.c
    EbPictureOperators_C.c
    EbSampleAdaptiveOffset_C.c
    EbTransforms_C.c
)



