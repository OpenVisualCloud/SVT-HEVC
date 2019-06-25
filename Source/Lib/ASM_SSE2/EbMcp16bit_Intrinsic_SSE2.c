/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "emmintrin.h"
#include "EbMcp_SSE2.h"
#include "EbDefinitions.h"

EB_EXTERN EB_ALIGN(16) const EB_S16 lumaIFCoeff16_SSE2_INTRIN[]= {
     -1,  4,  -1,  4,  -1,  4,  -1,  4,
    -10, 58, -10, 58, -10, 58, -10, 58,
     17, -5,  17, -5,  17, -5,  17, -5,
      1, -5,   1, -5,   1, -5,   1, -5,
     17, 58,  17, 58,  17, 58,  17, 58,
    -10,  4, -10,  4, -10,  4, -10,  4,
};

EB_EXTERN EB_ALIGN(16) const EB_S16 chromaFilterCoeffSR1[8][4] =
{
  { 0, 32,  0,  0},
  {-1, 29,  5, -1},
  {-2, 27,  8, -1},
  {-3, 23, 14, -2},
  {-2, 18, 18, -2},
  {-2, 14, 23, -3},
  {-1,  8, 27, -2},
  {-1,  5, 29, -1},
};

//extern const EB_S16 chromaFilterCoeff[8][4];

void PictureCopyKernelOutRaw16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_S16               *dst,
  EB_U32                puWidth,
  EB_U32                puHeight)
{
  EB_U32 rowCount, colCount;
  __m128i a0, a1, a2, a3;

  //PrefetchBlock(refPic, srcStride, puWidth, puHeight);

  if (puWidth & 2) {
    EB_U16 *ptr = refPic;
    rowCount = puHeight;
    do {
      a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a3 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a0 = _mm_unpacklo_epi32(a0, a1);
      a2 = _mm_unpacklo_epi32(a2, a3);
      a0 = _mm_unpacklo_epi64(a0, a2);
      a0 = _mm_slli_epi16(a0, BI_SHIFT_10BIT);
      a0 = _mm_sub_epi16(a0, _mm_set1_epi16(BI_OFFSET_10BIT));
      _mm_storeu_si128((__m128i *)dst, a0);

      dst += 8;
      rowCount -= 4;
    }
    while (rowCount != 0);

    puWidth -= 2;
    if (puWidth == 0) {
      return;
    }

    refPic += 2;
  }

  if (puWidth & 4) {
    EB_U16 *ptr = refPic;
    rowCount = puHeight;
    do {
      __m128i a0, a1;
      a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a0 = _mm_unpacklo_epi64(a0, a1);
      a0 = _mm_slli_epi16(a0, BI_SHIFT_10BIT);
      a0 = _mm_sub_epi16(a0, _mm_set1_epi16(BI_OFFSET_10BIT));
      _mm_storeu_si128((__m128i *)dst, a0);

      dst += 8;
      rowCount -= 2;
    }
    while (rowCount != 0);

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }

    refPic += 4;
  }

  colCount = puWidth;
  do {
    __m128i a0;
    EB_U16 *ptr = refPic;
    rowCount = puHeight;
    do {
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
      a0 = _mm_slli_epi16(a0, BI_SHIFT_10BIT);
      a0 = _mm_sub_epi16(a0, _mm_set1_epi16(BI_OFFSET_10BIT));
      _mm_storeu_si128((__m128i *)dst, a0);
      dst += 8;
    }
    while (--rowCount != 0);

    colCount -= 8;
    refPic += 8;
  }
  while (colCount != 0);
}

void LumaInterpolationFilterPosa16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_U16               *dst,
  EB_U32                dstStride,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr, *qtr;
  __m128i a0, a1, a2, a3, a4, a5, a6;

  (void)firstPassIFDst;
  refPic -= 3;

  if (puWidth & 4) {
    ptr = refPic;
    qtr = dst;
    for (i=0; i<puHeight; i+=2) {
      a0 = _mm_loadl_epi64((__m128i *)(ptr+0));
      a1 = _mm_loadl_epi64((__m128i *)(ptr+1));
      a2 = _mm_loadl_epi64((__m128i *)(ptr+2));
      a3 = _mm_loadl_epi64((__m128i *)(ptr+3));
      a4 = _mm_loadl_epi64((__m128i *)(ptr+4));
      a5 = _mm_loadl_epi64((__m128i *)(ptr+5));
      a6 = _mm_loadl_epi64((__m128i *)(ptr+6));
      ptr += srcStride;
      a0 = _mm_unpacklo_epi64(a0, _mm_loadl_epi64((__m128i *)(ptr+0)));
      a1 = _mm_unpacklo_epi64(a1, _mm_loadl_epi64((__m128i *)(ptr+1)));
      a2 = _mm_unpacklo_epi64(a2, _mm_loadl_epi64((__m128i *)(ptr+2)));
      a3 = _mm_unpacklo_epi64(a3, _mm_loadl_epi64((__m128i *)(ptr+3)));
      a4 = _mm_unpacklo_epi64(a4, _mm_loadl_epi64((__m128i *)(ptr+4)));
      a5 = _mm_unpacklo_epi64(a5, _mm_loadl_epi64((__m128i *)(ptr+5)));
      a6 = _mm_unpacklo_epi64(a6, _mm_loadl_epi64((__m128i *)(ptr+6)));
      ptr += srcStride;

      // Process taps with odd values, shift right by 1
      a4 = _mm_mullo_epi16(a4, _mm_set1_epi16(17)); // 0:17391
      a5 = _mm_mullo_epi16(a5, _mm_set1_epi16(-5)); // -5115:0
      a6 = _mm_sub_epi16(a6, a0); // -1023:1023
      a6 = _mm_add_epi16(a6, a4); // -1023:18414
      a6 = _mm_add_epi16(a6, a5); // -6138:18414
      a6 = _mm_srai_epi16(a6, 1); // -3069:9207

      // Process taps with even values
      a1 = _mm_add_epi16(a1, a1); // 0:2046
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-5)); // -5115:0
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      a6 = _mm_add_epi16(a1, a6); // -3069:11253
      a6 = _mm_add_epi16(a2, a6); // -8184:11253
      a6 = _mm_adds_epi16(a6, a3); // -8184:32767

      // Shift right by 5 with rounding
      a6 = _mm_adds_epi16(a6, _mm_set1_epi16(16)); // -8168:32767
      a6 = _mm_srai_epi16(a6, 5); // -256:1023

      // Clip to valid range
      a6 = _mm_max_epi16(a6, _mm_setzero_si128());
      _mm_storel_epi64((__m128i *)qtr, a6);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), _mm_srli_si128(a6, 8));
      qtr += 2*dstStride;
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
    dst += 4;
  }

  for (i=0; i<puHeight; i++) {
    for (j=0; j<puWidth; j+=8) {
      // {-1, 4, -10, 58, 17, -5, 1}
      a0 = _mm_loadu_si128((__m128i *)(refPic+0));
      a1 = _mm_loadu_si128((__m128i *)(refPic+1));
      a2 = _mm_loadu_si128((__m128i *)(refPic+2));
      a3 = _mm_loadu_si128((__m128i *)(refPic+3));
      a4 = _mm_loadu_si128((__m128i *)(refPic+4));
      a5 = _mm_loadu_si128((__m128i *)(refPic+5));
      a6 = _mm_loadu_si128((__m128i *)(refPic+6));
      refPic += 8;

      // Process taps with odd values, shift right by 1
      a4 = _mm_mullo_epi16(a4, _mm_set1_epi16(17)); // 0:17391
      a5 = _mm_mullo_epi16(a5, _mm_set1_epi16(-5)); // -5115:0
      a6 = _mm_sub_epi16(a6, a0); // -1023:1023
      a6 = _mm_add_epi16(a6, a4); // -1023:18414
      a6 = _mm_add_epi16(a6, a5); // -6138:18414
      a6 = _mm_srai_epi16(a6, 1); // -3069:9207

      // Process taps with even values
      a1 = _mm_add_epi16(a1, a1); // 0:2046
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-5)); // -5115:0
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      a6 = _mm_add_epi16(a1, a6); // -3069:11253
      a6 = _mm_add_epi16(a2, a6); // -8184:11253
      a6 = _mm_adds_epi16(a6, a3); // -8184:32767

      // Shift right by 5 with rounding
      a6 = _mm_adds_epi16(a6, _mm_set1_epi16(16)); // -8168:32767
      a6 = _mm_srai_epi16(a6, 5); // -256:1023

      // Clip to valid range
      a6 = _mm_max_epi16(a6, _mm_setzero_si128());
      _mm_storeu_si128((__m128i *)dst, a6);
      dst += 8;
    }
    refPic += srcStride - puWidth;
    dst += dstStride - puWidth;
  }
}

void LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_S16               *dst,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr;
  __m128i a0, a1, a2, a3, a4, a5, a6;

  (void)firstPassIFDst;
  refPic -= 3;
  if (puWidth & 4) {
    ptr = refPic;
    for (i=0; i<puHeight; i+=2) {
      a0 = _mm_loadl_epi64((__m128i *)(ptr+0));
      a1 = _mm_loadl_epi64((__m128i *)(ptr+1));
      a2 = _mm_loadl_epi64((__m128i *)(ptr+2));
      a3 = _mm_loadl_epi64((__m128i *)(ptr+3));
      a4 = _mm_loadl_epi64((__m128i *)(ptr+4));
      a5 = _mm_loadl_epi64((__m128i *)(ptr+5));
      a6 = _mm_loadl_epi64((__m128i *)(ptr+6));
      ptr += srcStride;
      a0 = _mm_unpacklo_epi64(a0, _mm_loadl_epi64((__m128i *)(ptr+0)));
      a1 = _mm_unpacklo_epi64(a1, _mm_loadl_epi64((__m128i *)(ptr+1)));
      a2 = _mm_unpacklo_epi64(a2, _mm_loadl_epi64((__m128i *)(ptr+2)));
      a3 = _mm_unpacklo_epi64(a3, _mm_loadl_epi64((__m128i *)(ptr+3)));
      a4 = _mm_unpacklo_epi64(a4, _mm_loadl_epi64((__m128i *)(ptr+4)));
      a5 = _mm_unpacklo_epi64(a5, _mm_loadl_epi64((__m128i *)(ptr+5)));
      a6 = _mm_unpacklo_epi64(a6, _mm_loadl_epi64((__m128i *)(ptr+6)));
      ptr += srcStride;

      // Process taps with odd values, shift right by 1
      a4 = _mm_mullo_epi16(a4, _mm_set1_epi16(17)); // 0:17391
      a5 = _mm_mullo_epi16(a5, _mm_set1_epi16(-5)); // -5115:0
      a6 = _mm_sub_epi16(a6, a0); // -1023:1023
      a6 = _mm_add_epi16(a6, a4); // -1023:18414
      a6 = _mm_add_epi16(a6, a5); // -6138:18414
      a6 = _mm_srai_epi16(a6, 1); // -3069:9207

      // Process taps with even values
      a1 = _mm_add_epi16(a1, a1); // 0:2046
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-5)); // -5115:0
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      a6 = _mm_add_epi16(a1, a6); // -3069:11253
      a6 = _mm_add_epi16(a2, a6); // -8184:11253
      a6 = _mm_add_epi16(a6, _mm_set1_epi16(-16384)); // -24568:-5131
      a6 = _mm_add_epi16(a3, a6); // -24568:24536

      // Shift right by 1 with offset
      a6 = _mm_srai_epi16(a6, 1);
      _mm_storeu_si128((__m128i *)dst, a6);
      dst += 8;
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    for (j=0; j<puHeight; j++) {
      // {-1, 4, -10, 58, 17, -5, 1}
      a0 = _mm_loadu_si128((__m128i *)(ptr+0));
      a1 = _mm_loadu_si128((__m128i *)(ptr+1));
      a2 = _mm_loadu_si128((__m128i *)(ptr+2));
      a3 = _mm_loadu_si128((__m128i *)(ptr+3));
      a4 = _mm_loadu_si128((__m128i *)(ptr+4));
      a5 = _mm_loadu_si128((__m128i *)(ptr+5));
      a6 = _mm_loadu_si128((__m128i *)(ptr+6));
      ptr += srcStride;

      // Process taps with odd values, shift right by 1
      a4 = _mm_mullo_epi16(a4, _mm_set1_epi16(17)); // 0:17391
      a5 = _mm_mullo_epi16(a5, _mm_set1_epi16(-5)); // -5115:0
      a6 = _mm_sub_epi16(a6, a0); // -1023:1023
      a6 = _mm_add_epi16(a6, a4); // -1023:18414
      a6 = _mm_add_epi16(a6, a5); // -6138:18414
      a6 = _mm_srai_epi16(a6, 1); // -3069:9207

      // Process taps with even values
      a1 = _mm_add_epi16(a1, a1); // 0:2046
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-5)); // -5115:0
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      a6 = _mm_add_epi16(a1, a6); // -3069:11253
      a6 = _mm_add_epi16(a2, a6); // -8184:11253
      a6 = _mm_add_epi16(a6, _mm_set1_epi16(-16384)); // -24568:-5131
      a6 = _mm_add_epi16(a6, a3); // -24568:24536

      // Shift right by 1 with offset
      a6 = _mm_srai_epi16(a6, 1);
      _mm_storeu_si128((__m128i *)dst, a6);
      dst += 8;
    }
    refPic += 8;
  }
}

