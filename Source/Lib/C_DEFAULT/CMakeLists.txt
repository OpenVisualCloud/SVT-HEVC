#
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
#

# C_DEFAULT Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/
    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec/
    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/)

# Utility Source Files
set(C_DEFAULT_SOURCE
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
    EbTransforms_C.c)

if(COMPILE_AS_CPP)
    set_source_files_properties(${C_DEFAULT_SOURCE}
        PROPERTIES LANGUAGE CXX)
endif()

add_library(HEVC_C_DEFAULT OBJECT
    ${C_DEFAULT_SOURCE}
    EbAvcStyleMcp_C.h
    EbComputeMean_C.h
    EbComputeSAD_C.h
    EbDeblockingFilter_C.h
    EbIntraPrediction_C.h
    EbMcp_C.h
    EbMeSadCalculation_C.h
    EbPackUnPack_C.h
    EbPictureOperators_C.h
    EbSampleAdaptiveOffset_C.h
    EbTransforms_C.h)
