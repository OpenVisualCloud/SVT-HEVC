# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# ASM_SSE4.1 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/API/)
include_directories (${PROJECT_SOURCE_DIR}/API/OpenMAX/IL/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/System/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/Codec/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/Utility/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/C_DEFAULT/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_SSE2/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_SSSE3/)
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_SSE4_1/)
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
add_library(ASM_SSE4_1
    EbIntraPrediction16bit_Intrinsic_SSE4_1.c
    EbPictureOperators_Intrinsic_SSE4_1.c
    EbTransforms_Intrinsic_SSE4_1.c
)