void LumaInterpolationFilterPosb16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_U16               *dst,
  EB_U32                dstStride,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr, *qtr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;

  (void)firstPassIFDst;
  refPic -= 3;

  if (puWidth & 4) {
    ptr = refPic;
    qtr = dst;
    for (i=0; i<puHeight; i+=2) {
      a0 = _mm_loadl_epi64((__m128i *)(ptr+0));
      a1 = _mm_loadl_epi64((__m128i *)(ptr+1));
      a2 = _mm_loadl_epi64((__m128i *)(ptr+2));
      a3 = _mm_loadl_epi64((__m128i *)(ptr+3));
      a4 = _mm_loadl_epi64((__m128i *)(ptr+4));
      a5 = _mm_loadl_epi64((__m128i *)(ptr+5));
      a6 = _mm_loadl_epi64((__m128i *)(ptr+6));
      a7 = _mm_loadl_epi64((__m128i *)(ptr+7));
      ptr += srcStride;
      a0 = _mm_unpacklo_epi64(a0, _mm_loadl_epi64((__m128i *)(ptr+0)));
      a1 = _mm_unpacklo_epi64(a1, _mm_loadl_epi64((__m128i *)(ptr+1)));
      a2 = _mm_unpacklo_epi64(a2, _mm_loadl_epi64((__m128i *)(ptr+2)));
      a3 = _mm_unpacklo_epi64(a3, _mm_loadl_epi64((__m128i *)(ptr+3)));
      a4 = _mm_unpacklo_epi64(a4, _mm_loadl_epi64((__m128i *)(ptr+4)));
      a5 = _mm_unpacklo_epi64(a5, _mm_loadl_epi64((__m128i *)(ptr+5)));
      a6 = _mm_unpacklo_epi64(a6, _mm_loadl_epi64((__m128i *)(ptr+6)));
      a7 = _mm_unpacklo_epi64(a7, _mm_loadl_epi64((__m128i *)(ptr+7)));
      ptr += srcStride;

      // Use symmetry
      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);

      // Process taps with odd values, shift right by 2
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-11));
      a0 = _mm_sub_epi16(a2, a0);
      a0 = _mm_srai_epi16(a0, 2);

      // Process taps with values that are multiples of 4
      a1 = _mm_add_epi16(a1, a0);
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(10));
      a1 = _mm_add_epi16(a1, a3);

      // Shift right by 4 with rounding
      a1 = _mm_add_epi16(a1, _mm_set1_epi16(8));
      a1 = _mm_srai_epi16(a1, 4);

      // Clip to valid range
      a1 = _mm_max_epi16(a1, _mm_setzero_si128());
      a1 = _mm_min_epi16(a1, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));
      _mm_storel_epi64((__m128i *)qtr, a1);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), _mm_srli_si128(a1, 8));
      qtr += 2*dstStride;
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
    dst += 4;
  }

  for (i=0; i<puHeight; i++) {
    for (j=0; j<puWidth; j+=8) {
      a0 = _mm_loadu_si128((__m128i *)(refPic+0));
      a1 = _mm_loadu_si128((__m128i *)(refPic+1));
      a2 = _mm_loadu_si128((__m128i *)(refPic+2));
      a3 = _mm_loadu_si128((__m128i *)(refPic+3));
      a4 = _mm_loadu_si128((__m128i *)(refPic+4));
      a5 = _mm_loadu_si128((__m128i *)(refPic+5));
      a6 = _mm_loadu_si128((__m128i *)(refPic+6));
      a7 = _mm_loadu_si128((__m128i *)(refPic+7));
      refPic += 8;

      // Use symmetry
      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);

      // Process taps with odd values, shift right by 2
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-11));
      a0 = _mm_sub_epi16(a2, a0);
      a0 = _mm_srai_epi16(a0, 2);

      // Process taps with values that are multiples of 4
      a1 = _mm_add_epi16(a1, a0);
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(10));
      a1 = _mm_add_epi16(a1, a3);

      // Shift right by 4 with rounding
      a1 = _mm_add_epi16(a1, _mm_set1_epi16(8));
      a1 = _mm_srai_epi16(a1, 4);

      // Clip to valid range
      a1 = _mm_max_epi16(a1, _mm_setzero_si128());
      a1 = _mm_min_epi16(a1, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));
      _mm_storeu_si128((__m128i *)dst, a1);
      dst += 8;
    }
    refPic += srcStride - puWidth;
    dst += dstStride - puWidth;
  }
}

void LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_S16               *dst,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;

  (void)firstPassIFDst;
  refPic -= 3;

  if (puWidth & 4) {
    ptr = refPic;
    for (i=0; i<puHeight; i+=2) {
      a0 = _mm_loadl_epi64((__m128i *)(ptr+0));
      a1 = _mm_loadl_epi64((__m128i *)(ptr+1));
      a2 = _mm_loadl_epi64((__m128i *)(ptr+2));
      a3 = _mm_loadl_epi64((__m128i *)(ptr+3));
      a4 = _mm_loadl_epi64((__m128i *)(ptr+4));
      a5 = _mm_loadl_epi64((__m128i *)(ptr+5));
      a6 = _mm_loadl_epi64((__m128i *)(ptr+6));
      a7 = _mm_loadl_epi64((__m128i *)(ptr+7));
      ptr += srcStride;
      a0 = _mm_unpacklo_epi64(a0, _mm_loadl_epi64((__m128i *)(ptr+0)));
      a1 = _mm_unpacklo_epi64(a1, _mm_loadl_epi64((__m128i *)(ptr+1)));
      a2 = _mm_unpacklo_epi64(a2, _mm_loadl_epi64((__m128i *)(ptr+2)));
      a3 = _mm_unpacklo_epi64(a3, _mm_loadl_epi64((__m128i *)(ptr+3)));
      a4 = _mm_unpacklo_epi64(a4, _mm_loadl_epi64((__m128i *)(ptr+4)));
      a5 = _mm_unpacklo_epi64(a5, _mm_loadl_epi64((__m128i *)(ptr+5)));
      a6 = _mm_unpacklo_epi64(a6, _mm_loadl_epi64((__m128i *)(ptr+6)));
      a7 = _mm_unpacklo_epi64(a7, _mm_loadl_epi64((__m128i *)(ptr+7)));
      ptr += srcStride;

      // Use symmetry
      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);

      // Process taps with odd values, shift right by 2
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-11));
      a0 = _mm_sub_epi16(a2, a0);
      a0 = _mm_srai_epi16(a0, 2);

      // Process taps with values that are multiples of 4
      a1 = _mm_add_epi16(a1, a0);
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(10));
      a1 = _mm_add_epi16(a1, a3);

      // Add offset
      a1 = _mm_add_epi16(a1, _mm_set1_epi16(-8192));
      _mm_storeu_si128((__m128i *)dst, a1);
      dst += 8;
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    for (j=0; j<puHeight; j++) {
      a0 = _mm_loadu_si128((__m128i *)(ptr+0));
      a1 = _mm_loadu_si128((__m128i *)(ptr+1));
      a2 = _mm_loadu_si128((__m128i *)(ptr+2));
      a3 = _mm_loadu_si128((__m128i *)(ptr+3));
      a4 = _mm_loadu_si128((__m128i *)(ptr+4));
      a5 = _mm_loadu_si128((__m128i *)(ptr+5));
      a6 = _mm_loadu_si128((__m128i *)(ptr+6));
      a7 = _mm_loadu_si128((__m128i *)(ptr+7));
      ptr += srcStride;

      // Use symmetry
      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);

      // Process taps with odd values, shift right by 2
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-11));
      a0 = _mm_sub_epi16(a2, a0);
      a0 = _mm_srai_epi16(a0, 2);

      // Process taps with values that are multiples of 4
      a1 = _mm_add_epi16(a1, a0);
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(10));
      a1 = _mm_add_epi16(a1, a3);

      // Add offset
      a1 = _mm_add_epi16(a1, _mm_set1_epi16(-8192));
      _mm_storeu_si128((__m128i *)dst, a1);
      dst += 8;
    }
    refPic += 8;
  }
}

void LumaInterpolationFilterPosc16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_U16               *dst,
  EB_U32                dstStride,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr, *qtr;
  __m128i a0, a1, a2, a3, a4, a5, a6;

  (void)firstPassIFDst;
  refPic -= 2;

  if (puWidth & 4) {
    ptr = refPic;
    qtr = dst;
    for (i=0; i<puHeight; i+=2) {
      a0 = _mm_loadl_epi64((__m128i *)(ptr+0));
      a1 = _mm_loadl_epi64((__m128i *)(ptr+1));
      a2 = _mm_loadl_epi64((__m128i *)(ptr+2));
      a3 = _mm_loadl_epi64((__m128i *)(ptr+3));
      a4 = _mm_loadl_epi64((__m128i *)(ptr+4));
      a5 = _mm_loadl_epi64((__m128i *)(ptr+5));
      a6 = _mm_loadl_epi64((__m128i *)(ptr+6));
      ptr += srcStride;
      a0 = _mm_unpacklo_epi64(a0, _mm_loadl_epi64((__m128i *)(ptr+0)));
      a1 = _mm_unpacklo_epi64(a1, _mm_loadl_epi64((__m128i *)(ptr+1)));
      a2 = _mm_unpacklo_epi64(a2, _mm_loadl_epi64((__m128i *)(ptr+2)));
      a3 = _mm_unpacklo_epi64(a3, _mm_loadl_epi64((__m128i *)(ptr+3)));
      a4 = _mm_unpacklo_epi64(a4, _mm_loadl_epi64((__m128i *)(ptr+4)));
      a5 = _mm_unpacklo_epi64(a5, _mm_loadl_epi64((__m128i *)(ptr+5)));
      a6 = _mm_unpacklo_epi64(a6, _mm_loadl_epi64((__m128i *)(ptr+6)));
      ptr += srcStride;

      // Process taps with odd values, shift right by 1
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(17)); // 0:17391
      a1 = _mm_mullo_epi16(a1, _mm_set1_epi16(-5)); // -5115:0
      a0 = _mm_sub_epi16(a0, a6); // -1023:1023
      a0 = _mm_add_epi16(a0, a2); // -1023:18414
      a0 = _mm_add_epi16(a0, a1); // -6138:18414
      a0 = _mm_srai_epi16(a0, 1); // -3069:9207

      // Process taps with even values
      a5 = _mm_add_epi16(a5, a5); // 0:2046
      a4 = _mm_mullo_epi16(a4, _mm_set1_epi16(-5)); // -5115:0
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      a0 = _mm_add_epi16(a0, a5); // -3069:11253
      a0 = _mm_add_epi16(a0, a4); // -8184:11253
      a0 = _mm_adds_epi16(a0, a3); // -8184:32767

      // Shift right by 5 with rounding
      a0 = _mm_adds_epi16(a0, _mm_set1_epi16(16)); // -8168:32767
      a0 = _mm_srai_epi16(a0, 5); // -256:1023

      // Clip to valid range
      a0 = _mm_max_epi16(a0, _mm_setzero_si128());
      _mm_storel_epi64((__m128i *)qtr, a0);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), _mm_srli_si128(a0, 8));
      qtr += 2*dstStride;
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
    dst += 4;
  }

  for (i=0; i<puHeight; i++) {
    for (j=0; j<puWidth; j+=8) {
      a0 = _mm_loadu_si128((__m128i *)(refPic+0));
      a1 = _mm_loadu_si128((__m128i *)(refPic+1));
      a2 = _mm_loadu_si128((__m128i *)(refPic+2));
      a3 = _mm_loadu_si128((__m128i *)(refPic+3));
      a4 = _mm_loadu_si128((__m128i *)(refPic+4));
      a5 = _mm_loadu_si128((__m128i *)(refPic+5));
      a6 = _mm_loadu_si128((__m128i *)(refPic+6));
      refPic += 8;

      // Process taps with odd values, shift right by 1
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(17)); // 0:17391
      a1 = _mm_mullo_epi16(a1, _mm_set1_epi16(-5)); // -5115:0
      a0 = _mm_sub_epi16(a0, a6); // -1023:1023
      a0 = _mm_add_epi16(a0, a2); // -1023:18414
      a0 = _mm_add_epi16(a0, a1); // -6138:18414
      a0 = _mm_srai_epi16(a0, 1); // -3069:9207

      // Process taps with even values
      a5 = _mm_add_epi16(a5, a5); // 0:2046
      a4 = _mm_mullo_epi16(a4, _mm_set1_epi16(-5)); // -5115:0
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      a0 = _mm_add_epi16(a0, a5); // -3069:11253
      a0 = _mm_add_epi16(a0, a4); // -8184:11253
      a0 = _mm_adds_epi16(a0, a3); // -8184:32767

      // Shift right by 5 with rounding
      a0 = _mm_adds_epi16(a0, _mm_set1_epi16(16)); // -8168:32767
      a0 = _mm_srai_epi16(a0, 5); // -256:1023

      // Clip to valid range
      a0 = _mm_max_epi16(a0, _mm_setzero_si128());
      _mm_storeu_si128((__m128i *)dst, a0);
      dst += 8;
    }
    refPic += srcStride - puWidth;
    dst += dstStride - puWidth;
  }
}

void LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_S16               *dst,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr;
  __m128i a0, a1, a2, a3, a4, a5, a6;

  (void)firstPassIFDst;
  refPic -= 2;
  if (puWidth & 4) {
    ptr = refPic;
    for (i=0; i<puHeight; i+=2) {
      a0 = _mm_loadl_epi64((__m128i *)(ptr+0));
      a1 = _mm_loadl_epi64((__m128i *)(ptr+1));
      a2 = _mm_loadl_epi64((__m128i *)(ptr+2));
      a3 = _mm_loadl_epi64((__m128i *)(ptr+3));
      a4 = _mm_loadl_epi64((__m128i *)(ptr+4));
      a5 = _mm_loadl_epi64((__m128i *)(ptr+5));
      a6 = _mm_loadl_epi64((__m128i *)(ptr+6));
      ptr += srcStride;
      a0 = _mm_unpacklo_epi64(a0, _mm_loadl_epi64((__m128i *)(ptr+0)));
      a1 = _mm_unpacklo_epi64(a1, _mm_loadl_epi64((__m128i *)(ptr+1)));
      a2 = _mm_unpacklo_epi64(a2, _mm_loadl_epi64((__m128i *)(ptr+2)));
      a3 = _mm_unpacklo_epi64(a3, _mm_loadl_epi64((__m128i *)(ptr+3)));
      a4 = _mm_unpacklo_epi64(a4, _mm_loadl_epi64((__m128i *)(ptr+4)));
      a5 = _mm_unpacklo_epi64(a5, _mm_loadl_epi64((__m128i *)(ptr+5)));
      a6 = _mm_unpacklo_epi64(a6, _mm_loadl_epi64((__m128i *)(ptr+6)));
      ptr += srcStride;

      // Process taps with odd values, shift right by 1
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(17)); // 0:17391
      a1 = _mm_mullo_epi16(a1, _mm_set1_epi16(-5)); // -5115:0
      a0 = _mm_sub_epi16(a0, a6); // -1023:1023
      a0 = _mm_add_epi16(a0, a2); // -1023:18414
      a0 = _mm_add_epi16(a0, a1); // -6138:18414
      a0 = _mm_srai_epi16(a0, 1); // -3069:9207

      // Process taps with even values
      a5 = _mm_add_epi16(a5, a5); // 0:2046
      a4 = _mm_mullo_epi16(a4, _mm_set1_epi16(-5)); // -5115:0
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      a0 = _mm_add_epi16(a0, a5); // -3069:11253
      a0 = _mm_add_epi16(a0, a4); // -8184:11253
      a0 = _mm_add_epi16(a0, _mm_set1_epi16(-16384)); // -24568:-5131
      a0 = _mm_add_epi16(a0, a3); // -24568:24536

      // Shift right by 1 with offset
      a0 = _mm_srai_epi16(a0, 1);
      _mm_storeu_si128((__m128i *)dst, a0);
      dst += 8;
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    for (j=0; j<puHeight; j++) {
      // {-1, 4, -10, 58, 17, -5, 1}
      a0 = _mm_loadu_si128((__m128i *)(ptr+0));
      a1 = _mm_loadu_si128((__m128i *)(ptr+1));
      a2 = _mm_loadu_si128((__m128i *)(ptr+2));
      a3 = _mm_loadu_si128((__m128i *)(ptr+3));
      a4 = _mm_loadu_si128((__m128i *)(ptr+4));
      a5 = _mm_loadu_si128((__m128i *)(ptr+5));
      a6 = _mm_loadu_si128((__m128i *)(ptr+6));
      ptr += srcStride;

      // Process taps with odd values, shift right by 1
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(17)); // 0:17391
      a1 = _mm_mullo_epi16(a1, _mm_set1_epi16(-5)); // -5115:0
      a0 = _mm_sub_epi16(a0, a6); // -1023:1023
      a0 = _mm_add_epi16(a0, a2); // -1023:18414
      a0 = _mm_add_epi16(a0, a1); // -6138:18414
      a0 = _mm_srai_epi16(a0, 1); // -3069:9207

      // Process taps with even values
      a5 = _mm_add_epi16(a5, a5); // 0:2046
      a4 = _mm_mullo_epi16(a4, _mm_set1_epi16(-5)); // -5115:0
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      a0 = _mm_add_epi16(a0, a5); // -3069:11253
      a0 = _mm_add_epi16(a0, a4); // -8184:11253
      a0 = _mm_add_epi16(a0, _mm_set1_epi16(-16384)); // -24568:-5131
      a0 = _mm_add_epi16(a0, a3); // -24568:24536

      // Shift right by 1 with offset
      a0 = _mm_srai_epi16(a0, 1);
      _mm_storeu_si128((__m128i *)dst, a0);
      dst += 8;
    }
    refPic += 8;
  }
}

void LumaInterpolationFilterPosd16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_U16               *dst,
  EB_U32                dstStride,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr, *qtr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i b0, b1, b2, b3, b4, b5, b6;

  (void)firstPassIFDst;
  refPic -= 3*srcStride;

  if (puWidth & 4) {
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi64(a0, a1);
    b1 = _mm_unpacklo_epi64(a1, a2);
    b2 = _mm_unpacklo_epi64(a2, a3);
    b3 = _mm_unpacklo_epi64(a3, a4);
    b4 = _mm_unpacklo_epi64(a4, a5);
    b5 = _mm_unpacklo_epi64(a5, a6);

    for (i=0; i<puHeight; i+=2) {
      a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b6 = _mm_unpacklo_epi64(a6, a7);

      // Process taps with odd values, shift right by 1
      a0 = _mm_mullo_epi16(b4, _mm_set1_epi16(17)); // 0:17391
      a1 = _mm_mullo_epi16(b5, _mm_set1_epi16(-5)); // -5115:0
      a2 = _mm_sub_epi16(b6, b0); // -1023:1023
      a2 = _mm_add_epi16(a2, a0); // -1023:18414
      a2 = _mm_add_epi16(a2, a1); // -6138:18414
      a2 = _mm_srai_epi16(a2, 1); // -3069:9207

      // Process taps with even values
      a3 = _mm_add_epi16(b1, b1); // 0:2046
      a4 = _mm_mullo_epi16(b2, _mm_set1_epi16(-5)); // -5115:0
      a5 = _mm_mullo_epi16(b3, _mm_set1_epi16(29)); // 0:29667

      a2 = _mm_add_epi16(a2, a3); // -3069:11253
      a2 = _mm_add_epi16(a2, a4); // -8184:11253
      a2 = _mm_adds_epi16(a2, a5); // -8184:32767

      // Shift right by 5 with rounding
      a2 = _mm_adds_epi16(a2, _mm_set1_epi16(16)); // -8168:32767
      a2 = _mm_srai_epi16(a2, 5); // -256:1023

      // Clip to valid range
      a2 = _mm_max_epi16(a2, _mm_setzero_si128());
      _mm_storel_epi64((__m128i *)qtr, a2);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), _mm_srli_si128(a2, 8));
      qtr += 2*dstStride;
      b0 = b2;
      b1 = b3;
      b2 = b4;
      b3 = b5;
      b4 = b6;
      a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b5 = _mm_unpacklo_epi64(a7, a6);
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
    dst += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

    for (j=0; j<puHeight; j++) {
      // {-1, 4, -10, 58, 17, -5, 1}
      a6 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      // Process taps with odd values, shift right by 1
      b0 = _mm_mullo_epi16(a4, _mm_set1_epi16(17)); // 0:17391
      b1 = _mm_mullo_epi16(a5, _mm_set1_epi16(-5)); // -5115:0
      b2 = _mm_sub_epi16(a6, a0); // -1023:1023
      b2 = _mm_add_epi16(b2, b0); // -1023:18414
      b2 = _mm_add_epi16(b2, b1); // -6138:18414
      b2 = _mm_srai_epi16(b2, 1); // -3069:9207

      // Process taps with even values
      b3 = _mm_add_epi16(a1, a1); // 0:2046
      b4 = _mm_mullo_epi16(a2, _mm_set1_epi16(-5)); // -5115:0
      b5 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      b2 = _mm_add_epi16(b3, b2); // -3069:11253
      b2 = _mm_add_epi16(b4, b2); // -8184:11253
      b2 = _mm_adds_epi16(b5, b2); // -8184:32767

      // Shift right by 5 with rounding
      b2 = _mm_adds_epi16(b2, _mm_set1_epi16(16)); // -8168:32767
      b2 = _mm_srai_epi16(b2, 5); // -256:1023

      // Clip to valid range
      b2 = _mm_max_epi16(b2, _mm_setzero_si128());
      _mm_storeu_si128((__m128i *)qtr, b2);
      qtr += dstStride;
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;
    }
    refPic += 8;
    dst += 8;
  }
}

void LumaInterpolationFilterPosdInRaw16bit_SSE2_INTRIN(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                coeffIndex)
{
  EB_U32 i, j;
  EB_U16 *qtr;
  __m128i a0, a1, a2, a3, a4, a5, a6;
  __m128i b0l, b0h, b1l, b1h;
  __m128i c0 = _mm_load_si128((__m128i *)(lumaIFCoeff16_SSE2_INTRIN+coeffIndex));
  __m128i c1 = _mm_load_si128((__m128i *)(lumaIFCoeff16_SSE2_INTRIN+coeffIndex+8));
  __m128i c2 = _mm_load_si128((__m128i *)(lumaIFCoeff16_SSE2_INTRIN+coeffIndex+16));

  if (puWidth & 4) {
    qtr = dst;
    a0 = _mm_load_si128((__m128i *)(firstPassIFDst+0*4));
    a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+2*4));
    a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
    a4 = _mm_load_si128((__m128i *)(firstPassIFDst+4*4));
    firstPassIFDst += 20;
    b1l = _mm_unpacklo_epi16(a2, a3);
    b1h = _mm_unpackhi_epi16(a2, a3);

    for (i=0; i<puHeight; i+=2) {
      __m128i sum0, sum1;
      a6 = _mm_load_si128((__m128i *)(firstPassIFDst+4));
      a0 = _mm_sub_epi16(a0, a6);
      b0l = _mm_unpacklo_epi16(a0, a1);
      b0h = _mm_unpackhi_epi16(a0, a1);
      sum0 = _mm_set1_epi32(OFFSET2D2_10BIT);
      sum1 = _mm_set1_epi32(OFFSET2D2_10BIT);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b0l, c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b0h, c0));
      a0 = a2;
      a1 = a3;
      a2 = a4;
      a4 = a6;
      a3 = _mm_loadu_si128((__m128i *)firstPassIFDst);

      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, c1));
      b1l = _mm_unpacklo_epi16(a2, a3);
      b1h = _mm_unpackhi_epi16(a2, a3);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, c2));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, c2));

      sum0 = _mm_srai_epi32(sum0, SHIFT2D2_10BIT);
      sum1 = _mm_srai_epi32(sum1, SHIFT2D2_10BIT);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_max_epi16(sum0, _mm_setzero_si128());
      sum0 = _mm_min_epi16(sum0, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));

      _mm_storel_epi64((__m128i *)qtr, sum0);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), _mm_srli_si128(sum0, 8));
      qtr += 2*dstStride;
      firstPassIFDst += 8;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    firstPassIFDst += 4;
    dst += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    qtr = dst;

    a0 = _mm_load_si128((__m128i *)(firstPassIFDst+0*8));
    a1 = _mm_load_si128((__m128i *)(firstPassIFDst+1*8));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+2*8));
    a3 = _mm_load_si128((__m128i *)(firstPassIFDst+3*8));
    a4 = _mm_load_si128((__m128i *)(firstPassIFDst+4*8));
    a5 = _mm_load_si128((__m128i *)(firstPassIFDst+5*8));
    firstPassIFDst += 48;

    for (j=0; j<puHeight; j++) {
      __m128i sum0, sum1;

      a6 = _mm_load_si128((__m128i *)firstPassIFDst);
      a0 = _mm_sub_epi16(a0, a6);

      sum0 = _mm_set1_epi32(OFFSET2D2_10BIT);
      sum1 = _mm_set1_epi32(OFFSET2D2_10BIT);

      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a2, a3), c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a2, a3), c1));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a4, a5), c2));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a4, a5), c2));
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;

      sum0 = _mm_srai_epi32(sum0, SHIFT2D2_10BIT);
      sum1 = _mm_srai_epi32(sum1, SHIFT2D2_10BIT);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_max_epi16(sum0, _mm_setzero_si128());
      sum0 = _mm_min_epi16(sum0, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));
      _mm_storeu_si128((__m128i *)qtr, sum0);
      qtr += dstStride;
      firstPassIFDst += 8;
    }
    dst += 8;
  }
}

