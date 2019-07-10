/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeSAD_h
#define EbComputeSAD_h

#include "EbDefinitions.h"

#include "EbComputeSAD_C.h"
#include "EbComputeSAD_SSE2.h"
#include "EbComputeSAD_AVX2.h"
#include "EbComputeSAD_SadLoopKernel_AVX512.h"

#include "EbUtility.h"

#ifdef __cplusplus
extern "C" {
#endif


    /***************************************
    * Function Ptr Types
    ***************************************/
    typedef EB_U32(*EB_SADKERNELNxM_TYPE)(
        EB_U8  *src,
        EB_U32  srcStride,
        EB_U8  *ref,
        EB_U32  refStride,
        EB_U32  height,
        EB_U32  width);

    static void NxMSadKernelVoidFunc() {}

    typedef void(*EB_SADLOOPKERNELNxM_TYPE)(
        EB_U8  *src,                            // input parameter, source samples Ptr
        EB_U32  srcStride,                      // input parameter, source stride
        EB_U8  *ref,                            // input parameter, reference samples Ptr
        EB_U32  refStride,                      // input parameter, reference stride
        EB_U32  height,                         // input parameter, block height (M)
        EB_U32  width,                          // input parameter, block width (N)
        EB_U64 *bestSad,
        EB_S16 *xSearchCenter,
        EB_S16 *ySearchCenter,
        EB_U32  srcStrideRaw,                   // input parameter, source stride (no line skipping)
        EB_S16 searchAreaWidth,
        EB_S16 searchAreaHeight);

    typedef EB_U32(*EB_SADAVGKERNELNxM_TYPE)(
        EB_U8  *src,
        EB_U32  srcStride,
        EB_U8  *ref1,
        EB_U32  ref1Stride,
        EB_U8  *ref2,
        EB_U32  ref2Stride,
        EB_U32  height,
        EB_U32  width);

    typedef EB_U32(*EB_COMPUTE8X4SAD_TYPE)(
        EB_U8  *src,                            // input parameter, source samples Ptr
        EB_U32  srcStride,                      // input parameter, source stride
        EB_U8  *ref,                            // input parameter, reference samples Ptr  
        EB_U32  refStride);                     // input parameter, reference stride

    typedef EB_U32(*EB_COMPUTE8X8SAD_TYPE)(
        EB_U8  *src,                            // input parameter, source samples Ptr
        EB_U32  srcStride,                      // input parameter, source stride
        EB_U8  *ref,                            // input parameter, reference samples Ptr  
        EB_U32  refStride);                     // input parameter, reference stride

    typedef void(*EB_GETEIGHTSAD8x8)(
        EB_U8   *src,
        EB_U32   srcStride,
        EB_U8   *ref,
        EB_U32   refStride,
        EB_U32  *pBestSad8x8,
        EB_U32  *pBestMV8x8,
        EB_U32  *pBestSad16x16,
        EB_U32  *pBestMV16x16,
        EB_U32   mv,
        EB_U16  *pSad16x16);

    typedef void(*EB_GETEIGHTSAD32x32)(
        EB_U16  *pSad16x16,
        EB_U32  *pBestSad32x32,
        EB_U32  *pBestSad64x64,
        EB_U32  *pBestMV32x32,
        EB_U32  *pBestMV64x64,
        EB_U32   mv);

    /***************************************
    * Function Tables
    ***************************************/
    static EB_SADKERNELNxM_TYPE FUNC_TABLE NxMSadKernel_funcPtrArray[EB_ASM_TYPE_TOTAL][9] =   // [ASM_TYPES][SAD - block height]
    {
        // C_DEFAULT
        {
            /*0 4xM  */ FastLoop_NxMSadKernel,
            /*1 8xM  */ FastLoop_NxMSadKernel,
            /*2 16xM */ FastLoop_NxMSadKernel,
            /*3 24xM */ FastLoop_NxMSadKernel,
            /*4 32xM */ FastLoop_NxMSadKernel,
            /*5 40xM */ FastLoop_NxMSadKernel,
            /*6 48xM */ FastLoop_NxMSadKernel,
            /*7 56xM */ FastLoop_NxMSadKernel,
            /*8 64xM */ FastLoop_NxMSadKernel
        },
        // AVX2
        {
            /*0 4xM  */ Compute4xMSad_SSE2_INTRIN,
            /*1 8xM  */ Compute8xMSad_SSE2_INTRIN,
            /*2 16xM */	Compute16xMSad_SSE2_INTRIN,//Compute16xMSad_AVX2_INTRIN is slower than the SSE2 version
            /*3 24xM */	Compute24xMSad_AVX2_INTRIN,
            /*4 32xM */	Compute32xMSad_AVX2_INTRIN,
            /*5 40xM */ Compute40xMSad_AVX2_INTRIN,
            /*6 48xM */	Compute48xMSad_AVX2_INTRIN,
            /*7 56xM */	Compute56xMSad_AVX2_INTRIN,
            /*8 64xM */ Compute64xMSad_AVX2_INTRIN,
        },
    };

    static EB_SADAVGKERNELNxM_TYPE FUNC_TABLE NxMSadAveragingKernel_funcPtrArray[EB_ASM_TYPE_TOTAL][9] =   // [ASM_TYPES][SAD - block height]
    {
        // C_DEFAULT
        {
            /*0 4xM  */     CombinedAveragingSAD,
            /*1 8xM  */     CombinedAveragingSAD,
            /*2 16xM */     CombinedAveragingSAD,
            /*3 24xM */     CombinedAveragingSAD,
            /*4 32xM */     CombinedAveragingSAD,
            /*5      */     (EB_SADAVGKERNELNxM_TYPE)NxMSadKernelVoidFunc,
            /*6 48xM */     CombinedAveragingSAD,
            /*7      */     (EB_SADAVGKERNELNxM_TYPE)NxMSadKernelVoidFunc,
            /*8 64xM */     CombinedAveragingSAD
        },
        // AVX2
        {
            /*0 4xM  */     CombinedAveraging4xMSAD_SSE2_INTRIN,
            /*1 8xM  */     CombinedAveraging8xMSAD_SSE2_INTRIN,
            /*2 16xM */     CombinedAveraging16xMSAD_SSE2_INTRIN,
            /*3 24xM */     CombinedAveraging24xMSAD_SSE2_INTRIN,
            /*4 32xM */     CombinedAveraging32xMSAD_SSE2_INTRIN,
            /*5      */     (EB_SADAVGKERNELNxM_TYPE)NxMSadKernelVoidFunc,
            /*6 48xM */     CombinedAveraging48xMSAD_SSE2_INTRIN,
            /*7      */     (EB_SADAVGKERNELNxM_TYPE)NxMSadKernelVoidFunc,
            /*8 64xM */     CombinedAveraging64xMSAD_SSE2_INTRIN
        },
    };


    static EB_SADLOOPKERNELNxM_TYPE FUNC_TABLE NxMSadLoopKernel_funcPtrArray[EB_ASM_TYPE_TOTAL] =
    {
        // C_DEFAULT
        SadLoopKernel,
        // AVX2
        SadLoopKernel_AVX2_INTRIN,
    };

#ifdef NON_AVX512_SUPPORT
    static EB_GETEIGHTSAD8x8 FUNC_TABLE GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[EB_ASM_TYPE_TOTAL] =
    {
        // C_DEFAULT
        GetEightHorizontalSearchPointResults_8x8_16x16_PU,
        // AVX2
        GetEightHorizontalSearchPointResults_8x8_16x16_PU_AVX2_INTRIN,
    };

    static EB_GETEIGHTSAD32x32 FUNC_TABLE GetEightHorizontalSearchPointResults_32x32_64x64_funcPtrArray[EB_ASM_TYPE_TOTAL] =
    {
        // C_DEFAULT
        GetEightHorizontalSearchPointResults_32x32_64x64,
        // AVX2
        GetEightHorizontalSearchPointResults_32x32_64x64_PU_AVX2_INTRIN,
    };
#endif
#ifdef __cplusplus
}
#endif

#endif
