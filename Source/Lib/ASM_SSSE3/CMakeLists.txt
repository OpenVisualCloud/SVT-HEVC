# 
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
# 

# ASM_SSSE3 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories (${PROJECT_SOURCE_DIR}/Source/API/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/Codec/)

include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/)
include_directories (${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

# Utility Source Files
add_library(ASM_SSSE3
	EbAvcStyleMcp_SSSE3.h
	EbDeblockingFilter_SSSE3.h
	EbIntraPrediction_SSSE3.h
	EbMcp_SSSE3.h
	EbSaoApplication_SSSE3.h
	EbTransforms_SSSE3.h
    EbAvcStyleMcp_Intrinsic_SSSE3.c
    EbDeblockingFilter16bit_Intrinsic_SSSE3.c
    EbDeblockingFilter_Intrinsic_SSSE3.c
    EbIntraPrediction16bit_Intrinsic_SSSE3.c
    EbMcp_Intrinsic_SSSE3.c
    EbSaoApplication_Intrinsic_SSSE3.c
    EbTransforms_Intrinsic_SSSE3.c
)