void LumaInterpolationFilterPosdInRawOutRaw_SSE2_INTRIN(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                coeffIndex)
{
  EB_U32 i, j;
  __m128i a0, a1, a2, a3, a4, a5, a6;
  __m128i b0l, b0h, b1l, b1h;
  __m128i c0 = _mm_load_si128((__m128i *)(lumaIFCoeff16_SSE2_INTRIN+coeffIndex));
  __m128i c1 = _mm_load_si128((__m128i *)(lumaIFCoeff16_SSE2_INTRIN+coeffIndex+8));
  __m128i c2 = _mm_load_si128((__m128i *)(lumaIFCoeff16_SSE2_INTRIN+coeffIndex+16));

  if (puWidth & 4) {
    a0 = _mm_load_si128((__m128i *)(firstPassIFDst+0*4));
    a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+2*4));
    a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
    a4 = _mm_load_si128((__m128i *)(firstPassIFDst+4*4));
    firstPassIFDst += 20;
    b1l = _mm_unpacklo_epi16(a2, a3);
    b1h = _mm_unpackhi_epi16(a2, a3);

    for (i=0; i<puHeight; i+=2) {
      __m128i sum0, sum1;
      a6 = _mm_load_si128((__m128i *)(firstPassIFDst+4));
      a0 = _mm_sub_epi16(a0, a6);
      b0l = _mm_unpacklo_epi16(a0, a1);
      b0h = _mm_unpackhi_epi16(a0, a1);
      sum0 = _mm_madd_epi16(b0l, c0);
      sum1 = _mm_madd_epi16(b0h, c0);
      a0 = a2;
      a1 = a3;
      a2 = a4;
      a4 = a6;
      a3 = _mm_loadu_si128((__m128i *)firstPassIFDst);

      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, c1));
      b1l = _mm_unpacklo_epi16(a2, a3);
      b1h = _mm_unpackhi_epi16(a2, a3);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, c2));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, c2));

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    firstPassIFDst += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    a0 = _mm_load_si128((__m128i *)(firstPassIFDst+0*8));
    a1 = _mm_load_si128((__m128i *)(firstPassIFDst+1*8));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+2*8));
    a3 = _mm_load_si128((__m128i *)(firstPassIFDst+3*8));
    a4 = _mm_load_si128((__m128i *)(firstPassIFDst+4*8));
    a5 = _mm_load_si128((__m128i *)(firstPassIFDst+5*8));
    firstPassIFDst += 48;

    for (j=0; j<puHeight; j++) {
      __m128i sum0, sum1;

      a6 = _mm_load_si128((__m128i *)firstPassIFDst);
      a0 = _mm_sub_epi16(a0, a6);

      sum0 = _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a2, a3), c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a2, a3), c1));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a4, a5), c2));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a4, a5), c2));
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);
      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
    }
  }
}

void LumaInterpolationFilterPosdOutRaw16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_S16               *dst,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i b0, b1, b2, b3, b4, b5, b6;

  (void)firstPassIFDst;
  refPic -= 3*srcStride;

  if (puWidth & 4) {
    ptr = refPic;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi64(a0, a1);
    b1 = _mm_unpacklo_epi64(a1, a2);
    b2 = _mm_unpacklo_epi64(a2, a3);
    b3 = _mm_unpacklo_epi64(a3, a4);
    b4 = _mm_unpacklo_epi64(a4, a5);
    b5 = _mm_unpacklo_epi64(a5, a6);

    for (i=0; i<puHeight; i+=2) {
      a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b6 = _mm_unpacklo_epi64(a6, a7);

      // Process taps with odd values, shift right by 1
      a0 = _mm_mullo_epi16(b4, _mm_set1_epi16(17)); // 0:17391
      a1 = _mm_mullo_epi16(b5, _mm_set1_epi16(-5)); // -5115:0
      a2 = _mm_sub_epi16(b6, b0); // -1023:1023
      a2 = _mm_add_epi16(a2, a0); // -1023:18414
      a2 = _mm_add_epi16(a2, a1); // -6138:18414
      a2 = _mm_srai_epi16(a2, 1); // -3069:9207

      // Process taps with even values
      a3 = _mm_add_epi16(b1, b1); // 0:2046
      a4 = _mm_mullo_epi16(b2, _mm_set1_epi16(-5)); // -5115:0
      a5 = _mm_mullo_epi16(b3, _mm_set1_epi16(29)); // 0:29667

      a2 = _mm_add_epi16(a2, a3); // -3069:11253
      a2 = _mm_add_epi16(a2, a4); // -8184:11253
      a2 = _mm_add_epi16(a2, _mm_set1_epi16(OFFSET2D1_10BIT>>1)); // -24568:-5131
      a2 = _mm_adds_epi16(a2, a5); // -24568:24536
      a2 = _mm_srai_epi16(a2, SHIFT2D1_10BIT-1);

      _mm_storeu_si128((__m128i *)dst, a2);
      dst +=8;
      b0 = b2;
      b1 = b3;
      b2 = b4;
      b3 = b5;
      b4 = b6;
      a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b5 = _mm_unpacklo_epi64(a7, a6);
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

    for (j=0; j<puHeight; j++) {
      // {-1, 4, -10, 58, 17, -5, 1}
      a6 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      // Process taps with odd values, shift right by 1
      b0 = _mm_mullo_epi16(a4, _mm_set1_epi16(17)); // 0:17391
      b1 = _mm_mullo_epi16(a5, _mm_set1_epi16(-5)); // -5115:0
      b2 = _mm_sub_epi16(a6, a0); // -1023:1023
      b2 = _mm_add_epi16(b2, b0); // -1023:18414
      b2 = _mm_add_epi16(b2, b1); // -6138:18414
      b2 = _mm_srai_epi16(b2, 1); // -3069:9207

      // Process taps with even values
      b3 = _mm_add_epi16(a1, a1); // 0:2046
      b4 = _mm_mullo_epi16(a2, _mm_set1_epi16(-5)); // -5115:0
      b5 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      b2 = _mm_add_epi16(b3, b2); // -3069:11253
      b2 = _mm_add_epi16(b4, b2); // -8184:11253
      b2 = _mm_add_epi16(b2, _mm_set1_epi16(OFFSET2D1_10BIT>>1)); // -24568:-5131
      b2 = _mm_adds_epi16(b2, b5); // -24568:24536

      b2 = _mm_srai_epi16(b2, SHIFT2D1_10BIT-1); // -256:1023
      _mm_storeu_si128((__m128i *)dst, b2);
      dst +=8;
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;
    }
    refPic += 8;
  }
}

void LumaInterpolationFilterPosh16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_U16               *dst,
  EB_U32                dstStride,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr, *qtr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a8, a7;
  __m128i b0, b1, b2, b3, b4, b5, b6, b7;

  (void)firstPassIFDst;
  refPic -= 3*srcStride;

  if (puWidth & 4) {
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi64(a0, a1);
    b1 = _mm_unpacklo_epi64(a1, a2);
    b2 = _mm_unpacklo_epi64(a2, a3);
    b3 = _mm_unpacklo_epi64(a3, a4);
    b4 = _mm_unpacklo_epi64(a4, a5);
    b5 = _mm_unpacklo_epi64(a5, a6);
    b6 = _mm_unpacklo_epi64(a6, a7);

    for (i=0; i<puHeight; i+=2) {
      a8 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b7 = _mm_unpacklo_epi64(a7, a8);

      // Use symmetry
      a0 = _mm_add_epi16(b0, b7);
      a1 = _mm_add_epi16(b1, b6);
      a2 = _mm_add_epi16(b2, b5);
      a3 = _mm_add_epi16(b3, b4);

      // Process taps with odd values, shift right by 2
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-11));
      a0 = _mm_sub_epi16(a2, a0);
      a0 = _mm_srai_epi16(a0, 2);

      // Process taps with values that are multiples of 4
      a1 = _mm_add_epi16(a1, a0);
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(10));
      a1 = _mm_add_epi16(a1, a3);

      // Shift right by 4 with rounding
      a1 = _mm_add_epi16(a1, _mm_set1_epi16(8));
      a1 = _mm_srai_epi16(a1, 4);

      // Clip to valid range
      a1 = _mm_max_epi16(a1, _mm_setzero_si128());
      a1 = _mm_min_epi16(a1, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));
      _mm_storel_epi64((__m128i *)qtr, a1);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), _mm_srli_si128(a1, 8));
      qtr += 2*dstStride;
      b0 = b2;
      b1 = b3;
      b2 = b4;
      b3 = b5;
      b4 = b6;
      b5 = b7;
      a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b6 = _mm_unpacklo_epi64(a8, a7);
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
    dst += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

    for (j=0; j<puHeight; j++) {
      a7 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      // Use symmetry
      b0 = _mm_add_epi16(a0, a7);
      b1 = _mm_add_epi16(a1, a6);
      b2 = _mm_add_epi16(a2, a5);
      b3 = _mm_add_epi16(a3, a4);

      // Process taps with odd values, shift right by 2
      b2 = _mm_mullo_epi16(b2, _mm_set1_epi16(-11));
      b0 = _mm_sub_epi16(b2, b0);
      b0 = _mm_srai_epi16(b0, 2);

      // Process taps with values that are multiples of 4
      b1 = _mm_add_epi16(b1, b0);
      b3 = _mm_mullo_epi16(b3, _mm_set1_epi16(10));
      b1 = _mm_add_epi16(b1, b3);

      // Shift right by 4 with rounding
      b1 = _mm_add_epi16(b1, _mm_set1_epi16(8));
      b1 = _mm_srai_epi16(b1, 4);

      // Clip to valid range
      b1 = _mm_max_epi16(b1, _mm_setzero_si128());
      b1 = _mm_min_epi16(b1, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));
      _mm_storeu_si128((__m128i *)qtr, b1);
      qtr += dstStride;
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;
      a6 = a7;
    }
    refPic += 8;
    dst += 8;
  }
}

