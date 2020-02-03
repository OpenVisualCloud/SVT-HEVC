/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "emmintrin.h"
#include "EbComputeSAD_SSE2.h"

EB_U32 Compute4xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width)                          // input parameter, block width (N)
{
    EB_U32 y;
    (void)width;
    __m128i xmm_sad = _mm_setzero_si128();

    for (y = 0; y < height; y+=4) {

        xmm_sad = _mm_add_epi16(xmm_sad, _mm_sad_epu8(_mm_cvtsi32_si128(*(EB_U32*)src), _mm_cvtsi32_si128(*(EB_U32*)ref)));
        xmm_sad = _mm_add_epi16(xmm_sad, _mm_sad_epu8(_mm_cvtsi32_si128(*(EB_U32*)(src+srcStride)), _mm_cvtsi32_si128(*(EB_U32*)(ref+refStride))));
        xmm_sad = _mm_add_epi16(xmm_sad, _mm_sad_epu8(_mm_cvtsi32_si128(*(EB_U32*)(src+(srcStride << 1))), _mm_cvtsi32_si128(*(EB_U32*)(ref+(refStride << 1)))));
        xmm_sad = _mm_add_epi16(xmm_sad, _mm_sad_epu8(_mm_cvtsi32_si128(*(EB_U32*)(src+3*srcStride)), _mm_cvtsi32_si128(*(EB_U32*)(ref+ 3*refStride))));

        src += (srcStride << 2);
        ref += (refStride << 2);
    }
    return _mm_cvtsi128_si32(xmm_sad);
}

EB_U32 Compute8xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width)                          // input parameter, block width (N)
{
    EB_U32 y;
    (void)width;
    __m128i xmm_sad = _mm_setzero_si128();

    for (y = 0; y < height; y+=4) {

        xmm_sad = _mm_add_epi16(xmm_sad, _mm_sad_epu8( _mm_loadl_epi64((__m128i*)src), _mm_loadl_epi64((__m128i*)ref)));
        xmm_sad = _mm_add_epi16(xmm_sad, _mm_sad_epu8( _mm_loadl_epi64((__m128i*)(src+srcStride)), _mm_loadl_epi64((__m128i*)(ref+refStride))));
        xmm_sad = _mm_add_epi16(xmm_sad, _mm_sad_epu8( _mm_loadl_epi64((__m128i*)(src+(srcStride << 1))), _mm_loadl_epi64((__m128i*)(ref+(refStride << 1)))));
        xmm_sad = _mm_add_epi16(xmm_sad, _mm_sad_epu8(_mm_loadl_epi64((__m128i*)(src + 3 * srcStride)), _mm_loadl_epi64((__m128i*)(ref + 3 * refStride))));

        src += (srcStride << 2);
        ref += (refStride << 2);
    }
    return _mm_cvtsi128_si32(xmm_sad);
}


EB_U32 Compute16xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width)                          // input parameter, block width (N)
{
    __m128i xmm_sad = _mm_setzero_si128();
    EB_U32 y;
    (void)width;

    for (y = 0; y < height; y += 4) {

        xmm_sad = _mm_add_epi32(xmm_sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src)), _mm_loadu_si128((__m128i*)(ref))));
        xmm_sad = _mm_add_epi32(xmm_sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride)), _mm_loadu_si128((__m128i*)(ref + refStride))));
        xmm_sad = _mm_add_epi32(xmm_sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + (srcStride << 1))), _mm_loadu_si128((__m128i*)(ref + (refStride << 1)))));
        xmm_sad = _mm_add_epi32(xmm_sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 3*srcStride)), _mm_loadu_si128((__m128i*)(ref + 3*refStride))));

        src += (srcStride << 2);
        ref += (refStride << 2);
    }

    xmm_sad = _mm_add_epi32(xmm_sad, _mm_srli_si128(xmm_sad, 8));
    return _mm_cvtsi128_si32(xmm_sad);
}


