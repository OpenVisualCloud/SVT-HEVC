/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX512.h"
#ifdef _WIN32
#include <intrin.h>
#endif


//BiPredAveragingOnthefly
#ifndef NON_AVX512_SUPPORT
AVX512_FUNC_TARGET
void BiPredAverageKernel_AVX512_INTRIN(
	EB_BYTE                  src0,
	EB_U32                   src0Stride,
	EB_BYTE                  src1,
	EB_U32                   src1Stride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight)
{
	__m128i xmm_avg1, xmm_avg2, xmm_avg3, xmm_avg4, xmm_avg5, xmm_avg6;
	__m256i xmm_avg1_256,  xmm_avg3_256;
	EB_U32 y;

	if (areaWidth > 16)
	{
		if (areaWidth == 24)
		{
			for (y = 0; y < areaHeight; y += 2) {
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
			for (y = 0; y < areaHeight; y += 2) {

				xmm_avg1_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
				xmm_avg3_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0Stride)), _mm256_loadu_si256((__m256i*)(src1 + src1Stride)));

				_mm256_storeu_si256((__m256i *) dst, xmm_avg1_256);
				_mm256_storeu_si256((__m256i *) (dst + dstStride), xmm_avg3_256);


				src0 += src0Stride << 1;
				src1 += src1Stride << 1;
				dst += dstStride << 1;
			}
		}
		else if (areaWidth == 48)
		{
			for (y = 0; y < areaHeight; y += 2) {
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
			EB_U32 src0Stride1 = src0Stride << 1;
			EB_U32 src1Stride1 = src1Stride << 1;
			EB_U32 dstStride1 = dstStride << 1;

			__m512i xmm_avg1_512, xmm_avg3_512;

			for (EB_U32 y = 0; y < areaHeight; y += 2, src0 += src0Stride1, src1 += src1Stride1, dst += dstStride1) {
				xmm_avg1_512 = _mm512_avg_epu8(_mm512_loadu_si512((__m512i *)src0), _mm512_loadu_si512((__m512i *)src1));
				xmm_avg3_512 = _mm512_avg_epu8(_mm512_loadu_si512((__m512i *)(src0 + src0Stride)), _mm512_loadu_si512((__m512i *)(src1 + src1Stride)));

				_mm512_storeu_si512((__m512i *)dst, xmm_avg1_512);
				_mm512_storeu_si512((__m512i *)(dst + dstStride), xmm_avg3_512);
			}

		}
	}
	else
	{
		if (areaWidth == 16)
		{
			for (y = 0; y < areaHeight; y += 2) {
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
			for (y = 0; y < areaHeight; y += 2) {

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
			for (y = 0; y < areaHeight; y += 2) {

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
			for (y = 0; y < areaHeight; y += 2) {

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
#else

//BiPredAveragingOnthefly
void BiPredAverageKernel_AVX2_INTRIN(
    EB_BYTE                  src0,
    EB_U32                   src0Stride,
    EB_BYTE                  src1,
    EB_U32                   src1Stride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight)
{
    __m128i xmm_avg1, xmm_avg2, xmm_avg3, xmm_avg4, xmm_avg5, xmm_avg6;
    __m256i xmm_avg1_256, xmm_avg2_256, xmm_avg3_256, xmm_avg4_256;
    EB_U32 y;

    if (areaWidth > 16)
    {
        if (areaWidth == 24)
        {
            for (y = 0; y < areaHeight; y += 2) {
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
            for (y = 0; y < areaHeight; y += 2) {

                xmm_avg1_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
                xmm_avg3_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0Stride)), _mm256_loadu_si256((__m256i*)(src1 + src1Stride)));

                _mm256_storeu_si256((__m256i *) dst, xmm_avg1_256);
                _mm256_storeu_si256((__m256i *) (dst + dstStride), xmm_avg3_256);


                src0 += src0Stride << 1;
                src1 += src1Stride << 1;
                dst += dstStride << 1;
            }
        }
        else if (areaWidth == 48)
        {
            for (y = 0; y < areaHeight; y += 2) {
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
            for (y = 0; y < areaHeight; y += 2) {
                xmm_avg1_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)src0), _mm256_loadu_si256((__m256i*)src1));
                xmm_avg2_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + 32)), _mm256_loadu_si256((__m256i*)(src1 + 32)));
                xmm_avg3_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0Stride)), _mm256_loadu_si256((__m256i*)(src1 + src1Stride)));
                xmm_avg4_256 = _mm256_avg_epu8(_mm256_loadu_si256((__m256i*)(src0 + src0Stride + 32)), _mm256_loadu_si256((__m256i*)(src1 + src1Stride + 32)));

                _mm256_storeu_si256((__m256i *) dst, xmm_avg1_256);
                _mm256_storeu_si256((__m256i *) (dst + 32), xmm_avg2_256);

                _mm256_storeu_si256((__m256i *) (dst + dstStride), xmm_avg3_256);
                _mm256_storeu_si256((__m256i *) (dst + dstStride + 32), xmm_avg4_256);

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
            for (y = 0; y < areaHeight; y += 2) {
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
            for (y = 0; y < areaHeight; y += 2) {

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
            for (y = 0; y < areaHeight; y += 2) {

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
            for (y = 0; y < areaHeight; y += 2) {

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

#endif