void LumaInterpolationFilterPoshInRaw16bit_SSE2_INTRIN(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
  EB_U32 i, j;
  EB_U16 *qtr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i b0, b1, b2, b3;
  __m128i b0l, b0h, b1l, b1h;

  if (puWidth & 4) {
    qtr = dst;
    a0 = _mm_load_si128((__m128i *)(firstPassIFDst+0*4));
    a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+2*4));
    a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
    a4 = _mm_load_si128((__m128i *)(firstPassIFDst+4*4));
    a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
    firstPassIFDst += 24;

    for (i=0; i<puHeight; i+=2) {
      __m128i sum0, sum1;
      a6 = _mm_load_si128((__m128i *)firstPassIFDst);
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      b0 = _mm_add_epi16(a0, a7);
      b1 = _mm_add_epi16(a1, a6);
      b2 = _mm_add_epi16(a2, a5);
      b3 = _mm_add_epi16(a3, a4);
      b0l = _mm_unpacklo_epi16(b0, b1);
      b0h = _mm_unpackhi_epi16(b0, b1);
      b1l = _mm_unpacklo_epi16(b2, b3);
      b1h = _mm_unpackhi_epi16(b2, b3);
      sum0 = _mm_set1_epi32(OFFSET2D2_10BIT);
      sum1 = _mm_set1_epi32(OFFSET2D2_10BIT);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b0l, _mm_setr_epi16(-1, 4, -1, 4, -1, 4, -1, 4)));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b0h, _mm_setr_epi16(-1, 4, -1, 4, -1, 4, -1, 4)));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, _mm_setr_epi16(-11, 40, -11, 40, -11, 40, -11, 40)));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, _mm_setr_epi16(-11, 40, -11, 40, -11, 40, -11, 40)));
      a0 = a2;
      a1 = a3;
      a2 = a4;
      a3 = a5;
      a4 = a6;
      a5 = a7;

      sum0 = _mm_srai_epi32(sum0, SHIFT2D2_10BIT);
      sum1 = _mm_srai_epi32(sum1, SHIFT2D2_10BIT);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_max_epi16(sum0, _mm_setzero_si128());
      sum0 = _mm_min_epi16(sum0, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));

      _mm_storel_epi64((__m128i *)qtr, sum0);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), _mm_srli_si128(sum0, 8));
      qtr += 2*dstStride;
      firstPassIFDst += 8;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    firstPassIFDst += 8;
    dst += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    qtr = dst;

    a0 = _mm_load_si128((__m128i *)(firstPassIFDst+0*8));
    a1 = _mm_load_si128((__m128i *)(firstPassIFDst+1*8));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+2*8));
    a3 = _mm_load_si128((__m128i *)(firstPassIFDst+3*8));
    a4 = _mm_load_si128((__m128i *)(firstPassIFDst+4*8));
    a5 = _mm_load_si128((__m128i *)(firstPassIFDst+5*8));
    a6 = _mm_load_si128((__m128i *)(firstPassIFDst+6*8));
    firstPassIFDst += 56;

    for (j=0; j<puHeight; j++) {
      __m128i sum0, sum1;

      a7 = _mm_load_si128((__m128i *)firstPassIFDst);
      b0 = _mm_add_epi16(a0, a7);
      b1 = _mm_add_epi16(a1, a6);
      b2 = _mm_add_epi16(a2, a5);
      b3 = _mm_add_epi16(a3, a4);
      b0l = _mm_unpacklo_epi16(b0, b1);
      b0h = _mm_unpackhi_epi16(b0, b1);
      b1l = _mm_unpacklo_epi16(b2, b3);
      b1h = _mm_unpackhi_epi16(b2, b3);
      sum0 = _mm_set1_epi32(OFFSET2D2_10BIT);
      sum1 = _mm_set1_epi32(OFFSET2D2_10BIT);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b0l, _mm_setr_epi16(-1, 4, -1, 4, -1, 4, -1, 4)));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b0h, _mm_setr_epi16(-1, 4, -1, 4, -1, 4, -1, 4)));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, _mm_setr_epi16(-11, 40, -11, 40, -11, 40, -11, 40)));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, _mm_setr_epi16(-11, 40, -11, 40, -11, 40, -11, 40)));
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;
      a6 = a7;

      sum0 = _mm_srai_epi32(sum0, SHIFT2D2_10BIT);
      sum1 = _mm_srai_epi32(sum1, SHIFT2D2_10BIT);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_max_epi16(sum0, _mm_setzero_si128());
      sum0 = _mm_min_epi16(sum0, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));
      _mm_storeu_si128((__m128i *)qtr, sum0);
      qtr += dstStride;
      firstPassIFDst += 8;
    }
    dst += 8;
  }
}

void LumaInterpolationFilterPoshInRawOutRaw_SSE2_INTRIN(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight)
{
  EB_U32 i, j;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i b0, b1, b2, b3;
  __m128i b0l, b0h, b1l, b1h;

  if (puWidth & 4) {
    a0 = _mm_load_si128((__m128i *)(firstPassIFDst+0*4));
    a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+2*4));
    a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
    a4 = _mm_load_si128((__m128i *)(firstPassIFDst+4*4));
    a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
    firstPassIFDst += 24;

    for (i=0; i<puHeight; i+=2) {
      __m128i sum0, sum1;
      a6 = _mm_load_si128((__m128i *)firstPassIFDst);
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      b0 = _mm_add_epi16(a0, a7);
      b1 = _mm_add_epi16(a1, a6);
      b2 = _mm_add_epi16(a2, a5);
      b3 = _mm_add_epi16(a3, a4);
      b0l = _mm_unpacklo_epi16(b0, b1);
      b0h = _mm_unpackhi_epi16(b0, b1);
      b1l = _mm_unpacklo_epi16(b2, b3);
      b1h = _mm_unpackhi_epi16(b2, b3);
      sum0 = _mm_madd_epi16(b0l, _mm_setr_epi16(-1, 4, -1, 4, -1, 4, -1, 4));
      sum1 = _mm_madd_epi16(b0h, _mm_setr_epi16(-1, 4, -1, 4, -1, 4, -1, 4));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, _mm_setr_epi16(-11, 40, -11, 40, -11, 40, -11, 40)));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, _mm_setr_epi16(-11, 40, -11, 40, -11, 40, -11, 40)));
      a0 = a2;
      a1 = a3;
      a2 = a4;
      a3 = a5;
      a4 = a6;
      a5 = a7;

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    firstPassIFDst += 8;
  }

  for (i=0; i<puWidth; i+=8) {
    a0 = _mm_load_si128((__m128i *)(firstPassIFDst+0*8));
    a1 = _mm_load_si128((__m128i *)(firstPassIFDst+1*8));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+2*8));
    a3 = _mm_load_si128((__m128i *)(firstPassIFDst+3*8));
    a4 = _mm_load_si128((__m128i *)(firstPassIFDst+4*8));
    a5 = _mm_load_si128((__m128i *)(firstPassIFDst+5*8));
    a6 = _mm_load_si128((__m128i *)(firstPassIFDst+6*8));
    firstPassIFDst += 56;

    for (j=0; j<puHeight; j++) {
      __m128i sum0, sum1;

      a7 = _mm_load_si128((__m128i *)firstPassIFDst);
      b0 = _mm_add_epi16(a0, a7);
      b1 = _mm_add_epi16(a1, a6);
      b2 = _mm_add_epi16(a2, a5);
      b3 = _mm_add_epi16(a3, a4);
      b0l = _mm_unpacklo_epi16(b0, b1);
      b0h = _mm_unpackhi_epi16(b0, b1);
      b1l = _mm_unpacklo_epi16(b2, b3);
      b1h = _mm_unpackhi_epi16(b2, b3);
      sum0 = _mm_madd_epi16(b0l, _mm_setr_epi16(-1, 4, -1, 4, -1, 4, -1, 4));
      sum1 = _mm_madd_epi16(b0h, _mm_setr_epi16(-1, 4, -1, 4, -1, 4, -1, 4));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, _mm_setr_epi16(-11, 40, -11, 40, -11, 40, -11, 40)));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, _mm_setr_epi16(-11, 40, -11, 40, -11, 40, -11, 40)));
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;
      a6 = a7;

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);
      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
    }
  }
}

void LumaInterpolationFilterPoshOutRaw16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_S16               *dst,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a8, a7;
  __m128i b0, b1, b2, b3, b4, b5, b6, b7;

  (void)firstPassIFDst;
  refPic -= 3*srcStride;

  if (puWidth & 4) {
    ptr = refPic;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi64(a0, a1);
    b1 = _mm_unpacklo_epi64(a1, a2);
    b2 = _mm_unpacklo_epi64(a2, a3);
    b3 = _mm_unpacklo_epi64(a3, a4);
    b4 = _mm_unpacklo_epi64(a4, a5);
    b5 = _mm_unpacklo_epi64(a5, a6);
    b6 = _mm_unpacklo_epi64(a6, a7);

    for (i=0; i<puHeight; i+=2) {
      a8 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b7 = _mm_unpacklo_epi64(a7, a8);

      // Use symmetry
      a0 = _mm_add_epi16(b0, b7);
      a1 = _mm_add_epi16(b1, b6);
      a2 = _mm_add_epi16(b2, b5);
      a3 = _mm_add_epi16(b3, b4);

      // Process taps with odd values, shift right by 2
      a2 = _mm_mullo_epi16(a2, _mm_set1_epi16(-11));
      a0 = _mm_sub_epi16(a2, a0);
      a0 = _mm_srai_epi16(a0, 2);

      // Process taps with values that are multiples of 4
      a1 = _mm_add_epi16(a1, a0);
      a3 = _mm_mullo_epi16(a3, _mm_set1_epi16(10));
      a1 = _mm_add_epi16(a1, a3);
      a1 = _mm_add_epi16(a1, _mm_set1_epi16(OFFSET2D1_10BIT>>2));
      //a1 = _mm_srai_epi16(a1, SHIFT2D1_10BIT-2);

      _mm_storeu_si128((__m128i *)dst, a1);
      dst += 8;
      b0 = b2;
      b1 = b3;
      b2 = b4;
      b3 = b5;
      b4 = b6;
      b5 = b7;
      a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b6 = _mm_unpacklo_epi64(a8, a7);
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

    for (j=0; j<puHeight; j++) {
      a7 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      // Use symmetry
      b0 = _mm_add_epi16(a0, a7);
      b1 = _mm_add_epi16(a1, a6);
      b2 = _mm_add_epi16(a2, a5);
      b3 = _mm_add_epi16(a3, a4);

      // Process taps with odd values, shift right by 2
      b2 = _mm_mullo_epi16(b2, _mm_set1_epi16(-11));
      b0 = _mm_sub_epi16(b2, b0);
      b0 = _mm_srai_epi16(b0, 2);

      // Process taps with values that are multiples of 4
      b1 = _mm_add_epi16(b1, b0);
      b3 = _mm_mullo_epi16(b3, _mm_set1_epi16(10));
      b1 = _mm_add_epi16(b1, b3);
      b1 = _mm_add_epi16(b1, _mm_set1_epi16(OFFSET2D1_10BIT>>2));
      //b1 = _mm_srai_epi16(b1, SHIFT2D1_10BIT-2);

      _mm_storeu_si128((__m128i *)dst, b1);
      dst += 8;
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;
      a6 = a7;
    }
    refPic += 8;
  }
}

void LumaInterpolationFilterPosn16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_U16               *dst,
  EB_U32                dstStride,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr, *qtr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i b0, b1, b2, b3, b4, b5, b6;

  (void)firstPassIFDst;
  refPic -= 2*srcStride;

  if (puWidth & 4) {
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi64(a0, a1);
    b1 = _mm_unpacklo_epi64(a1, a2);
    b2 = _mm_unpacklo_epi64(a2, a3);
    b3 = _mm_unpacklo_epi64(a3, a4);
    b4 = _mm_unpacklo_epi64(a4, a5);
    b5 = _mm_unpacklo_epi64(a5, a6);

    for (i=0; i<puHeight; i+=2) {
      a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b6 = _mm_unpacklo_epi64(a6, a7);

      // Process taps with odd values, shift right by 1
      a0 = _mm_mullo_epi16(b2, _mm_set1_epi16(17)); // 0:17391
      a1 = _mm_mullo_epi16(b1, _mm_set1_epi16(-5)); // -5115:0
      a2 = _mm_sub_epi16(b0, b6); // -1023:1023
      a2 = _mm_add_epi16(a2, a0); // -1023:18414
      a2 = _mm_add_epi16(a2, a1); // -6138:18414
      a2 = _mm_srai_epi16(a2, 1); // -3069:9207

      // Process taps with even values
      a3 = _mm_add_epi16(b5, b5); // 0:2046
      a4 = _mm_mullo_epi16(b4, _mm_set1_epi16(-5)); // -5115:0
      a5 = _mm_mullo_epi16(b3, _mm_set1_epi16(29)); // 0:29667

      a2 = _mm_add_epi16(a2, a3); // -3069:11253
      a2 = _mm_add_epi16(a2, a4); // -8184:11253
      a2 = _mm_adds_epi16(a2, a5); // -8184:32767

      // Shift right by 5 with rounding
      a2 = _mm_adds_epi16(a2, _mm_set1_epi16(16)); // -8168:32767
      a2 = _mm_srai_epi16(a2, 5); // -256:1023

      // Clip to valid range
      a2 = _mm_max_epi16(a2, _mm_setzero_si128());
      _mm_storel_epi64((__m128i *)qtr, a2);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), _mm_srli_si128(a2, 8));
      qtr += 2*dstStride;
      b0 = b2;
      b1 = b3;
      b2 = b4;
      b3 = b5;
      b4 = b6;
      a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b5 = _mm_unpacklo_epi64(a7, a6);
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
    dst += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

    for (j=0; j<puHeight; j++) {
      a6 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      // Process taps with odd values, shift right by 1
      b0 = _mm_mullo_epi16(a2, _mm_set1_epi16(17)); // 0:17391
      b1 = _mm_mullo_epi16(a1, _mm_set1_epi16(-5)); // -5115:0
      b2 = _mm_sub_epi16(a0, a6); // -1023:1023
      b2 = _mm_add_epi16(b2, b0); // -1023:18414
      b2 = _mm_add_epi16(b2, b1); // -6138:18414
      b2 = _mm_srai_epi16(b2, 1); // -3069:9207

      // Process taps with even values
      b3 = _mm_add_epi16(a5, a5); // 0:2046
      b4 = _mm_mullo_epi16(a4, _mm_set1_epi16(-5)); // -5115:0
      b5 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      b2 = _mm_add_epi16(b3, b2); // -3069:11253
      b2 = _mm_add_epi16(b4, b2); // -8184:11253
      b2 = _mm_adds_epi16(b5, b2); // -8184:32767

      // Shift right by 5 with rounding
      b2 = _mm_adds_epi16(b2, _mm_set1_epi16(16)); // -8168:32767
      b2 = _mm_srai_epi16(b2, 5); // -256:1023

      // Clip to valid range
      b2 = _mm_max_epi16(b2, _mm_setzero_si128());
      _mm_storeu_si128((__m128i *)qtr, b2);
      qtr += dstStride;
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;
    }
    refPic += 8;
    dst += 8;
  }
}

