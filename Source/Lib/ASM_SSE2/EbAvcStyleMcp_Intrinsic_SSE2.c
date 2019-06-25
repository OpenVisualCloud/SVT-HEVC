/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbAvcStyleMcp_SSE2.h"
#include "EbMcp_SSE2.h" // THIS SHOULD BE _SSE2 in the future
#include "emmintrin.h"
void AvcStyleCopy_SSE2(
    EB_BYTE               refPic,
    EB_U32                srcStride,
    EB_BYTE               dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_BYTE               tempBuf,
    EB_U32                fracPos)
{
    (void)tempBuf;
    (void)fracPos;

    PictureCopyKernel_SSE2(refPic, srcStride, dst, dstStride, puWidth, puHeight);
}

//This function should be removed and replace by AvcStyleCopy_SSE2

void PictureAverageKernel_SSE2_INTRIN(
    EB_BYTE                  src0,
    EB_U32                   src0Stride,
    EB_BYTE                  src1,
    EB_U32                   src1Stride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight)
{
    __m128i xmm_avg1, xmm_avg2, xmm_avg3, xmm_avg4, xmm_avg5, xmm_avg6, xmm_avg7, xmm_avg8;
    EB_U32 y;

    if (areaWidth > 16)
    {
        if (areaWidth == 24)
        {
            for (y = 0; y < areaHeight; y += 2){
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + 16)), _mm_loadl_epi64((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride)), _mm_loadu_si128((__m128i*)(src1 + src1Stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0Stride + 16)), _mm_loadl_epi64((__m128i*)(src1 + src1Stride + 16)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storel_epi64((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + dstStride), xmm_avg3);
                _mm_storel_epi64((__m128i*) (dst + dstStride + 16), xmm_avg4);

                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;
            }
        }
        else if (areaWidth == 32)
        {
            for (y = 0; y < areaHeight; y += 2){

                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 16)), _mm_loadu_si128((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride)), _mm_loadu_si128((__m128i*)(src1 + src1Stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 16)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 16)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + dstStride), xmm_avg3);
                _mm_storeu_si128((__m128i*) (dst + dstStride + 16), xmm_avg4);

                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;
            }
        }
        else if (areaWidth == 48)
        {
            for (y = 0; y < areaHeight; y += 2){
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 16)), _mm_loadu_si128((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 32)), _mm_loadu_si128((__m128i*)(src1 + 32)));

                xmm_avg4 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride)), _mm_loadu_si128((__m128i*)(src1 + src1Stride)));
                xmm_avg5 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 16)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 16)));
                xmm_avg6 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 32)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 32)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + 32), xmm_avg3);
                _mm_storeu_si128((__m128i*) (dst + dstStride), xmm_avg4);
                _mm_storeu_si128((__m128i*) (dst + dstStride + 16), xmm_avg5);
                _mm_storeu_si128((__m128i*) (dst + dstStride + 32), xmm_avg6);

                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;

            }
        }
        else
        {
            for (y = 0; y < areaHeight; y += 2){
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 16)), _mm_loadu_si128((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 32)), _mm_loadu_si128((__m128i*)(src1 + 32)));
                xmm_avg4 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 48)), _mm_loadu_si128((__m128i*)(src1 + 48)));

                xmm_avg5 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride)), _mm_loadu_si128((__m128i*)(src1 + src1Stride)));
                xmm_avg6 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 16)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 16)));
                xmm_avg7 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 32)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 32)));
                xmm_avg8 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 48)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 48)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + 32), xmm_avg3);
                _mm_storeu_si128((__m128i*) (dst + 48), xmm_avg4);

                _mm_storeu_si128((__m128i*) (dst + dstStride), xmm_avg5);
                _mm_storeu_si128((__m128i*) (dst + dstStride + 16), xmm_avg6);
                _mm_storeu_si128((__m128i*) (dst + dstStride + 32), xmm_avg7);
                _mm_storeu_si128((__m128i*) (dst + dstStride + 48), xmm_avg8);

                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;
            }
        }
    }
    else
    {
        if (areaWidth == 16)
        {
            for (y = 0; y < areaHeight; y += 2){
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride)), _mm_loadu_si128((__m128i*)(src1 + src1Stride)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + dstStride), xmm_avg2);

                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;
            }
        }
        else if (areaWidth == 4)
        {
            for (y = 0; y < areaHeight; y += 2){

                xmm_avg1 = _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)src0), _mm_cvtsi32_si128(*(EB_U32 *)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)(src0 + src0Stride)), _mm_cvtsi32_si128(*(EB_U32 *)(src1 + src1Stride)));

                *(EB_U32 *)dst = _mm_cvtsi128_si32(xmm_avg1);
                *(EB_U32 *)(dst + dstStride) = _mm_cvtsi128_si32(xmm_avg2);

                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;
            }
        }
        else if (areaWidth == 8)
        {
            for (y = 0; y < areaHeight; y += 2){

                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)src0), _mm_loadl_epi64((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0Stride)), _mm_loadl_epi64((__m128i*)(src1 + src1Stride)));

                _mm_storel_epi64((__m128i*) dst, xmm_avg1);
                _mm_storel_epi64((__m128i*) (dst + dstStride), xmm_avg2);

                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;
            }
        }
        else
        {
            for (y = 0; y < areaHeight; y += 2){

                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)src0), _mm_loadl_epi64((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)(src0 + 8)), _mm_cvtsi32_si128(*(EB_U32 *)(src1 + 8)));

                xmm_avg3 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0Stride)), _mm_loadl_epi64((__m128i*)(src1 + src1Stride)));
                xmm_avg4 = _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)(src0 + src0Stride + 8)), _mm_cvtsi32_si128(*(EB_U32 *)(src1 + src1Stride + 8)));

                _mm_storel_epi64((__m128i*) dst, xmm_avg1);
                *(EB_U32 *)(dst + 8) = _mm_cvtsi128_si32(xmm_avg2);
                _mm_storel_epi64((__m128i*) (dst + dstStride), xmm_avg3);
                *(EB_U32 *)(dst + dstStride + 8) = _mm_cvtsi128_si32(xmm_avg4);

                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;
            }
        }
    }
}
void PictureAverageKernel1Line_SSE2_INTRIN(
    EB_BYTE                  src0,
    EB_BYTE                  src1,
    EB_BYTE                  dst,
    EB_U32                   areaWidth)
{
    __m128i xmm_avg1, xmm_avg2, xmm_avg3, xmm_avg4;

    if (areaWidth > 16)
    {
        if (areaWidth == 32)
        {
            //for (y = 0; y < areaHeight; y += 2)
            {

                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 16)), _mm_loadu_si128((__m128i*)(src1 + 16)));
                //xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride)), _mm_loadu_si128((__m128i*)(src1 + src1Stride)));
                //xmm_avg4 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 16)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 16)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + 16), xmm_avg2);
                //_mm_storeu_si128((__m128i*) (dst + dstStride), xmm_avg3);
                //_mm_storeu_si128((__m128i*) (dst + dstStride + 16), xmm_avg4);

                //src0 += src0Stride << 1;
                //src1 += src1Stride << 1;
                //dst += dstStride << 1;
            }
        }
        else
        {
            //for (y = 0; y < areaHeight; y += 2)
            {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 16)), _mm_loadu_si128((__m128i*)(src1 + 16)));
                xmm_avg3 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 32)), _mm_loadu_si128((__m128i*)(src1 + 32)));
                xmm_avg4 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + 48)), _mm_loadu_si128((__m128i*)(src1 + 48)));

                //xmm_avg5 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride)), _mm_loadu_si128((__m128i*)(src1 + src1Stride)));
                //xmm_avg6 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 16)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 16)));
                //xmm_avg7 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 32)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 32)));
                //xmm_avg8 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride + 48)), _mm_loadu_si128((__m128i*)(src1 + src1Stride + 48)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                _mm_storeu_si128((__m128i*) (dst + 16), xmm_avg2);
                _mm_storeu_si128((__m128i*) (dst + 32), xmm_avg3);
                _mm_storeu_si128((__m128i*) (dst + 48), xmm_avg4);

                //_mm_storeu_si128((__m128i*) (dst + dstStride), xmm_avg5);
                //_mm_storeu_si128((__m128i*) (dst + dstStride + 16), xmm_avg6);
                //_mm_storeu_si128((__m128i*) (dst + dstStride + 32), xmm_avg7);
                //_mm_storeu_si128((__m128i*) (dst + dstStride + 48), xmm_avg8);

                //src0 += src0Stride << 1;
                //src1 += src1Stride << 1;
                //dst += dstStride << 1;
            }
        }
    }
    else
    {
        if (areaWidth == 16)
        {
            //for (y = 0; y < areaHeight; y += 2)
            {
                xmm_avg1 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)src0), _mm_loadu_si128((__m128i*)src1));
                //xmm_avg2 = _mm_avg_epu8(_mm_loadu_si128((__m128i*)(src0 + src0Stride)), _mm_loadu_si128((__m128i*)(src1 + src1Stride)));

                _mm_storeu_si128((__m128i*) dst, xmm_avg1);
                //_mm_storeu_si128((__m128i*) (dst + dstStride), xmm_avg2);

                //src0 += src0Stride << 1;
                //src1 += src1Stride << 1;
                //dst += dstStride << 1;
            }
        }
        else if (areaWidth == 4)
        {
            //for (y = 0; y < areaHeight; y += 2)
            {

                xmm_avg1 = _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)src0), _mm_cvtsi32_si128(*(EB_U32 *)src1));
                //xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)(src0 + src0Stride)), _mm_cvtsi32_si128(*(EB_U32 *)(src1 + src1Stride)));

                *(EB_U32 *)dst = _mm_cvtsi128_si32(xmm_avg1);
                //*(EB_U32 *)(dst + dstStride) = _mm_cvtsi128_si32(xmm_avg2);

                //src0 += src0Stride << 1;
                //src1 += src1Stride << 1;
                //dst += dstStride << 1;
            }
        }
        else if (areaWidth == 8)
        {
            //for (y = 0; y < areaHeight; y += 2)
            {

                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)src0), _mm_loadl_epi64((__m128i*)src1));
                //xmm_avg2 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0Stride)), _mm_loadl_epi64((__m128i*)(src1 + src1Stride)));

                _mm_storel_epi64((__m128i*) dst, xmm_avg1);
                //_mm_storel_epi64((__m128i*) (dst + dstStride), xmm_avg2);

                //src0 += src0Stride << 1;
                //src1 += src1Stride << 1;
                //dst += dstStride << 1;
            }
        }
        else
        {
            //for (y = 0; y < areaHeight; y += 2)
            {

                xmm_avg1 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)src0), _mm_loadl_epi64((__m128i*)src1));
                xmm_avg2 = _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)(src0 + 8)), _mm_cvtsi32_si128(*(EB_U32 *)(src1 + 8)));

                //xmm_avg3 = _mm_avg_epu8(_mm_loadl_epi64((__m128i*)(src0 + src0Stride)), _mm_loadl_epi64((__m128i*)(src1 + src1Stride)));
                //xmm_avg4 = _mm_avg_epu8(_mm_cvtsi32_si128(*(EB_U32 *)(src0 + src0Stride + 8)), _mm_cvtsi32_si128(*(EB_U32 *)(src1 + src1Stride + 8)));

                _mm_storel_epi64((__m128i*) dst, xmm_avg1);
                *(EB_U32 *)(dst + 8) = _mm_cvtsi128_si32(xmm_avg2);
                //_mm_storel_epi64((__m128i*) (dst + dstStride), xmm_avg3);
                //*(EB_U32 *)(dst + dstStride + 8) = _mm_cvtsi128_si32(xmm_avg4);

                //src0 += src0Stride << 1;
                //src1 += src1Stride << 1;
                //dst += dstStride << 1;
            }
        }
    }
}
