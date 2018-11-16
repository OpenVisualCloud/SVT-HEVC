# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# ASM_SSE4.1 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/Source/API/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/Codec/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

# Utility Source Files
add_library(ASM_SSE4_1
    EbIntraPrediction_SSE4_1.h
	EbPictureOperators_SSE4_1.h
	EbTransforms_SSE4_1.h
    EbIntraPrediction16bit_Intrinsic_SSE4_1.c
    EbPictureOperators_Intrinsic_SSE4_1.c
    EbTransforms_Intrinsic_SSE4_1.c
)