EB_U32 Compute64xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width)                        // input parameter, block width (N)
{
    __m128i sad;
    (void)width;
    EB_U32 y;

    sad = _mm_setzero_si128();

    for (y = 0; y < height; y+=4) {

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src)), _mm_loadu_si128((__m128i*)(ref))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+16)), _mm_loadu_si128((__m128i*)(ref+16))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+32)), _mm_loadu_si128((__m128i*)(ref+32))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+48)), _mm_loadu_si128((__m128i*)(ref+48))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+srcStride)), _mm_loadu_si128((__m128i*)(ref+refStride))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+srcStride+16)), _mm_loadu_si128((__m128i*)(ref+refStride+16))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+srcStride+32)), _mm_loadu_si128((__m128i*)(ref+refStride+32))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+srcStride+48)), _mm_loadu_si128((__m128i*)(ref+refStride+48))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+2*srcStride)), _mm_loadu_si128((__m128i*)(ref+2*refStride))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+2*srcStride+16)), _mm_loadu_si128((__m128i*)(ref+2*refStride+16))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+2*srcStride+32)), _mm_loadu_si128((__m128i*)(ref+2*refStride+32))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+2*srcStride+48)), _mm_loadu_si128((__m128i*)(ref+2*refStride+48))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+3 * srcStride )), _mm_loadu_si128((__m128i*)(ref+3 * refStride))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+3 * srcStride +16)), _mm_loadu_si128((__m128i*)(ref+3 * refStride+16))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+3 * srcStride +32)), _mm_loadu_si128((__m128i*)(ref+3 * refStride+32))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+3 * srcStride +48)), _mm_loadu_si128((__m128i*)(ref+3 * refStride+48))));

        src += 4 * srcStride;
        ref += 4 * refStride;
    }

    sad = _mm_add_epi32(sad, _mm_srli_si128(sad, 8));
    return _mm_cvtsi128_si32(sad);
}


EB_U32 CombinedAveraging4xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width)
{
    __m128i sad0,sad1;
    EB_U32 y;
    (void)width;
    sad0 = sad1 = _mm_setzero_si128();

    for (y = 0; y < height; y+=2) {

        sad0 = _mm_add_epi32(sad0, _mm_sad_epu8(_mm_cvtsi32_si128(*(EB_U32 *)src), _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)ref1), _mm_cvtsi32_si128(*(EB_U32 *)ref2))));

        sad1 = _mm_add_epi32(sad1, _mm_sad_epu8(_mm_cvtsi32_si128(*(EB_U32 *)(src+srcStride)), _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)(ref1+ref1Stride)), _mm_cvtsi32_si128(*(EB_U32 *)(ref2+ref2Stride)))));
        src  += srcStride  << 1;
        ref1 += ref1Stride << 1;
        ref2 += ref2Stride << 1;
    }
    return _mm_cvtsi128_si32(_mm_add_epi32(sad0, sad1));
}


EB_U32 CombinedAveraging8xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width)
{
    __m128i sad0, sad1;
    EB_U32 y;
    (void)width;

    sad0 = sad1 = _mm_setzero_si128();

    for (y = 0; y < height; y += 2) {

        sad0 = _mm_add_epi32(sad0, _mm_sad_epu8(_mm_loadl_epi64((__m128i*)src), _mm_avg_epu8(_mm_loadl_epi64((__m128i*)ref1), _mm_loadl_epi64((__m128i*)ref2))));
        sad1 = _mm_add_epi32(sad1, _mm_sad_epu8(_mm_loadl_epi64((__m128i*)(src + srcStride)), _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(ref1 + ref1Stride)), _mm_loadl_epi64((__m128i*)(ref2 + ref2Stride)))));
        src += srcStride << 1;
        ref1 += ref1Stride << 1;
        ref2 += ref2Stride << 1;

    }
    return _mm_cvtsi128_si32(_mm_add_epi32(sad0, sad1));
}


EB_U32 CombinedAveraging16xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width)
{
    __m128i sad0, sad1, sad;
    EB_U32 y;
    (void)width;

    sad0 = sad1 = _mm_setzero_si128();

    for (y = 0; y < height; y += 2) {

        sad0 = _mm_add_epi32(sad0, _mm_sad_epu8(_mm_loadu_si128((__m128i*)src), _mm_avg_epu8(_mm_loadu_si128((__m128i*)ref1), _mm_loadu_si128((__m128i*)ref2))));
        sad1 = _mm_add_epi32(sad1, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + ref1Stride)), _mm_loadu_si128((__m128i*)(ref2 + ref2Stride)))));
        src += srcStride << 1;
        ref1 += ref1Stride << 1;
        ref2 += ref2Stride << 1;
    }

    sad = _mm_add_epi32(sad0, sad1);
    sad = _mm_add_epi32(sad, _mm_srli_si128(sad, 8));
    return _mm_cvtsi128_si32(sad);
}


