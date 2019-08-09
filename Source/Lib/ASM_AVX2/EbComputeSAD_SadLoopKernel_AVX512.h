/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeSAD_AVX512_h
#define EbComputeSAD_AVX512_h

#include "EbComputeSAD_AVX2.h"
#include "EbMotionEstimation.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifndef NON_AVX512_SUPPORT
void SadLoopKernel_AVX512_HmeL0_INTRIN(
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
#else
    void SadLoopKernel_AVX2_HmeL0_INTRIN(
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
#endif

#ifndef NON_AVX512_SUPPORT
extern void GetEightHorizontalSearchPointResultsAll85PUs_AVX512_INTRIN(
        MeContext_t             *contextPtr,
        EB_U32                   listIndex,
        EB_U32                   searchRegionIndex,
        EB_U32                   xSearchIndex,
        EB_U32                   ySearchIndex
    );
#endif


#ifdef __cplusplus
}
#endif
#endif // EbComputeSAD_AVX512_h

