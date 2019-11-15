/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "immintrin.h"
#include "EbMcp_AVX2.h"
#include "EbMcp_SSE2.h"
#include "EbDefinitions.h"

EB_EXTERN EB_ALIGN(16) const EB_S16 EbHevcchromaFilterCoeffSR1_AVX[8][4] =
{
{  0, 32,  0,  0 },
{ -1, 29,  5, -1 },
{ -2, 27,  8, -1 },
{ -3, 23, 14, -2 },
{ -2, 18, 18, -2 },
{ -2, 14, 23, -3 },
{ -1,  8, 27, -2 },
{ -1,  5, 29, -1 },
};


void ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN(refPic - ((MaxChromaFilterTag - 1) >> 1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight + MaxChromaFilterTag - 1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif


    //vertical filtering
    ChromaInterpolationFilterTwoDInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight, fracPosy);
}

void ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN(refPic - ((MaxChromaFilterTag - 1) >> 1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight + MaxChromaFilterTag - 1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    ChromaInterpolationFilterTwoDInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight, fracPosy);
}


void ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN(
	EB_U16               *refPic,
	EB_U32                srcStride,
	EB_S16               *dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst,
	EB_U32                fracPosx,
	EB_U32                fracPosy)
{
	EB_U32 rowCount, colCount;
	EB_U16 *ptr;
	__m128i a0, a1, a2, a3, b0, b1, b2, b3, c0, c1, c2, c3, sum;

	__m256i e0, e1, e2, e3;
	EB_U32 remain = 0, height = 0;
	EB_S16 *dst2, *dst_cpy;

	(void)firstPassIFDst;
	(void)fracPosy;

	refPic--;
	//PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

	c0 = _mm_loadl_epi64((__m128i *)EbHevcchromaFilterCoeffSR1_AVX[fracPosx]);
	c0 = _mm_unpacklo_epi16(c0, c0);
	c3 = _mm_shuffle_epi32(c0, 0xff);
	c2 = _mm_shuffle_epi32(c0, 0xaa);
	c1 = _mm_shuffle_epi32(c0, 0x55);
	c0 = _mm_shuffle_epi32(c0, 0x00);

	if (puWidth & 2) {
		ptr = refPic;
		for (rowCount = 0; rowCount<puHeight; rowCount += 4) {
			// Need 5 samples, load 8
			a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride; // 00 01 02 03 04 .. .. ..
			a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride; // 10 11 12 13 14 .. .. ..
			a2 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride; // 20 21 22 23 24 .. .. ..
			a3 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride; // 30 31 32 33 34 .. .. ..

			b0 = _mm_unpacklo_epi32(a0, a1); // 00 01 10 11 02 03 12 13
			b1 = _mm_unpacklo_epi32(a2, a3); // 20 21 30 31 22 23 32 33
			a0 = _mm_srli_si128(a0, 2);
			a1 = _mm_srli_si128(a1, 2);
			a2 = _mm_srli_si128(a2, 2);
			a3 = _mm_srli_si128(a3, 2);
			b2 = _mm_unpacklo_epi32(a0, a1); // 01 02 11 12 03 04 13 14
			b3 = _mm_unpacklo_epi32(a2, a3); // 21 22 31 32 23 24 33 34

			a0 = _mm_unpacklo_epi64(b0, b1); // 00 01 10 11 20 21 30 31
			a2 = _mm_unpackhi_epi64(b0, b1); // 02 03 12 13 22 23 32 33
			a1 = _mm_unpacklo_epi64(b2, b3); // 01 02 11 12 21 22 31 32
			a3 = _mm_unpackhi_epi64(b2, b3); // 03 04 13 14 23 24 33 34

			sum = _mm_set1_epi16(OFFSET2D1_10BIT >> 1);
			sum = _mm_add_epi16(sum, _mm_mullo_epi16(a0, c0));
			sum = _mm_add_epi16(sum, _mm_mullo_epi16(a1, c1));
			sum = _mm_add_epi16(sum, _mm_mullo_epi16(a3, c3));
			sum = _mm_adds_epi16(sum, _mm_mullo_epi16(a2, c2));
			sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT - 1);

			_mm_storeu_si128((__m128i *)dst, sum);
			dst += 8;
		}

		puWidth -= 2;
		if (!puWidth) {
			return;
		}
		refPic += 2;
	}

	if (puWidth & 4) {
		ptr = refPic;
		for (rowCount = 0; rowCount<puHeight; rowCount += 2) {
			// Need 5 samples, load 8
			a0 = _mm_loadu_si128((__m128i *)ptr);             // 00 01 02 03 04 05 06 ..
			a1 = _mm_loadu_si128((__m128i *)(ptr + srcStride)); // 10 11 12 13 14 15 16 ..
			ptr += 2 * srcStride;

			b0 = _mm_unpacklo_epi64(a0, a1); // 00 01 02 03 10 11 12 13
			a0 = _mm_srli_si128(a0, 2);
			a1 = _mm_srli_si128(a1, 2);
			b1 = _mm_unpacklo_epi64(a0, a1); // 01 02 03 04 11 12 13 14
			a0 = _mm_srli_si128(a0, 2);
			a1 = _mm_srli_si128(a1, 2);
			b2 = _mm_unpacklo_epi64(a0, a1); // 02 03 04 05 12 13 14 15
			a0 = _mm_srli_si128(a0, 2);
			a1 = _mm_srli_si128(a1, 2);
			b3 = _mm_unpacklo_epi64(a0, a1); // 03 04 05 06 13 14 15 16

			sum = _mm_set1_epi16(OFFSET2D1_10BIT >> 1);
			sum = _mm_add_epi16(sum, _mm_mullo_epi16(b0, c0));
			sum = _mm_add_epi16(sum, _mm_mullo_epi16(b1, c1));
			sum = _mm_add_epi16(sum, _mm_mullo_epi16(b3, c3));
			sum = _mm_adds_epi16(sum, _mm_mullo_epi16(b2, c2));
			sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT - 1);

			_mm_storeu_si128((__m128i *)dst, sum);
			dst += 8;
		}

		puWidth -= 4;
		if (puWidth == 0) {
			return;
		}
		refPic += 4;
	}

	e0 = _mm256_insertf128_si256(_mm256_castsi128_si256(c0), c0, 1);
	e1 = _mm256_insertf128_si256(_mm256_castsi128_si256(c1), c1, 1);
	e2 = _mm256_insertf128_si256(_mm256_castsi128_si256(c2), c2, 1);
	e3 = _mm256_insertf128_si256(_mm256_castsi128_si256(c3), c3, 1);

	if (puWidth >= 16 && !(puWidth & 15)) {
		dst_cpy = dst;

		for (colCount = 0; colCount<puWidth; colCount += 16) {
			ptr = refPic;
			dst = dst_cpy + (puHeight * colCount);
			dst2 = dst + (puHeight * 8);

			for (rowCount = 0; rowCount<puHeight; rowCount++) {
				__m256i d0 = _mm256_loadu_si256((__m256i *)ptr);
				__m256i d1 = _mm256_loadu_si256((__m256i *)(ptr + 1));
				__m256i d2 = _mm256_loadu_si256((__m256i *)(ptr + 2));
				__m256i d3 = _mm256_loadu_si256((__m256i *)(ptr + 3));
				__m256i sum1 = _mm256_set1_epi16(OFFSET2D1_10BIT >> 1);
				ptr += srcStride;

				sum1 = _mm256_add_epi16(sum1, _mm256_mullo_epi16(d0, e0));
				sum1 = _mm256_add_epi16(sum1, _mm256_mullo_epi16(d1, e1));
				sum1 = _mm256_add_epi16(sum1, _mm256_mullo_epi16(d3, e3));
				sum1 = _mm256_adds_epi16(sum1, _mm256_mullo_epi16(d2, e2));
				sum1 = _mm256_srai_epi16(sum1, SHIFT2D1_10BIT - 1);

				_mm_storeu_si128((__m128i *)dst, _mm256_castsi256_si128(sum1));
				_mm_storeu_si128((__m128i *)dst2, _mm256_extracti128_si256(sum1, 1)); // suboptimal code-gen of a cross-lane shuffle by MSVC, no way to fix
				dst += 8;
				dst2 += 8;
			}
			refPic += 16;
		}
	}
	else {
		remain = puHeight & 1;
		height = puHeight - remain;

		for (colCount = 0; colCount < puWidth; colCount += 8) {
			ptr = refPic;
			for (rowCount = 0; rowCount < height; rowCount += 2) {
				__m256i d0 = _mm256_castsi128_si256(_mm_loadu_si128((__m128i *)ptr));
				__m256i d1 = _mm256_castsi128_si256(_mm_loadu_si128((__m128i *)(ptr + 1)));
				__m256i d2 = _mm256_castsi128_si256(_mm_loadu_si128((__m128i *)(ptr + 2)));
				__m256i d3 = _mm256_castsi128_si256(_mm_loadu_si128((__m128i *)(ptr + 3)));
				__m256i sum1 = _mm256_set1_epi16(OFFSET2D1_10BIT >> 1);
				ptr += srcStride;

				{ // fixing MSVC code-gen of reg-mem VINSERT instructions
					EB_U16 *ptr_ = ptr;
					ptr += srcStride;
					d0 = _mm256_inserti128_si256(d0, _mm_loadu_si128((__m128i *)ptr_), 1);
					d1 = _mm256_inserti128_si256(d1, _mm_loadu_si128((__m128i *)(ptr_ + 1)), 1);
					d2 = _mm256_inserti128_si256(d2, _mm_loadu_si128((__m128i *)(ptr_ + 2)), 1);
					d3 = _mm256_inserti128_si256(d3, _mm_loadu_si128((__m128i *)(ptr_ + 3)), 1);
				}

				sum1 = _mm256_add_epi16(sum1, _mm256_mullo_epi16(d0, e0));
				sum1 = _mm256_add_epi16(sum1, _mm256_mullo_epi16(d1, e1));
				sum1 = _mm256_add_epi16(sum1, _mm256_mullo_epi16(d3, e3));
				sum1 = _mm256_adds_epi16(sum1, _mm256_mullo_epi16(d2, e2));
				sum1 = _mm256_srai_epi16(sum1, SHIFT2D1_10BIT - 1);

				_mm256_storeu_si256((__m256i *)dst, sum1);
				dst += 16;
			}

			for (rowCount = 0; rowCount < remain; rowCount++) {
				a0 = _mm_loadu_si128((__m128i *)ptr);
				a1 = _mm_loadu_si128((__m128i *)(ptr + 1));
				a2 = _mm_loadu_si128((__m128i *)(ptr + 2));
				a3 = _mm_loadu_si128((__m128i *)(ptr + 3));
				ptr += srcStride;

				sum = _mm_set1_epi16(OFFSET2D1_10BIT >> 1);
				sum = _mm_add_epi16(sum, _mm_mullo_epi16(a0, c0));
				sum = _mm_add_epi16(sum, _mm_mullo_epi16(a1, c1));
				sum = _mm_add_epi16(sum, _mm_mullo_epi16(a3, c3));
				sum = _mm_adds_epi16(sum, _mm_mullo_epi16(a2, c2));
				sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT - 1);

				_mm_storeu_si128((__m128i *)dst, sum);
				dst += 8;
			}
			refPic += 8;
		}
	}

}
