#
# Copyright(c) 2018 Intel Corporation
# SPDX - License - Identifier: BSD - 2 - Clause - Patent
#

# ASM_SSE4.1 Directory CMakeLists.txt

# Include Encoder Subdirectories
include_directories(${PROJECT_SOURCE_DIR}/Source/API/
    ${PROJECT_SOURCE_DIR}/Source/Lib/Codec/
    ${PROJECT_SOURCE_DIR}/Source/Lib/C_DEFAULT/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE2/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSSE3/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_SSE4_1/
    ${PROJECT_SOURCE_DIR}/Source/Lib/ASM_AVX2/)

set(flags_to_test
    -msse4.1
    -static-intel)

foreach(cflag ${flags_to_test})
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" cflag_var "${cflag}")
    set(test_c_flag "C_FLAG${cflag_var}")
    if(COMPILE_AS_CPP)
        set(test_cxx_flag "CXX_FLAG${cflag_var}")
    endif()
    check_c_compiler_flag(${cflag} "${test_c_flag}")
    if(COMPILE_AS_CPP)
        check_cxx_compiler_flag(${cflag} "${test_cxx_flag}")
    endif()
    if(${test_c_flag})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}")
    endif()
    if(COMPILE_AS_CPP)
        if(${test_cxx_flag})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cflag}")
        endif()
    endif()
endforeach()

if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
    if(COMPILE_AS_CPP)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    endif()
endif()

set(ASM_SSE4_1_SOURCE
    EbIntraPrediction16bit_Intrinsic_SSE4_1.c
    EbPictureOperators_Intrinsic_SSE4_1.c
    EbTransforms_Intrinsic_SSE4_1.c)

if(COMPILE_AS_CPP)
    set_source_files_properties(${ASM_SSE4_1_SOURCE}
        PROPERTIES LANGUAGE CXX)
endif()

# Utility Source Files
add_library(HEVC_ASM_SSE4_1 OBJECT
    ${ASM_SSE4_1_SOURCE}
    EbIntraPrediction_SSE4_1.h
    EbPictureOperators_SSE4_1.h
    EbTransforms_SSE4_1.h)