void LumaInterpolationFilterPosnOutRaw16bit_SSE2_INTRIN(
  EB_U16               *refPic,
  EB_U32                srcStride,
  EB_S16               *dst,
  EB_U32                puWidth,
  EB_U32                puHeight,
  EB_S16               *firstPassIFDst)
{
  EB_U32 i, j;
  EB_U16 *ptr;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i b0, b1, b2, b3, b4, b5, b6;

  (void)firstPassIFDst;
  refPic -= 2*srcStride;

  if (puWidth & 4) {
    ptr = refPic;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi64(a0, a1);
    b1 = _mm_unpacklo_epi64(a1, a2);
    b2 = _mm_unpacklo_epi64(a2, a3);
    b3 = _mm_unpacklo_epi64(a3, a4);
    b4 = _mm_unpacklo_epi64(a4, a5);
    b5 = _mm_unpacklo_epi64(a5, a6);

    for (i=0; i<puHeight; i+=2) {
      a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b6 = _mm_unpacklo_epi64(a6, a7);

      // Process taps with odd values, shift right by 1
      a0 = _mm_mullo_epi16(b2, _mm_set1_epi16(17)); // 0:17391
      a1 = _mm_mullo_epi16(b1, _mm_set1_epi16(-5)); // -5115:0
      a2 = _mm_sub_epi16(b0, b6); // -1023:1023
      a2 = _mm_add_epi16(a2, a0); // -1023:18414
      a2 = _mm_add_epi16(a2, a1); // -6138:18414
      a2 = _mm_srai_epi16(a2, 1); // -3069:9207

      // Process taps with even values
      a3 = _mm_add_epi16(b5, b5); // 0:2046
      a4 = _mm_mullo_epi16(b4, _mm_set1_epi16(-5)); // -5115:0
      a5 = _mm_mullo_epi16(b3, _mm_set1_epi16(29)); // 0:29667

      a2 = _mm_add_epi16(a2, a3); // -3069:11253
      a2 = _mm_add_epi16(a2, a4); // -8184:11253
      a2 = _mm_add_epi16(a2, _mm_set1_epi16(OFFSET2D1_10BIT>>1)); // -24568:-5131
      a2 = _mm_adds_epi16(a2, a5); // -24568:24536
      a2 = _mm_srai_epi16(a2, SHIFT2D1_10BIT-1);

      _mm_storeu_si128((__m128i *)dst, a2);
      dst += 8;
      b0 = b2;
      b1 = b3;
      b2 = b4;
      b3 = b5;
      b4 = b6;
      a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b5 = _mm_unpacklo_epi64(a7, a6);
    }

    puWidth -= 4;
    if (!puWidth) {
      return;
    }
    refPic += 4;
  }

  for (i=0; i<puWidth; i+=8) {
    ptr = refPic;
    a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

    for (j=0; j<puHeight; j++) {
      a6 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      // Process taps with odd values, shift right by 1
      b0 = _mm_mullo_epi16(a2, _mm_set1_epi16(17)); // 0:17391
      b1 = _mm_mullo_epi16(a1, _mm_set1_epi16(-5)); // -5115:0
      b2 = _mm_sub_epi16(a0, a6); // -1023:1023
      b2 = _mm_add_epi16(b2, b0); // -1023:18414
      b2 = _mm_add_epi16(b2, b1); // -6138:18414
      b2 = _mm_srai_epi16(b2, 1); // -3069:9207

      // Process taps with even values
      b3 = _mm_add_epi16(a5, a5); // 0:2046
      b4 = _mm_mullo_epi16(a4, _mm_set1_epi16(-5)); // -5115:0
      b5 = _mm_mullo_epi16(a3, _mm_set1_epi16(29)); // 0:29667

      b2 = _mm_add_epi16(b3, b2); // -3069:11253
      b2 = _mm_add_epi16(b4, b2); // -8184:11253
      b2 = _mm_add_epi16(b2, _mm_set1_epi16(OFFSET2D1_10BIT>>1)); // -8168:32767
      b2 = _mm_adds_epi16(b5, b2); // -8184:32767
      b2 = _mm_srai_epi16(b2, SHIFT2D1_10BIT-1); // -256:1023

      _mm_storeu_si128((__m128i *)dst, b2);
      dst += 8;
      a0 = a1;
      a1 = a2;
      a2 = a3;
      a3 = a4;
      a4 = a5;
      a5 = a6;
    }
    refPic += 8;
  }
}

void ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN(
EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy)
{
    EB_U32 rowCount, colCount;
    EB_U16 *ptr, *qtr;
    __m128i a0, a1, a2, a3, b0, b1, b2, b3, c0, c1, c2, c3, sum;

  (void)firstPassIFDst;
  (void)fracPosy;

  refPic--;
  //PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

  c0 = _mm_loadl_epi64((__m128i *)chromaFilterCoeffSR1[fracPosx]);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 2) {
    ptr = refPic;
    qtr = dst;
    for (rowCount=0; rowCount<puHeight; rowCount+=4) {
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

      sum = _mm_set1_epi16(OFFSET1D_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a1, c1));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a3, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(a2, c2));
      sum = _mm_srai_epi16(sum, SHIFT1D_10BIT-1);
      sum = _mm_max_epi16(sum, _mm_setzero_si128());

      *(EB_U32 *)qtr = _mm_cvtsi128_si32(sum); qtr += dstStride; sum = _mm_srli_si128(sum, 4);
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(sum); qtr += dstStride; sum = _mm_srli_si128(sum, 4);
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(sum); qtr += dstStride; sum = _mm_srli_si128(sum, 4);
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(sum); qtr += dstStride;
    }

    puWidth -= 2;
    if (!puWidth) {
      return;
    }
    refPic += 2;
    dst += 2;
  }

  if (puWidth & 4) {
    ptr = refPic;
    qtr = dst;
    for (rowCount=0; rowCount<puHeight; rowCount+=2) {
      // Need 5 samples, load 8
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride; // 00 01 02 03 04 05 06 ..
      a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride; // 10 11 12 13 14 15 16 ..

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

      sum = _mm_set1_epi16(OFFSET1D_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b1, c1));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b3, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(b2, c2));
      sum = _mm_srai_epi16(sum, SHIFT1D_10BIT-1);
      sum = _mm_max_epi16(sum, _mm_setzero_si128());

      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride; sum = _mm_srli_si128(sum, 8);
      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    refPic += 4;
    dst += 4;
  }

  for (rowCount=0; rowCount<puHeight; rowCount++) {
    ptr = refPic;
    qtr = dst;
    for (colCount=0; colCount<puWidth; colCount+=8) {
      a0 = _mm_loadu_si128((__m128i *)ptr);
      a1 = _mm_loadu_si128((__m128i *)(ptr+1));
      a2 = _mm_loadu_si128((__m128i *)(ptr+2));
      a3 = _mm_loadu_si128((__m128i *)(ptr+3));
      ptr += 8;

      sum = _mm_set1_epi16(OFFSET1D_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a1, c1));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a3, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(a2, c2));
      sum = _mm_srai_epi16(sum, SHIFT1D_10BIT-1);
      sum = _mm_max_epi16(sum, _mm_setzero_si128());

      _mm_storeu_si128((__m128i *)qtr, sum);
      qtr += 8;
    }
    refPic += srcStride;
    dst += dstStride;
  }
}

void ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN(
EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy)
{
    EB_U32 rowCount, colCount;
    EB_U16 *ptr, *qtr;
    __m128i a0, a1, a2, a3, b0, b1, c0, c1, c2, c3;
    __m128i sum;

  (void)firstPassIFDst;
  (void)fracPosx;

  c0 = _mm_loadl_epi64((__m128i *)chromaFilterCoeffSR1[fracPosy]);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  refPic -= srcStride;
  //PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

  if (puWidth & 2) {
      __m128i a4, a5, b2, b3, d0, d1, d2, d3;
    ptr = refPic;
    qtr = dst;

    a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi32(a0, a1);
    b1 = _mm_unpacklo_epi32(a1, a2);
    for (rowCount=0; rowCount<puHeight; rowCount+=4) {
      a3 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a4 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b2 = _mm_unpacklo_epi32(a2, a3);
      b3 = _mm_unpacklo_epi32(a3, a4);
      d0 = _mm_unpacklo_epi64(b0, b2);
      d1 = _mm_unpacklo_epi64(b1, b3);
      a5 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b0 = _mm_unpacklo_epi32(a4, a5);
      b1 = _mm_unpacklo_epi32(a5, a2);
      d2 = _mm_unpacklo_epi64(b2, b0);
      d3 = _mm_unpacklo_epi64(b3, b1);

      sum = _mm_set1_epi16(OFFSET1D_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(d0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(d1, c1));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(d3, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(d2, c2));
      sum = _mm_srai_epi16(sum, SHIFT1D_10BIT-1);
      sum = _mm_max_epi16(sum, _mm_setzero_si128());

      *(EB_U32 *)qtr = _mm_cvtsi128_si32(sum); qtr += dstStride; sum = _mm_srli_si128(sum, 4);
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(sum); qtr += dstStride; sum = _mm_srli_si128(sum, 4);
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(sum); qtr += dstStride; sum = _mm_srli_si128(sum, 4);
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(sum); qtr += dstStride;
    }

    puWidth -= 2;
    if (!puWidth) {
      return;
    }
    refPic += 2;
    dst += 2;
  }

  if (puWidth & 4) {
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi64(a0, a1);
    b1 = _mm_unpacklo_epi64(a1, a2);
    for (rowCount=0; rowCount<puHeight; rowCount+=2) {
      sum = _mm_set1_epi16(OFFSET1D_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b1, c1));
      a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b0 = _mm_unpacklo_epi64(a2, a3);
      a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;

      b1 = _mm_unpacklo_epi64(a3, a2);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b1, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(b0, c2));
      sum = _mm_srai_epi16(sum, SHIFT1D_10BIT-1);
      sum = _mm_max_epi16(sum, _mm_setzero_si128());

      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride; sum = _mm_srli_si128(sum, 8);
      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    refPic += 4;
    dst += 4;
  }

  for (colCount=0; colCount<puWidth; colCount+=8) {
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadu_si128((__m128i *)ptr);
    a1 = _mm_loadu_si128((__m128i *)(ptr+1*srcStride));
    a2 = _mm_loadu_si128((__m128i *)(ptr+2*srcStride));
    for (rowCount=0; rowCount<puHeight; rowCount++) {
      sum = _mm_set1_epi16(OFFSET1D_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a1, c1));
      a0 = a1;
      a1 = a2;
      a2 = _mm_loadu_si128((__m128i *)(ptr+3*srcStride));
      ptr += srcStride;
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a2, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(a1, c2));
      sum = _mm_srai_epi16(sum, SHIFT1D_10BIT-1);
      sum = _mm_max_epi16(sum, _mm_setzero_si128());

      _mm_storeu_si128((__m128i *)qtr, sum);
      qtr += dstStride;
    }
    refPic += 8;
    dst += 8;
  }
}

