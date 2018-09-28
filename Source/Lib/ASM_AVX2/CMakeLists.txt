# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# ASM_AVX2 Directory CMakeLists.txt

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
include_directories (${PROJECT_SOURCE_DIR}/Lib/ASM_AVX2/)
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


if(UNIX)
	# Intel Linux
	if("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
		SET(CMAKE_C_FLAGS "-fPIC -static-intel -w")
	else()
		SET(CMAKE_CXX_FLAGS "-fPIC -Wall -Wextra -msse4.1 -march=core-avx2 -fvisibility=hidden")
		SET(CMAKE_C_FLAGS "-fPIC -Wall -Wextra -msse4.1 -march=core-avx2 -fvisibility=hidden")
		      
	endif()

else()
	# Intel Windows (*Note - The Warning level /W0 should be made to /W4 at some point)
	if("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
		SET(CMAKE_C_FLAGS "/W0 /Qwd10148 /Qwd10010 /Qwd10157")
	else()
		SET(CMAKE_C_FLAGS "-Wall -Wextra -Werror -msse4.1 -march=core-avx2 -fvisibility=hidden")	
		SET(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -msse4.1 -march=core-avx2 -fvisibility=hidden")
	endif()

endif()

# Utility Source Files
add_library(ASM_AVX2
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



