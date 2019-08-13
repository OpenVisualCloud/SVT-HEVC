/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "EbMcp_SSE2.h"

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
EB_ALIGN(16) const EB_S16 IntraPredictionConst_SSE2[344]= {
    17, 19, 21, 23, 25, 27, 29, 31,
     1,  3,  5,  7,  9, 11, 13, 15,
    25, 26, 27, 28, 29, 30, 31, 32,
    17, 18, 19, 20, 21, 22, 23, 24,
     9, 10, 11, 12, 13, 14, 15, 16,
     1,  2,  3,  4,  5,  6,  7,  8,
    31, 30, 29, 28, 27, 26, 25, 24,
    23, 22, 21, 20, 19, 18, 17, 16,
    15, 14, 13, 12, 11, 10,  9,  8,
     7,  6,  5,  4,  3,  2,  1,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     1,  1,  1,  1,  1,  1,  1,  1,
     2,  2,  2,  2,  2,  2,  2,  2,
     3,  3,  3,  3,  3,  3,  3,  3,
     4,  4,  4,  4,  4,  4,  4,  4,
     5,  5,  5,  5,  5,  5,  5,  5,
     6,  6,  6,  6,  6,  6,  6,  6,
     7,  7,  7,  7,  7,  7,  7,  7,
     8,  8,  8,  8,  8,  8,  8,  8,
     9,  9,  9,  9,  9,  9,  9,  9,
    10, 10, 10, 10, 10, 10, 10, 10,
    11, 11, 11, 11, 11, 11, 11, 11,
    12, 12, 12, 12, 12, 12, 12, 12,
    13, 13, 13, 13, 13, 13, 13, 13,
    14, 14, 14, 14, 14, 14, 14, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    16, 16, 16, 16, 16, 16, 16, 16,
    17, 17, 17, 17, 17, 17, 17, 17,
    18, 18, 18, 18, 18, 18, 18, 18,
    19, 19, 19, 19, 19, 19, 19, 19,
    20, 20, 20, 20, 20, 20, 20, 20,
    21, 21, 21, 21, 21, 21, 21, 21,
    22, 22, 22, 22, 22, 22, 22, 22,
    23, 23, 23, 23, 23, 23, 23, 23,
    24, 24, 24, 24, 24, 24, 24, 24,
    25, 25, 25, 25, 25, 25, 25, 25,
    26, 26, 26, 26, 26, 26, 26, 26,
    27, 27, 27, 27, 27, 27, 27, 27,
    28, 28, 28, 28, 28, 28, 28, 28,
    29, 29, 29, 29, 29, 29, 29, 29,
    30, 30, 30, 30, 30, 30, 30, 30,
    31, 31, 31, 31, 31, 31, 31, 31,
    32, 32, 32, 32, 32, 32, 32, 32,
};

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
void LumaInterpolationCopy16bit_SSE2(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    (void)firstPassIFDst;
    PictureCopyKernel_SSE2((EB_BYTE)refPic, srcStride*sizeof(EB_U16), (EB_BYTE)dst, dstStride*sizeof(EB_U16), puWidth*sizeof(EB_U16), puHeight);
}

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
void ChromaInterpolationCopy16bit_SSE2(
    EB_U16*               refPic,
    EB_U32                srcStride,
    EB_U16*               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16                *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    (void)firstPassIFDst;
    (void)fracPosx;
    (void)fracPosy;
    PictureCopyKernel_SSE2((EB_BYTE)refPic, srcStride*sizeof(EB_U16), (EB_BYTE)dst, dstStride*sizeof(EB_U16), puWidth*sizeof(EB_U16), puHeight);
}