void ChromaInterpolationFilterOneDOutRaw16bitHorizontal_SSE2_INTRIN(
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

  (void)firstPassIFDst;
  (void)fracPosy;

  refPic--;
  //PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

  c0 = _mm_loadl_epi64((__m128i *)chromaFilterCoeffSR1[fracPosx]);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 2) {
    ptr = refPic;
    for (rowCount=0; rowCount<puHeight; rowCount+=4) {
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

      sum = _mm_set1_epi16(OFFSET2D1_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a1, c1));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a3, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(a2, c2));
      sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT-1);

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
    for (rowCount=0; rowCount<puHeight; rowCount+=2) {
      // Need 5 samples, load 8
      a0 = _mm_loadu_si128((__m128i *)ptr);             // 00 01 02 03 04 05 06 ..
      a1 = _mm_loadu_si128((__m128i *)(ptr+srcStride)); // 10 11 12 13 14 15 16 ..
      ptr += 2*srcStride;

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

      sum = _mm_set1_epi16(OFFSET2D1_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b1, c1));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b3, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(b2, c2));
      sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT-1);

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    refPic += 4;
  }

  for (colCount=0; colCount<puWidth; colCount+=8) {
    ptr = refPic;
      for (rowCount=0; rowCount<puHeight; rowCount++) {
      a0 = _mm_loadu_si128((__m128i *)ptr);
      a1 = _mm_loadu_si128((__m128i *)(ptr+1));
      a2 = _mm_loadu_si128((__m128i *)(ptr+2));
      a3 = _mm_loadu_si128((__m128i *)(ptr+3));
      ptr += srcStride;

      sum = _mm_set1_epi16(OFFSET2D1_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a1, c1));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a3, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(a2, c2));
      sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT-1);

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;
    }
    refPic += 8;
  }
}

void ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN(
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
    __m128i a0, a1, a2, a3, b0, b1, c0, c1, c2, c3;
    __m128i sum;

  (void)firstPassIFDst;
  (void)fracPosx;

  c0 = _mm_loadl_epi64((__m128i *)chromaFilterCoeffSR1[fracPosy]);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  refPic -= srcStride;
  //PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

  if (puWidth & 2) {
      __m128i a4, a5, b2, b3, d0, d1, d2, d3;
    ptr = refPic;

    a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi32(a0, a1);
    b1 = _mm_unpacklo_epi32(a1, a2);
    for (rowCount=0; rowCount<puHeight; rowCount+=4) {
      a3 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a4 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b2 = _mm_unpacklo_epi32(a2, a3);
      b3 = _mm_unpacklo_epi32(a3, a4);
      d0 = _mm_unpacklo_epi64(b0, b2);
      d1 = _mm_unpacklo_epi64(b1, b3);
      a5 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b0 = _mm_unpacklo_epi32(a4, a5);
      b1 = _mm_unpacklo_epi32(a5, a2);
      d2 = _mm_unpacklo_epi64(b2, b0);
      d3 = _mm_unpacklo_epi64(b3, b1);

      sum = _mm_set1_epi16(OFFSET2D1_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(d0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(d1, c1));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(d3, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(d2, c2));
      sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT-1);

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
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi64(a0, a1);
    b1 = _mm_unpacklo_epi64(a1, a2);
    for (rowCount=0; rowCount<puHeight; rowCount+=2) {
      sum = _mm_set1_epi16(OFFSET2D1_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b1, c1));
      a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b0 = _mm_unpacklo_epi64(a2, a3);
      a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;

      b1 = _mm_unpacklo_epi64(a3, a2);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(b1, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(b0, c2));
      sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT-1);

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    refPic += 4;
  }

  for (colCount=0; colCount<puWidth; colCount+=8) {
    ptr = refPic;
    a0 = _mm_loadu_si128((__m128i *)ptr);
    a1 = _mm_loadu_si128((__m128i *)(ptr+1*srcStride));
    a2 = _mm_loadu_si128((__m128i *)(ptr+2*srcStride));
    for (rowCount=0; rowCount<puHeight; rowCount++) {
      sum = _mm_set1_epi16(OFFSET2D1_10BIT>>1);
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a0, c0));
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a1, c1));
      a0 = a1;
      a1 = a2;
      a2 = _mm_loadu_si128((__m128i *)(ptr+3*srcStride));
      ptr += srcStride;
      sum = _mm_add_epi16(sum,  _mm_mullo_epi16(a2, c3));
      sum = _mm_adds_epi16(sum, _mm_mullo_epi16(a1, c2));
      sum = _mm_srai_epi16(sum, SHIFT2D1_10BIT-1);

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;
    }
    refPic += 8;
  }
}

void ChromaInterpolationFilterTwoDInRaw16bit_SSE2_INTRIN(
    EB_S16               *firstPassIFDst,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                fracPosy)
{
    EB_U32 rowCount, colCount;
    EB_U16 *qtr;
    __m128i a0, a1, a2, a3, b0, b1, b2, b3, c0, c1;
    __m128i sum0, sum1;

  //PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

  c0 = _mm_loadl_epi64((__m128i *)chromaFilterCoeffSR1[fracPosy]);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0);

  if (puWidth & 2) {
    qtr = dst;
    for (rowCount=0; rowCount<puHeight; rowCount+=4) {
      a0 = _mm_load_si128((__m128i *)firstPassIFDst);
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6));

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      b2 = _mm_unpacklo_epi16(a2, a3);
      b3 = _mm_unpackhi_epi16(a2, a3);

      sum0 = sum1 = _mm_set1_epi32(OFFSET2D2_10BIT>>1);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b0, c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1, c0));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b2, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, SHIFT2D2_10BIT-1);
      sum1 = _mm_srai_epi32(sum1, SHIFT2D2_10BIT-1);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_max_epi16(sum0, _mm_setzero_si128());
      sum0 = _mm_min_epi16(sum0, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));

      *(EB_U32 *)qtr             = _mm_cvtsi128_si32(sum0); sum0 = _mm_srli_si128(sum0, 4);
      *(EB_U32 *)(qtr+dstStride) = _mm_cvtsi128_si32(sum0); sum0 = _mm_srli_si128(sum0, 4); qtr += 2*dstStride;
      *(EB_U32 *)qtr             = _mm_cvtsi128_si32(sum0); sum0 = _mm_srli_si128(sum0, 4);
      *(EB_U32 *)(qtr+dstStride) = _mm_cvtsi128_si32(sum0); qtr += 2*dstStride;
      firstPassIFDst += 8;
    }

    puWidth -= 2;
    if (!puWidth) {
      return;
    }
    firstPassIFDst += 8;
    dst += 2;
  }

  if (puWidth & 4) {
    qtr = dst;
    a0 = _mm_load_si128 ((__m128i *)firstPassIFDst);
    a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
    for (rowCount=0; rowCount<puHeight; rowCount+=2) {
      firstPassIFDst += 8;
      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      a0 = _mm_load_si128 ((__m128i *)firstPassIFDst);
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      b2 = _mm_unpacklo_epi16(a0, a1);
      b3 = _mm_unpackhi_epi16(a0, a1);

      sum0 = sum1 = _mm_set1_epi32(OFFSET2D2_10BIT>>1);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b0, c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1, c0));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b2, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, SHIFT2D2_10BIT-1);
      sum1 = _mm_srai_epi32(sum1, SHIFT2D2_10BIT-1);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_max_epi16(sum0, _mm_setzero_si128());
      sum0 = _mm_min_epi16(sum0, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));

      _mm_storel_epi64((__m128i *)qtr, sum0);
      sum0 = _mm_srli_si128(sum0, 8);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), sum0);
      qtr += 2*dstStride;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    firstPassIFDst += 16;
    dst += 4;
  }

  for (colCount=0; colCount<puWidth; colCount+=8) {
    qtr = dst;
    a0 = _mm_load_si128((__m128i *)firstPassIFDst);
    a1 = _mm_load_si128((__m128i *)(firstPassIFDst+8));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+16));
    for (rowCount=0; rowCount<puHeight; rowCount++) {
      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      a0 = a1;
      a1 = a2;
      a2 = _mm_load_si128((__m128i *)(firstPassIFDst+24));
      b2 = _mm_unpacklo_epi16(a1, a2);
      b3 = _mm_unpackhi_epi16(a1, a2);

      sum0 = sum1 = _mm_set1_epi32(OFFSET2D2_10BIT>>1);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b0, c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1, c0));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b2, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, SHIFT2D2_10BIT-1);
      sum1 = _mm_srai_epi32(sum1, SHIFT2D2_10BIT-1);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_max_epi16(sum0, _mm_setzero_si128());
      sum0 = _mm_min_epi16(sum0, _mm_set1_epi16(MAX_SAMPLE_VALUE_10BIT));

      _mm_storeu_si128((__m128i *)qtr, sum0);
      firstPassIFDst += 8;
      qtr += dstStride;
    }
    firstPassIFDst += 24;
    dst += 8;
  }
}

void ChromaInterpolationFilterTwoDInRawOutRaw_SSE2_INTRIN(
    EB_S16               *firstPassIFDst,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_U32                fracPosy)
{
    EB_U32 rowCount, colCount;
    __m128i a0, a1, a2, a3, b0, b1, b2, b3, c0, c1;
    __m128i sum0, sum1;

  //PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

  c0 = _mm_loadl_epi64((__m128i *)chromaFilterCoeffSR1[fracPosy]);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0);

  if (puWidth & 2) {
    for (rowCount=0; rowCount<puHeight; rowCount+=4) {
      a0 = _mm_load_si128((__m128i *)firstPassIFDst);
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6));

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      b2 = _mm_unpacklo_epi16(a2, a3);
      b3 = _mm_unpackhi_epi16(a2, a3);

      sum0 = _mm_madd_epi16(b0, c0);
      sum1 = _mm_madd_epi16(b1, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b2, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, 6-1);
      sum1 = _mm_srai_epi32(sum1, 6-1);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      firstPassIFDst += 8;
      dst += 8;
    }

    puWidth -= 2;
    if (!puWidth) {
      return;
    }
    firstPassIFDst += 8;
  }

  if (puWidth & 4) {
    a0 = _mm_load_si128 ((__m128i *)firstPassIFDst);
    a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
    for (rowCount=0; rowCount<puHeight; rowCount+=2) {
      firstPassIFDst += 8;
      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      a0 = _mm_load_si128 ((__m128i *)firstPassIFDst);
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      b2 = _mm_unpacklo_epi16(a0, a1);
      b3 = _mm_unpackhi_epi16(a0, a1);

      sum0 = _mm_madd_epi16(b0, c0);
      sum1 = _mm_madd_epi16(b1, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b2, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, 6-1);
      sum1 = _mm_srai_epi32(sum1, 6-1);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
    }

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    firstPassIFDst += 16;
  }

  for (colCount=0; colCount<puWidth; colCount+=8) {
    a0 = _mm_load_si128((__m128i *)firstPassIFDst);
    a1 = _mm_load_si128((__m128i *)(firstPassIFDst+8));
    a2 = _mm_load_si128((__m128i *)(firstPassIFDst+16));
    for (rowCount=0; rowCount<puHeight; rowCount++) {
      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      a0 = a1;
      a1 = a2;
      a2 = _mm_load_si128((__m128i *)(firstPassIFDst+24));
      b2 = _mm_unpacklo_epi16(a1, a2);
      b3 = _mm_unpackhi_epi16(a1, a2);

      sum0 = _mm_madd_epi16(b0, c0);
      sum1 = _mm_madd_epi16(b1, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b2, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, 6-1);
      sum1 = _mm_srai_epi32(sum1, 6-1);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
    }
    firstPassIFDst += 24;
  }
}