EB_U32 CombinedAveraging24xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width)
{
    __m128i sad0, sad1, sad2, sad3, sad;
    EB_U32 y;
    (void)width;
    sad0 = sad1 = sad2 = sad3 = _mm_setzero_si128();
    for (y = 0; y < height; y += 2) {

        sad0 = _mm_add_epi32(sad0, _mm_sad_epu8(_mm_loadu_si128((__m128i*)src), _mm_avg_epu8(_mm_loadu_si128((__m128i*)ref1), _mm_loadu_si128((__m128i*)ref2))));
        sad1 = _mm_add_epi32(sad1, _mm_sad_epu8(_mm_loadl_epi64((__m128i*)(src + 16)), _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(ref1 + 16)), _mm_loadl_epi64((__m128i*)(ref2 + 16)))));
        sad2 = _mm_add_epi32(sad2, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + ref1Stride)), _mm_loadu_si128((__m128i*)(ref2 + ref2Stride)))));
        sad3 = _mm_add_epi32(sad3, _mm_sad_epu8(_mm_loadl_epi64((__m128i*)(src + srcStride + 16)), _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(ref1 + ref1Stride + 16)), _mm_loadl_epi64((__m128i*)(ref2 + ref2Stride + 16)))));
        src += srcStride << 1;
        ref1 += ref1Stride << 1;
        ref2 += ref2Stride << 1;
    }
    sad = _mm_add_epi32(_mm_add_epi32(sad0, sad1), _mm_add_epi32(sad2, sad3));
    sad = _mm_add_epi32(sad, _mm_srli_si128(sad, 8));
    return _mm_cvtsi128_si32(sad);
}


EB_U32 CombinedAveraging32xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width)
{
    __m128i sad0, sad1, sad2, sad3, sad;
    EB_U32 y;
    (void)width;
    sad0 = sad1 = sad2 = sad3 = _mm_setzero_si128();
    for (y = 0; y < height; y += 2) {

        sad0 = _mm_add_epi32(sad0, _mm_sad_epu8(_mm_loadu_si128((__m128i*)src), _mm_avg_epu8(_mm_loadu_si128((__m128i*)ref1), _mm_loadu_si128((__m128i*)ref2))));
        sad1 = _mm_add_epi32(sad1, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 16)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + 16)), _mm_loadu_si128((__m128i*)(ref2 + 16)))));
        sad2 = _mm_add_epi32(sad2, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + ref1Stride)), _mm_loadu_si128((__m128i*)(ref2 + ref2Stride)))));
        sad3 = _mm_add_epi32(sad3, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride + 16)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + ref1Stride + 16)), _mm_loadu_si128((__m128i*)(ref2 + ref2Stride + 16)))));
        src += srcStride << 1;
        ref1 += ref1Stride << 1;
        ref2 += ref2Stride << 1;
    }
    sad = _mm_add_epi32(_mm_add_epi32(sad0, sad1), _mm_add_epi32(sad2, sad3));
    sad = _mm_add_epi32(sad, _mm_srli_si128(sad, 8));
    return _mm_cvtsi128_si32(sad);
}


EB_U32 CombinedAveraging48xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width)
{
    __m128i sad;
    EB_U32 y;
    (void)width;

    sad = _mm_setzero_si128();

    for (y = 0; y < height; y+=4) {
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1)), _mm_loadu_si128((__m128i*)(ref2)))));
        sad =  _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+16)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+16)), _mm_loadu_si128((__m128i*)(ref2+16)))));
        sad  = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+32)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+32)), _mm_loadu_si128((__m128i*)(ref2+32)))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+srcStride)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+ref1Stride)), _mm_loadu_si128((__m128i*)(ref2+ref2Stride)))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+srcStride+16)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+ref1Stride+16)), _mm_loadu_si128((__m128i*)(ref2+ref2Stride+16)))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+srcStride+32)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+ref1Stride+32)), _mm_loadu_si128((__m128i*)(ref2+ref2Stride+32)))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+2*srcStride)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+2*ref1Stride)), _mm_loadu_si128((__m128i*)(ref2+2*ref2Stride)))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+2*srcStride+16)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+2*ref1Stride+16)), _mm_loadu_si128((__m128i*)(ref2+2*ref2Stride+16)))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+2*srcStride+32)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+2*ref1Stride+32)), _mm_loadu_si128((__m128i*)(ref2+2*ref2Stride+32)))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+3*srcStride)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+3*ref1Stride)), _mm_loadu_si128((__m128i*)(ref2+3*ref2Stride)))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+3*srcStride+16)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+3*ref1Stride+16)), _mm_loadu_si128((__m128i*)(ref2+3*ref2Stride+16)))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src+3*srcStride+32)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1+3*ref1Stride+32)), _mm_loadu_si128((__m128i*)(ref2+3*ref2Stride+32)))));

        src += (srcStride << 2);
        ref1 += (ref1Stride << 2);
        ref2 += (ref2Stride << 2);
    }

    sad = _mm_add_epi32(sad, _mm_srli_si128(sad, 8));
    return _mm_cvtsi128_si32(sad);
}

