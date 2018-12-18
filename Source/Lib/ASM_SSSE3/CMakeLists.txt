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

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mssse3")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -static-intel -w")
endif()

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