void BiPredClipping16bit_SSE2_INTRIN(
    EB_U32     puWidth,
    EB_U32     puHeight,
    EB_S16    *list0Src,
    EB_S16    *list1Src,
    EB_U16    *dst,
    EB_U32     dstStride)
{
  EB_U32 rowCount, colCount;
  __m128i a0, a1;

  if (puWidth & 2) {
    EB_U16 *qtr = dst;
    rowCount = puHeight;
    do {
      a0 = _mm_load_si128((__m128i *)list0Src);
      a1 = _mm_load_si128((__m128i *)list1Src);
      list0Src += 8;
      list1Src += 8;

      a0 = _mm_adds_epi16(a0, a1);
      a0 = _mm_adds_epi16(a0, _mm_set1_epi16(BI_AVG_OFFSET_10BIT));
      a0 = _mm_srai_epi16(a0, BI_AVG_SHIFT_10BIT);
      a0 = _mm_max_epi16(a0, _mm_setzero_si128());

      *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); a0 = _mm_srli_si128(a0, 4); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); a0 = _mm_srli_si128(a0, 4); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); a0 = _mm_srli_si128(a0, 4); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); qtr += dstStride;
      rowCount -= 4;
    }
    while (rowCount != 0);

    puWidth -= 2;
    if (puWidth == 0) {
      return;
    }
    dst += 2;
  }

  if (puWidth & 4) {
    EB_U16 *qtr = dst;
    rowCount = puHeight;
    do {
      a0 = _mm_load_si128((__m128i *)list0Src);
      a1 = _mm_load_si128((__m128i *)list1Src);
      list0Src += 8;
      list1Src += 8;

      a0 = _mm_adds_epi16(a0, a1);
      a0 = _mm_adds_epi16(a0, _mm_set1_epi16(BI_AVG_OFFSET_10BIT));
      a0 = _mm_srai_epi16(a0, BI_AVG_SHIFT_10BIT);
      a0 = _mm_max_epi16(a0, _mm_setzero_si128());
      _mm_storel_epi64((__m128i *)qtr, a0);
      a0 = _mm_srli_si128(a0, 8);
      _mm_storel_epi64((__m128i *)(qtr+dstStride), a0); qtr += 2*dstStride;
      rowCount -= 2;
    }
    while (rowCount != 0);

    puWidth -= 4;
    if (puWidth == 0) {
      return;
    }
    dst += 4;
  }

  colCount = puWidth;
  do {
    __m128i a2, a3;
    EB_U16 *qtr = dst;
    rowCount = puHeight;
    do {
      a0 = _mm_load_si128((__m128i *)list0Src);
      a1 = _mm_load_si128((__m128i *)list1Src);
      list0Src += 8;
      list1Src += 8;
      a0 = _mm_adds_epi16(a0, a1);
      a0 = _mm_adds_epi16(a0, _mm_set1_epi16(BI_AVG_OFFSET_10BIT));
      a0 = _mm_srai_epi16(a0, BI_AVG_SHIFT_10BIT);
      a0 = _mm_max_epi16(a0, _mm_setzero_si128());

      a2 = _mm_load_si128((__m128i *)list0Src);
      a3 = _mm_load_si128((__m128i *)list1Src);
      list0Src += 8;
      list1Src += 8;
      a2 = _mm_adds_epi16(a2, a3);
      a2 = _mm_adds_epi16(a2, _mm_set1_epi16(BI_AVG_OFFSET_10BIT));
      a2 = _mm_srai_epi16(a2, BI_AVG_SHIFT_10BIT);
      a2 = _mm_max_epi16(a2, _mm_setzero_si128());

      _mm_storeu_si128((__m128i *)qtr,             a0);
      _mm_storeu_si128((__m128i *)(qtr+dstStride), a2);
      qtr += 2*dstStride;

      rowCount -= 2;
    }
    while (rowCount != 0);

    colCount -= 8;
    dst += 8;
  }
  while (colCount != 0);
}

    void BiPredClippingOnTheFly16bit_SSE2(
        EB_U16    *list0Src,
        EB_U32     list0SrcStride,
        EB_U16    *list1Src,
        EB_U32     list1SrcStride,
        EB_U16    *dst,
        EB_U32     dstStride,
        EB_U32     puWidth,
        EB_U32     puHeight)
    {
        EB_U32 rowCount, colCount;
        __m128i a0, a1;
        __m128i a2, a3;

        if (puWidth & 2) {
            __m128i a4;
            EB_U16 *qtr = dst;
            EB_U16 *ptrSrc0 = list0Src;
            EB_U16 *ptrSrc1 = list1Src;
            rowCount = puHeight;
            do {

                a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc0); ptrSrc0 += list0SrcStride;
                a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc0); ptrSrc0 += list0SrcStride;
                a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc0); ptrSrc0 += list0SrcStride;
                a3 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc0); ptrSrc0 += list0SrcStride;
                a0 = _mm_unpacklo_epi32(a0, a1);
                a2 = _mm_unpacklo_epi32(a2, a3);
                a0 = _mm_unpacklo_epi64(a0, a2);
                a0 = _mm_slli_epi16(a0, BI_SHIFT_10BIT);
                a0 = _mm_sub_epi16(a0, _mm_set1_epi16(BI_OFFSET_10BIT));

                a4 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc1); ptrSrc1 += list0SrcStride;
                a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc1); ptrSrc1 += list0SrcStride;
                a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc1); ptrSrc1 += list0SrcStride;
                a3 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc1); ptrSrc1 += list0SrcStride;
                a4 = _mm_unpacklo_epi32(a4, a1);
                a2 = _mm_unpacklo_epi32(a2, a3);
                a4 = _mm_unpacklo_epi64(a4, a2);
                a4 = _mm_slli_epi16(a4, BI_SHIFT_10BIT);
                a4 = _mm_sub_epi16(a4, _mm_set1_epi16(BI_OFFSET_10BIT));


                a0 = _mm_adds_epi16(a0, a4);
                a0 = _mm_adds_epi16(a0, _mm_set1_epi16(BI_AVG_OFFSET_10BIT));
                a0 = _mm_srai_epi16(a0, BI_AVG_SHIFT_10BIT);
                a0 = _mm_max_epi16(a0, _mm_setzero_si128());

                *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); a0 = _mm_srli_si128(a0, 4); qtr += dstStride;
                *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); a0 = _mm_srli_si128(a0, 4); qtr += dstStride;
                *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); a0 = _mm_srli_si128(a0, 4); qtr += dstStride;
                *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); qtr += dstStride;
                rowCount -= 4;
            } while (rowCount != 0);

            puWidth -= 2;
            if (puWidth == 0) {
                return;
            }

            list0Src += 2;
            list1Src += 2;
            dst += 2;
        }

        if (puWidth & 4) {
            EB_U16 *qtr = dst;
            EB_U16 *ptrSrc0 = list0Src;
            EB_U16 *ptrSrc1 = list1Src;
            rowCount = puHeight;
            do {

                a0 = _mm_loadl_epi64((__m128i *)ptrSrc0);
                ptrSrc0 += list0SrcStride;
                a1 = _mm_loadl_epi64((__m128i *)ptrSrc0);
                ptrSrc0 += list0SrcStride;
                a0 = _mm_unpacklo_epi64(a0, a1);
                a0 = _mm_slli_epi16(a0, BI_SHIFT_10BIT);
                a0 = _mm_sub_epi16(a0, _mm_set1_epi16(BI_OFFSET_10BIT));

                a2 = _mm_loadl_epi64((__m128i *)ptrSrc1);
                ptrSrc1 += list1SrcStride;
                a3 = _mm_loadl_epi64((__m128i *)ptrSrc1);
                ptrSrc1 += list1SrcStride;
                a2 = _mm_unpacklo_epi64(a2, a3);
                a2 = _mm_slli_epi16(a2, BI_SHIFT_10BIT);
                a2 = _mm_sub_epi16(a2, _mm_set1_epi16(BI_OFFSET_10BIT));

                a0 = _mm_adds_epi16(a0, a2/*a1*/);
                a0 = _mm_adds_epi16(a0, _mm_set1_epi16(BI_AVG_OFFSET_10BIT));
                a0 = _mm_srai_epi16(a0, BI_AVG_SHIFT_10BIT);
                a0 = _mm_max_epi16(a0, _mm_setzero_si128());
                _mm_storel_epi64((__m128i *)qtr, a0);
                a0 = _mm_srli_si128(a0, 8);
                _mm_storel_epi64((__m128i *)(qtr + dstStride), a0); qtr += 2 * dstStride;
                rowCount -= 2;
            } while (rowCount != 0);

            puWidth -= 4;
            if (puWidth == 0) {
                return;
            }

            list0Src += 4;
            list1Src += 4;
            dst += 4;
        }

        colCount = puWidth;
        do {
            EB_U16 *qtr = dst;
            EB_U16 *ptrSrc0 = list0Src;
            EB_U16 *ptrSrc1 = list1Src;
            rowCount = puHeight;
            do {
                a0 = _mm_loadu_si128((__m128i *)ptrSrc0);
                a1 = _mm_loadu_si128((__m128i *)ptrSrc1);
                ptrSrc0 += list0SrcStride;
                ptrSrc1 += list1SrcStride;
                a0 = _mm_slli_epi16(a0, BI_SHIFT_10BIT);
                a0 = _mm_sub_epi16(a0, _mm_set1_epi16(BI_OFFSET_10BIT));
                a1 = _mm_slli_epi16(a1, BI_SHIFT_10BIT);
                a1 = _mm_sub_epi16(a1, _mm_set1_epi16(BI_OFFSET_10BIT));

                a0 = _mm_adds_epi16(a0, a1);
                a0 = _mm_adds_epi16(a0, _mm_set1_epi16(BI_AVG_OFFSET_10BIT));
                a0 = _mm_srai_epi16(a0, BI_AVG_SHIFT_10BIT);
                a0 = _mm_max_epi16(a0, _mm_setzero_si128());

                a2 = _mm_loadu_si128((__m128i *)ptrSrc0);
                a3 = _mm_loadu_si128((__m128i *)ptrSrc1);
                ptrSrc0 += list0SrcStride;
                ptrSrc1 += list1SrcStride;
                a2 = _mm_slli_epi16(a2, BI_SHIFT_10BIT);
                a2 = _mm_sub_epi16(a2, _mm_set1_epi16(BI_OFFSET_10BIT));
                a3 = _mm_slli_epi16(a3, BI_SHIFT_10BIT);
                a3 = _mm_sub_epi16(a3, _mm_set1_epi16(BI_OFFSET_10BIT));
                a2 = _mm_adds_epi16(a2, a3);
                a2 = _mm_adds_epi16(a2, _mm_set1_epi16(BI_AVG_OFFSET_10BIT));
                a2 = _mm_srai_epi16(a2, BI_AVG_SHIFT_10BIT);
                a2 = _mm_max_epi16(a2, _mm_setzero_si128());

                _mm_storeu_si128((__m128i *)qtr, a0);
                _mm_storeu_si128((__m128i *)(qtr + dstStride), a2);
                qtr += 2 * dstStride;

                rowCount -= 2;
            } while (rowCount != 0);

            colCount -= 8;
            list0Src += 8;
            list1Src += 8;
            dst += 8;
        } while (colCount != 0);

    }

void LumaInterpolationFilterPose16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight, 0);
}

void LumaInterpolationFilterPosf16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight, 0);
}

void LumaInterpolationFilterPosg16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight, 0);
}

void LumaInterpolationFilterPosi16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosj16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosk16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosp16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN(refPic - 2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight, 24);
}

void LumaInterpolationFilterPosq16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN(refPic - 2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight, 24);
}

void LumaInterpolationFilterPosr16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_U16               *dst,
    EB_U32                dstStride,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN(refPic - 2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight, 24);
}

void LumaInterpolationFilterPoseOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight, 0);
}

void LumaInterpolationFilterPosfOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight, 0);
}

void LumaInterpolationFilterPosgOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight, 0);
}

void LumaInterpolationFilterPosiOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPosjOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPoskOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN(refPic - 3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPoshInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPospOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN(refPic - 2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight, 24);
}

void LumaInterpolationFilterPosqOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN(refPic - 2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight, 24);
}

void LumaInterpolationFilterPosrOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    //horizontal filtering
#if USE_PRE_COMPUTE
    //pre-compute scheme
#else
    //on-the-fly scheme
    LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN(refPic - 2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, (EB_S16 *)EB_NULL);
#endif

    //vertical filtering
    LumaInterpolationFilterPosdInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight, 24);
}

void LumaInterpolationCopyOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst)
{
    (void)firstPassIFDst;
    PictureCopyKernelOutRaw16bit_SSE2_INTRIN(refPic, srcStride, dst, puWidth, puHeight);
}

void ChromaInterpolationFilterTwoD16bit_SSE2_INTRIN(
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
    ChromaInterpolationFilterOneDOutRaw16bitHorizontal_SSE2_INTRIN(refPic - ((MaxChromaFilterTag-1)>>1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+MaxChromaFilterTag-1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    ChromaInterpolationFilterTwoDInRaw16bit_SSE2_INTRIN(firstPassIFDst, dst, dstStride, puWidth, puHeight, fracPosy);
}

void ChromaInterpolationFilterTwoDOutRaw16bit_SSE2_INTRIN(
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
    ChromaInterpolationFilterOneDOutRaw16bitHorizontal_SSE2_INTRIN(refPic - ((MaxChromaFilterTag-1)>>1)*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+MaxChromaFilterTag-1, (EB_S16 *)EB_NULL, fracPosx, 0);
#endif

    //vertical filtering
    ChromaInterpolationFilterTwoDInRawOutRaw_SSE2_INTRIN(firstPassIFDst, dst, puWidth, puHeight, fracPosy);
}

void ChromaInterpolationCopyOutRaw16bit_SSE2_INTRIN(
    EB_U16               *refPic,
    EB_U32                srcStride,
    EB_S16               *dst,
    EB_U32                puWidth,
    EB_U32                puHeight,
    EB_S16               *firstPassIFDst,
    EB_U32                fracPosx,
    EB_U32                fracPosy)
{
    (void)firstPassIFDst;
    (void)fracPosx;
    (void)fracPosy;
    PictureCopyKernelOutRaw16bit_SSE2_INTRIN(refPic, srcStride, dst, puWidth, puHeight);
}