/**************************************************************************************************************
Compute32xMSad_SSE2_INTRIN
**************************************************************************************************************/

EB_U32 Compute32xMSad_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref,
    EB_U32  refStride,
    EB_U32  height,
    EB_U32  width)
{
    __m128i sad;
    (void)width;
    EB_U32 y;

    sad = _mm_setzero_si128();

    for (y = 0; y < height; y += 4) {

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src)), _mm_loadu_si128((__m128i*)(ref))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 16)), _mm_loadu_si128((__m128i*)(ref + 16))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride)), _mm_loadu_si128((__m128i*)(ref + refStride))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride + 16)), _mm_loadu_si128((__m128i*)(ref + refStride + 16))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 2 * srcStride)), _mm_loadu_si128((__m128i*)(ref + 2 * refStride))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 2 * srcStride + 16)), _mm_loadu_si128((__m128i*)(ref + 2 * refStride + 16))));

        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + (srcStride * 3))), _mm_loadu_si128((__m128i*)(ref + (refStride * 3)))));
        sad = _mm_add_epi32(sad, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + (srcStride * 3) + 16)), _mm_loadu_si128((__m128i*)(ref + (refStride * 3) + 16))));

        src += 4 * srcStride;
        ref += 4 * refStride;
    }

    sad = _mm_add_epi32(sad, _mm_srli_si128(sad, 8));
    return _mm_cvtsi128_si32(sad);
}

EB_U32 CombinedAveraging64xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width)
{
    __m128i sad0, sad1, sad2, sad3, sad;
    EB_U32 y;
    (void)width;
    sad0 = sad1 = sad2 = sad3 = _mm_setzero_si128();
    for (y = 0; y < height; y += 2) {
        sad0 = _mm_add_epi32(sad0, _mm_sad_epu8(_mm_loadu_si128((__m128i*)src), _mm_avg_epu8(_mm_loadu_si128((__m128i*)ref1), _mm_loadu_si128((__m128i*)ref2))));
        sad1 = _mm_add_epi32(sad1, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 16)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + 16)), _mm_loadu_si128((__m128i*)(ref2 + 16)))));
        sad2 = _mm_add_epi32(sad2, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 32)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + 32)), _mm_loadu_si128((__m128i*)(ref2 + 32)))));
        sad3 = _mm_add_epi32(sad3, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 48)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + 48)), _mm_loadu_si128((__m128i*)(ref2 + 48)))));
        sad0 = _mm_add_epi32(sad0, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + ref1Stride)), _mm_loadu_si128((__m128i*)(ref2 + ref2Stride)))));
        sad1 = _mm_add_epi32(sad1, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride + 16)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + ref1Stride + 16)), _mm_loadu_si128((__m128i*)(ref2 + ref2Stride + 16)))));
        sad2 = _mm_add_epi32(sad2, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride + 32)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + ref1Stride + 32)), _mm_loadu_si128((__m128i*)(ref2 + ref2Stride + 32)))));
        sad3 = _mm_add_epi32(sad3, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride + 48)), _mm_avg_epu8(_mm_loadu_si128((__m128i*)(ref1 + ref1Stride + 48)), _mm_loadu_si128((__m128i*)(ref2 + ref2Stride + 48)))));
        src += srcStride << 1;
        ref1 += ref1Stride << 1;
        ref2 += ref2Stride << 1;
    }
    sad = _mm_add_epi32(_mm_add_epi32(sad0, sad1), _mm_add_epi32(sad2, sad3));
    sad = _mm_add_epi32(sad, _mm_srli_si128(sad, 8));
    return _mm_cvtsi128_si32(sad);
}