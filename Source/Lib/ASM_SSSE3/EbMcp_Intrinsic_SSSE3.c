/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbMcp_SSSE3.h"
#include "EbDefinitions.h"

#include "emmintrin.h"

#ifndef PREFETCH
#define PREFETCH 0 // prefetching: enables prefetching of data before interpolation
#endif

#include "tmmintrin.h"
#include "smmintrin.h"
// Note: _mm_extract_epi32 & _mm_extract_epi64 are SSE4 functions


#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
const EB_S16 EbHevcLumaFilterCoeff[4][8] =
{
  { 0, 0,  0, 64,  0,  0, 0,  0},
  {-1, 4,-10, 58, 17, -5, 1,  0},
  {-1, 4,-11, 40, 40,-11, 4, -1},
  { 0, 1, -5, 17, 58,-10, 4, -1}
};

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
const EB_S16 EbHevcLumaFilterCoeff7[4][8] =
{
  { 0, 0,  0, 64,  0,  0, 0,  0},
  {-1, 4,-10, 58, 17, -5, 1,  0},
  {-1, 4,-11, 40, 40,-11, 4, -1},
  { 1, -5, 17, 58,-10, 4, -1, 0}
};

#ifdef __GNUC__
#ifndef __cplusplus
__attribute__((visibility("hidden")))
#endif
#endif
const EB_S16 EbHevcChromaFilterCoeff[8][4] =
{
  { 0, 64,  0,  0},
  {-2, 58, 10, -2},
  {-4, 54, 16, -2},
  {-6, 46, 28, -4},
  {-4, 36, 36, -4},
  {-4, 28, 46, -6},
  {-2, 16, 54, -4},
  {-2, 10, 58, -2},
};

static void _mm_storeh_epi64(__m128i * p, __m128i x)
{
  _mm_storeh_pd((double *)p, _mm_castsi128_pd(x));
}

void PrefetchBlock(EB_U8 *src, EB_U32 srcStride, EB_U32 blkWidth, EB_U32 blkHeight)
{
#if PREFETCH
  EB_U32 rowCount = blkHeight;

  do
  {
    EB_U8 *addr0 = src;
    EB_U8 *addr1 = addr0 + blkWidth - 1;
    src += srcStride;

    _mm_prefetch((char*)addr0, _MM_HINT_T0);
    _mm_prefetch((char*)addr1, _MM_HINT_T0);
  }
  while (--rowCount != 0);
#else
  (void)src;
  (void)srcStride;
  (void)blkWidth;
  (void)blkHeight;
#endif
}

void PictureCopyKernel_SSSE3(
	EB_BYTE                  src,
	EB_U32                   srcStride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight,
	EB_U32                   bytesPerSample)
{

  EB_U32 rowCount, colCount;
  (void)bytesPerSample;


  PrefetchBlock(src, srcStride, areaWidth, areaHeight);



  if (areaWidth & 2)
  {
    EB_BYTE ptr = src;
    EB_BYTE qtr = dst;
    rowCount = areaHeight;
    do
    {
      // Note: do only one at a time to minimize number of registers used
      EB_U16 a0;
      a0 = *(EB_U16 *)ptr; ptr += srcStride;
      *(EB_U16 *)qtr = a0; qtr += dstStride;
      rowCount--;
    }
    while (rowCount != 0);
    areaWidth -= 2;
    if (areaWidth == 0)
    {
      return;
    }
    src += 2;
    dst += 2;
  }

  if (areaWidth & 4)
  {
    EB_BYTE ptr = src;
    EB_BYTE qtr = dst;
    rowCount = areaHeight;
    do
    {
      __m128i a0, a1;
      a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(a0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_cvtsi128_si32(a1); qtr += dstStride;
      rowCount -= 2;
    }
    while (rowCount != 0);
    areaWidth -= 4;
    if (areaWidth == 0)
    {
      return;
    }
    src += 4;
    dst += 4;
  }

  if (areaWidth & 8)
  {
    EB_BYTE ptr = src;
    EB_BYTE qtr = dst;
    rowCount = areaHeight;
    do
    {
      __m128i a0, a1;
      a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      _mm_storel_epi64((__m128i *)qtr, a0); qtr += dstStride;
      _mm_storel_epi64((__m128i *)qtr, a1); qtr += dstStride;
      rowCount -= 2;
    }
    while (rowCount != 0);
    areaWidth -= 8;
    if (areaWidth == 0)
    {
      return;
    }
    src += 8;
    dst += 8;
  }

  colCount = areaWidth;
  do
  {
    EB_BYTE ptr = src;
    EB_BYTE qtr = dst;
    rowCount = areaHeight;
    do
    {
      __m128i a0, a1;
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
      _mm_storeu_si128((__m128i *)qtr, a0); qtr += dstStride;
      _mm_storeu_si128((__m128i *)qtr, a1); qtr += dstStride;
      rowCount -= 2;
    }
    while (rowCount != 0);
    colCount -= 16;
    src += 16;
    dst += 16;
  }
  while (colCount != 0);
}

void ChromaInterpolationCopy_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst,
	EB_U32                fracPosx,
	EB_U32                fracPosy)
{
  (void)firstPassIFDst;
  (void)fracPosx;
  (void)fracPosy;
  PictureCopyKernel_SSSE3(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);
}

void LumaInterpolationCopy_SSSE3(
		EB_BYTE               refPic,
		EB_U32                srcStride,
		EB_BYTE               dst,
		EB_U32                dstStride,
		EB_U32                puWidth,
		EB_U32                puHeight,
		EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;
  PictureCopyKernel_SSSE3(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);
}

void EbHevcLumaInterpolationFilterTwoDInRaw7_SSSE3(EB_S16 *firstPassIFDst, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy)
{   
  EB_S32 rowCount, colCount;
  __m128i c0, c1, c2;
  __m128i a0, a1, a2, a3, a4, a5, a6;
  __m128i sum0 , sum1;
  __m128i b0l, b0h, b1l, b1h, b2l, b2h;

  EB_BYTE qtr;

  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff7[fracPosy]);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);


  if (puWidth & 4)
  {
    rowCount = puHeight;

    qtr = dst;

    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*4));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*4));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*4));
      a0 = _mm_sub_epi16(a0, a6);

      sum0 = _mm_set1_epi32(257<<11);
      sum1 = _mm_set1_epi32(257<<11);


      b0l = _mm_unpacklo_epi16(a0, a1);
      b0h = _mm_unpackhi_epi16(a0, a1);
      b1l = _mm_unpacklo_epi16(a2, a3);
      b1h = _mm_unpackhi_epi16(a2, a3);
      b2l = _mm_unpacklo_epi16(a4, a5);
      b2h = _mm_unpackhi_epi16(a4, a5);

      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b0l, c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b0h, c0));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, c1));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b2l, c2));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b2h, c2));

      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 1); qtr += dstStride;

      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += (fracPosy == 2) ? 32 : 24;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    EB_BYTE qtr = dst;

    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*8));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*8));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*8));
      a0 = _mm_sub_epi16(a0, a6);

      sum0 = _mm_set1_epi32(257<<11);
      sum1 = _mm_set1_epi32(257<<11);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a2, a3), c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a2, a3), c1));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a4, a5), c2));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a4, a5), c2));

      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      _mm_storel_epi64((__m128i *)qtr, sum0); qtr += dstStride;

      firstPassIFDst += 8;
      rowCount--;
    }
    while (rowCount > 0);

    firstPassIFDst += (fracPosy == 2) ? 56 : 48;
    dst += 8;
    colCount -= 8;
  }
  while (colCount > 0); 

}

void EbHevcLumaInterpolationFilterTwoDInRawOutRaw7_SSSE3(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy)
{
  EB_S32 rowCount, colCount;

  __m128i a0, a1, a2, a3, a4, a5, a6;
  __m128i c0, c1, c2;
  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff7[fracPosy]);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    rowCount = puHeight;

    do
    {
      __m128i sum0, sum1;
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*4));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*4));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*4));
      a0 = _mm_sub_epi16(a0, a6);

      sum0 = _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a2, a3), c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a2, a3), c1));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a4, a5), c2));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a4, a5), c2));

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;

      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += (fracPosy == 2) ? 32 : 24;
  }

  colCount = puWidth;
  do
  {
    rowCount = puHeight;
    do
    {
      __m128i b0l, b0h, b1l, b1h, b2l, b2h;
      __m128i sum0, sum1;

      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*8));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*8));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*8));
      a0 = _mm_sub_epi16(a0, a6);

      b0l = _mm_unpacklo_epi16(a0, a1);
      b0h = _mm_unpackhi_epi16(a0, a1);
      b1l = _mm_unpacklo_epi16(a2, a3);
      b1h = _mm_unpackhi_epi16(a2, a3);
      b2l = _mm_unpacklo_epi16(a4, a5);
      b2h = _mm_unpackhi_epi16(a4, a5);

      sum0 = _mm_madd_epi16(b0l, c0);
      sum1 = _mm_madd_epi16(b0h, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1l, c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b1h, c1));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b2l, c2));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b2h, c2));

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;

      firstPassIFDst += 8;
      rowCount--;
    }
    while (rowCount > 0);

    firstPassIFDst += (fracPosy == 2) ? 56 : 48;
    colCount -= 8;
  }
  while (colCount > 0);

}

void EbHevcLumaInterpolationFilterTwoDInRawM_SSSE3(EB_S16 *firstPassIFDst, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight)
{
  EB_S32 rowCount, colCount;

  __m128i c0, c1;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i sum0, sum1;

  EB_BYTE qtr;

  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff7[2]);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);



  if (puWidth & 4)
  {
    rowCount = puHeight;
    qtr = dst;

    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*4));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*4));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*4));
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+7*4));

      sum0 = _mm_set1_epi32(257<<11);
      sum1 = _mm_set1_epi32(257<<11);

      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a2, a3), c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a2, a3), c1));

      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 1); qtr += dstStride;
      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += 32;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    qtr = dst;

    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*8));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*8));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*8));
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+7*8));

      sum0 = _mm_set1_epi32(257<<11);
      sum1 = _mm_set1_epi32(257<<11);
      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0));
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a2, a3), c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a2, a3), c1));

      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      _mm_storel_epi64((__m128i *)qtr, sum0); qtr += dstStride;
      firstPassIFDst += 8;
    }
    while (--rowCount > 0);

    firstPassIFDst += 56;
    dst += 8;
    colCount -= 8;
  }
  while (colCount > 0);
}

void EbHevcLumaInterpolationFilterTwoDInRawOutRawM_SSSE3(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight)
{
  EB_S32 rowCount, colCount;

  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i c0, c1;
  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff7[2]);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    rowCount = puHeight;

    do
    {
      __m128i sum0, sum1;
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*4));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*4));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*4));
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+7*4));

      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);
      sum0 = _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a2, a3), c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a2, a3), c1));

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += 32;
  }

  colCount = puWidth;
  do
  {
    rowCount = puHeight;
    do
    {
      __m128i sum0, sum1;
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*8));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*8));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*8));
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+7*8));

      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);
      sum0 = _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(_mm_unpacklo_epi16(a2, a3), c1));
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(_mm_unpackhi_epi16(a2, a3), c1));

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
    }
    while (--rowCount > 0);

    firstPassIFDst += 56;
    colCount -= 8;
  }
  while (colCount > 0);
}

void EbHevcPictureCopyKernelOutRaw_SSSE3(
	EB_BYTE                  refPic,
	EB_U32                   srcStride,
	EB_S16                  *dst,
	EB_U32                   puWidth,
	EB_U32                   puHeight,
	EB_S16                   offset)
{

  EB_U32 rowCount, colCount;
  __m128i o;

  PrefetchBlock(refPic, srcStride, puWidth, puHeight);



  /*__m128i*/ o = _mm_set1_epi16(offset);

  if (puWidth & 2)
  {
    __m128i a0;
    EB_BYTE ptr = refPic;
    rowCount = puHeight;
    /*__m128i*/ a0 = _mm_setzero_si128();
    do
    {
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 0); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 1); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 2); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 3); ptr += srcStride;
      a0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
      a0 = _mm_slli_epi16(a0, 6);
      a0 = _mm_sub_epi16(a0, o);
      _mm_storeu_si128((__m128i *)dst, a0);

      dst += 8;
      rowCount -= 4;
    }
    while (rowCount != 0);

    puWidth -= 2;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 2;
  }

  if (puWidth & 4)
  {
    EB_BYTE ptr = refPic;
    rowCount = puHeight;
    do
    {
      __m128i a0, a1;
      a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a0 = _mm_unpacklo_epi32(a0, a1);
      a0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
      a0 = _mm_slli_epi16(a0, 6);
      a0 = _mm_sub_epi16(a0, o);
      _mm_storeu_si128((__m128i *)dst, a0);

      dst += 8;
      rowCount -= 2;
    }
    while (rowCount != 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
  }

  colCount = puWidth;
  do
  {
   __m128i a0;
    EB_BYTE ptr = refPic;
    rowCount = puHeight;
    do
    {
      /*__m128i*/ a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
      a0 = _mm_slli_epi16(a0, 6);
      a0 = _mm_sub_epi16(a0, o);
      _mm_storeu_si128((__m128i *)dst, a0);
      dst += 8;
    }
    while (--rowCount != 0);

    colCount -= 8;
    refPic += 8;
  }
  while (colCount != 0);
}

void ChromaInterpolationCopyOutRaw_SSSE3(
	EB_BYTE               refPic,
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
  EbHevcPictureCopyKernelOutRaw_SSSE3(refPic, srcStride, dst, puWidth, puHeight, 0);
}
void ChromaInterpolationFilterTwoDInRaw_SSSE3(EB_S16 *firstPassIFDst, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy)
{
  EB_U32 rowCount, colCount;
  __m128i c0, c1; // coeffs
  __m128i offset;
  EB_BYTE qtr;
  __m128i a0,a1,a2,a3,b0, b1, b2, b3;
  __m128i sum0, sum1;

  c0 = _mm_loadl_epi64((__m128i *)EbHevcChromaFilterCoeff[fracPosy]);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0);
  offset = _mm_set1_epi32(1 << 11);

  if (puWidth & 2)
  {

    rowCount = puHeight;
    qtr = dst;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6));

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpacklo_epi16(a2, a3);
      b2 = _mm_unpackhi_epi16(a0, a1);
      b3 = _mm_unpackhi_epi16(a2, a3);

      sum0 = _mm_madd_epi16(b0, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1, c1));
      sum1 = _mm_madd_epi16(b2, c0);
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_add_epi32(sum0, offset);
      sum1 = _mm_add_epi32(sum1, offset);
      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum0, 0)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum0, 1)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum0, 2)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum0, 3)); qtr += dstStride;

      firstPassIFDst += 8;
      rowCount -= 4;
    }
    while (rowCount != 0);

    puWidth -= 2;
    if (puWidth == 0)
    {
      return;
    }
    firstPassIFDst += 8;
    dst += 2;
  }

  if (puWidth & 4)
  {
    rowCount = puHeight;
    qtr = dst;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+12));
      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpacklo_epi16(a2, a3);
      b2 = _mm_unpackhi_epi16(a0, a1);
      b3 = _mm_unpackhi_epi16(a2, a3);

      sum0 = _mm_madd_epi16(b0, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1, c1));
      sum1 = _mm_madd_epi16(b2, c0);
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_add_epi32(sum0, offset);
      sum1 = _mm_add_epi32(sum1, offset);
      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 1); qtr += dstStride;

      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount != 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += 16;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    qtr = dst;
    rowCount = puHeight;
    a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
    a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
    a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));

    do
    {
     a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpacklo_epi16(a2, a3);
      b2 = _mm_unpackhi_epi16(a0, a1);
      b3 = _mm_unpackhi_epi16(a2, a3);

      sum0 = _mm_madd_epi16(b0, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1, c1));
      sum1 = _mm_madd_epi16(b2, c0);
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_add_epi32(sum0, offset);
      sum1 = _mm_add_epi32(sum1, offset);
      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);
      _mm_storel_epi64((__m128i *)qtr, sum0); qtr += dstStride;

      a0 = a1;
      a1 = a2;
      a2 = a3;
      firstPassIFDst += 8;
    }
    while (--rowCount != 0);

    firstPassIFDst += 24;
    colCount -= 8;
    dst += 8;
  }
  while (colCount != 0);
}


void ChromaInterpolationFilterOneDHorizontal_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst,
	EB_U32                fracPosx,
	EB_U32                fracPosy)
{
	EB_S32 rowCount, colCount;
	__m128i c0, c2; // coeffs
	__m128i a0, a1, a2, a3;
	__m128i b0, b1;
	__m128i sum;

	EB_BYTE ptr,qtr;

  (void)firstPassIFDst;
  (void)fracPosy;


  refPic--;

  PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

  c0 = _mm_loadl_epi64((__m128i *)EbHevcChromaFilterCoeff[fracPosx]);
  c0 = _mm_packs_epi16(c0, c0);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c2 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 2)
  {
    rowCount = puHeight;
    ptr = refPic;
    qtr = dst;

    do
    {
      // Need 5 samples, load 8
      a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;

      a0 = _mm_unpacklo_epi8(a0, a1); // 10 samples
      a2 = _mm_unpacklo_epi8(a2, a3); // 10 samples

      b0 = _mm_unpacklo_epi16(a0, a2);

      sum = _mm_set1_epi16(32);
      sum = _mm_add_epi16(sum ,_mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 4, 4, 8, 1, 5, 5, 9, 2, 6, 6, 10, 3, 7, 7, 11)), c0));
      b0 = _mm_unpacklo_epi16(_mm_srli_si128(a0, 4), _mm_srli_si128(a2, 4));
      sum = _mm_add_epi16(sum ,_mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 4, 4, 8, 1, 5, 5, 9, 2, 6, 6, 10, 3, 7, 7, 11)), c2));
      sum = _mm_srai_epi16(sum , 6);
      sum = _mm_packus_epi16(sum, sum);

      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum, 0)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum, 1)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum, 2)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum, 3)); qtr += dstStride;

      rowCount -= 4;
    }
    while (rowCount > 0);

    puWidth -= 2;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 2;
    dst += 2;
  }


  if (puWidth & 4)
  {
    rowCount = puHeight;
    ptr = refPic;
    qtr = dst;
    do
    {
      // Need 7 samples, load 8
      a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;

      sum = _mm_set1_epi16(32);
      a0 = _mm_unpacklo_epi64(a0, a1);
      b0 = _mm_shuffle_epi8(a0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12));
      b1 = _mm_shuffle_epi8(a0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(b0, c0));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(b1, c2));

      sum = _mm_srai_epi16(sum , 6);
      sum = _mm_packus_epi16(sum, sum);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum, 1); qtr += dstStride;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    rowCount = puHeight;
    ptr = refPic;
    qtr = dst;
    do
    {
      // Need 11 samples, load 16
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      sum = _mm_set1_epi16(32);
      a2 = _mm_shuffle_epi8(a0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10));
      a0 = _mm_shuffle_epi8(a0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(a0, c0));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(a2, c2));
      sum = _mm_srai_epi16(sum , 6);
      sum = _mm_packus_epi16(sum, sum);

      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride;
    }
    while (--rowCount > 0);

    refPic += 8;
    dst += 8;
    colCount -= 8;
  }
  while (colCount > 0);
}


void ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16               *dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst,
	EB_U32                fracPosx,
	EB_U32                fracPosy)
{
  EB_S32 rowCount, colCount;
  __m128i c0, c2; // coeffs
  __m128i a0, a1, a2, a3;
  __m128i b0, b1;
  __m128i sum;

  EB_BYTE ptr;
  (void)firstPassIFDst;
  (void)fracPosy;

  refPic--;
  PrefetchBlock(refPic, srcStride, puWidth+8, puHeight);

  c0 = _mm_loadl_epi64((__m128i *)EbHevcChromaFilterCoeff[fracPosx]);
  c0 = _mm_packs_epi16(c0, c0);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c2 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 2)
  {
    rowCount = puHeight;
    ptr = refPic;
    do
    {
      // Need 5 samples, load 8
      a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;

      a0 = _mm_unpacklo_epi8(a0, a1); // 10 samples
      a2 = _mm_unpacklo_epi8(a2, a3); // 10 samples
      b0 = _mm_unpacklo_epi16(a0, a2);

      sum = _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 4, 4, 8, 1, 5, 5, 9, 2, 6, 6, 10, 3, 7, 7, 11)), c0);
      b0 = _mm_unpacklo_epi16(_mm_srli_si128(a0, 4), _mm_srli_si128(a2, 4));
      sum = _mm_add_epi16(sum ,_mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 4, 4, 8, 1, 5, 5, 9, 2, 6, 6, 10, 3, 7, 7, 11)), c2));

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      rowCount -= 4;
    }
    while (rowCount > 0);

    puWidth -= 2;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 2;
  }

  if (puWidth & 4)
  {
    rowCount = puHeight;
    ptr = refPic;
    do
    {

      // Need 7 samples, load 8
      a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;

      a0 = _mm_unpacklo_epi64(a0, a1);
      b0 = _mm_shuffle_epi8(a0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12));
      b1 = _mm_shuffle_epi8(a0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14));
      sum = _mm_maddubs_epi16(b0, c0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(b1, c2));

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
  }


  colCount = puWidth;
  do
  {
    ptr = refPic;
    rowCount = puHeight;
    do
    {

      // Need 11 samples, load 16
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
      a2 = _mm_shuffle_epi8(a0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10));
      a0 = _mm_shuffle_epi8(a0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8));
      sum = _mm_maddubs_epi16(a0, c0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(a2, c2));
      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;
    }
    while (--rowCount > 0);

    colCount -= 8;
    refPic += 8;
  }
  while (colCount > 0);
}



void ChromaInterpolationFilterOneDVertical_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst,
	EB_U32                fracPosx,
	EB_U32                fracPosy)
{
	EB_S32 rowCount, colCount;
	__m128i c0, c2; // coeffs
	__m128i a0, a1, a2, a3;
	__m128i b0, b1, b2, b3;
	__m128i sum;

	EB_BYTE ptr,qtr;

  (void)firstPassIFDst;
  (void)fracPosx;

  refPic -= srcStride;
  PrefetchBlock(refPic, srcStride, puWidth, puHeight+3);


  c0 = _mm_loadl_epi64((__m128i *)EbHevcChromaFilterCoeff[fracPosy]);
  c0 = _mm_packs_epi16(c0, c0);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c2 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 2)
  {
    rowCount = puHeight;
    ptr = refPic;
    qtr = dst;

    a0 = _mm_setzero_si128();
    a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 0); ptr += srcStride;
    a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 1); ptr += srcStride;
    a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 2); ptr += srcStride;

    do
    {
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 3); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 4); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 5); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 6); ptr += srcStride;

      sum = _mm_set1_epi16(32);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9)), c0));
      a0 = _mm_srli_si128(a0, 4);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9)), c2));
      a0 = _mm_srli_si128(a0, 4);
      sum = _mm_srai_epi16(sum , 6);
      sum = _mm_packus_epi16(sum, sum);

      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum, 0)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum, 1)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum, 2)); qtr += dstStride;
      *(EB_U16 *)qtr = (EB_U16)(_mm_extract_epi16(sum, 3)); qtr += dstStride;

      rowCount -= 4;
    }
    while (rowCount > 0);

    puWidth -= 2;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 2;
    dst += 2;
  }

  if (puWidth & 4)
  {
    rowCount = puHeight;
    ptr = refPic;
    qtr = dst;
    b0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    b1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    b2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a0 = _mm_unpacklo_epi32(b2, b1);
    a0 = _mm_unpacklo_epi64(a0, b0);

    do
    {
      sum = _mm_set1_epi16(32);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(8, 4, 9, 5, 10, 6, 11, 7, 4, 0, 5, 1, 6, 2, 7, 3)), c0));
      b3 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b3 = _mm_unpacklo_epi32(b0, b3);
      a0 = _mm_unpacklo_epi64(b3, a0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(8, 4, 9, 5, 10, 6, 11, 7, 4, 0, 5, 1, 6, 2, 7, 3)), c2));
      sum = _mm_srai_epi16(sum , 6);
      sum = _mm_packus_epi16(sum, sum);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum, 1); qtr += dstStride;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    rowCount = puHeight;
    ptr = refPic;
    qtr = dst;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;

    do
    {

      sum = _mm_set1_epi16(32);
      a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_unpacklo_epi8(a0, a1), c0));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_unpacklo_epi8(a2, a3), c2));

      a0 = a1;
      a1 = a2;
      a2 = a3;

      sum = _mm_srai_epi16(sum , 6);
      sum = _mm_packus_epi16(sum, sum);

      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride;

    }
    while (--rowCount > 0);

    refPic += 8;
    dst += 8;
    colCount -= 8;
  }
  while (colCount > 0);
}


void ChromaInterpolationFilterOneDOutRawVertical_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16               *dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst,
	EB_U32                fracPosx,
	EB_U32                fracPosy)
{
	EB_S32 rowCount, colCount;
	__m128i c0, c2; // coeffs
	__m128i a0, a1, a2, a3;
	__m128i b0, b1, b2, b3;
	__m128i sum;

	EB_BYTE ptr;

  (void)firstPassIFDst;
  (void)fracPosx;

  refPic -= srcStride;
  PrefetchBlock(refPic, srcStride, puWidth, puHeight+3);

  c0 = _mm_loadl_epi64((__m128i *)EbHevcChromaFilterCoeff[fracPosy]);
  c0 = _mm_packs_epi16(c0, c0);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c2 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 2)
  {
    rowCount = puHeight;
    ptr = refPic;
    a0 = _mm_setzero_si128();
    a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 0); ptr += srcStride;
    a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 1); ptr += srcStride;
    a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 2); ptr += srcStride;

    do
    {
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 3); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 4); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 5); ptr += srcStride;
      a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptr, 6); ptr += srcStride;

      sum = _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9)), c0);
      a0 = _mm_srli_si128(a0, 4);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9)), c2));
      a0 = _mm_srli_si128(a0, 4);

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      rowCount -= 4;
    }
    while (rowCount > 0);

    puWidth -= 2;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 2;
  }

  if (puWidth & 4)
  {
    rowCount = puHeight;
    ptr = refPic;
    b0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    b1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    b2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a0 = _mm_unpacklo_epi32(b2, b1);
    a0 = _mm_unpacklo_epi64(a0, b0);

    do
    {
      sum = _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(8, 4, 9, 5, 10, 6, 11, 7, 4, 0, 5, 1, 6, 2, 7, 3)), c0);
      b3 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b3 = _mm_unpacklo_epi32(b0, b3);
      a0 = _mm_unpacklo_epi64(b3, a0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(8, 4, 9, 5, 10, 6, 11, 7, 4, 0, 5, 1, 6, 2, 7, 3)), c2));

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
  }

  colCount = puWidth;
  do
  {
    rowCount = puHeight;
    ptr = refPic;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;

    do
    {

	  a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      sum = _mm_maddubs_epi16(_mm_unpacklo_epi8(a0, a1), c0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_unpacklo_epi8(a2, a3), c2));
      a0 = a1;
      a1 = a2;
      a2 = a3;

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;
    }
    while (--rowCount > 0);

    refPic += 8;
    colCount -= 8;
  }
  while (colCount > 0);
}



void ChromaInterpolationFilterTwoD_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst,
	EB_U32                fracPosx,
	EB_U32                fracPosy)
{
  ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3(refPic-srcStride, srcStride, firstPassIFDst, puWidth, puHeight+3, NULL, fracPosx, 0);
  ChromaInterpolationFilterTwoDInRaw_SSSE3(firstPassIFDst, dst, dstStride, puWidth, puHeight, fracPosy);
}

void ChromaInterpolationFilterTwoDInRawOutRaw_SSSE3(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy)
{
  EB_S32 rowCount, colCount;
  __m128i c0, c1; // coeffs

  c0 = _mm_loadl_epi64((__m128i *)EbHevcChromaFilterCoeff[fracPosy]);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0);

  if (puWidth & 2)
  {
    rowCount = puHeight;
    do
    {
      __m128i a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0));
      __m128i a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2));
      __m128i a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      __m128i a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6));
      __m128i b0, b1, b2, b3;
      __m128i sum0, sum1;

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpacklo_epi16(a2, a3);
      b2 = _mm_unpackhi_epi16(a0, a1);
      b3 = _mm_unpackhi_epi16(a2, a3);
      sum0 = _mm_madd_epi16(b0, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1, c1));
      sum1 = _mm_madd_epi16(b2, c0);
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);
      _mm_storeu_si128((__m128i *)dst, sum0);

      firstPassIFDst += 8;
      dst += 8;
      rowCount -= 4;
    }
    while (rowCount > 0);

    puWidth -= 2;
    if (puWidth == 0)
    {
      return;
    }
    firstPassIFDst += 8;
  }

  if (puWidth & 4)
  {
    rowCount = puHeight;
    do
    {
      __m128i a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0));
      __m128i a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4));
      __m128i a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+8));
      __m128i a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+12));
      __m128i b0, b1, b2, b3;
      __m128i sum0, sum1;

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpacklo_epi16(a2, a3);
      b2 = _mm_unpackhi_epi16(a0, a1);
      b3 = _mm_unpackhi_epi16(a2, a3);
      sum0 = _mm_madd_epi16(b0, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1, c1));
      sum1 = _mm_madd_epi16(b2, c0);
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);
      _mm_storeu_si128((__m128i *)dst, sum0);

      firstPassIFDst += 8;
      dst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += 16;
  }

  colCount = puWidth;
  do
  {
    __m128i a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
    __m128i a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
    __m128i a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
    rowCount = puHeight;

    do
    {
      __m128i a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      __m128i b0, b1, b2, b3;
      __m128i sum0, sum1;
      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpacklo_epi16(a2, a3);
      b2 = _mm_unpackhi_epi16(a0, a1);
      b3 = _mm_unpackhi_epi16(a2, a3);
      sum0 = _mm_madd_epi16(b0, c0);
      sum0 = _mm_add_epi32(sum0, _mm_madd_epi16(b1, c1));
      sum1 = _mm_madd_epi16(b2, c0);
      sum1 = _mm_add_epi32(sum1, _mm_madd_epi16(b3, c1));
      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);
      _mm_storeu_si128((__m128i *)dst, sum0);

      a0 = a1;
      a1 = a2;
      a2 = a3;
      firstPassIFDst += 8;
      dst += 8;
    }
    while (--rowCount > 0);

    firstPassIFDst += 24;
    colCount -= 8;
  }
  while (colCount > 0);
}


void ChromaInterpolationFilterTwoDOutRaw_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16               *dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst,
	EB_U32                fracPosx,
	EB_U32                fracPosy)
{
  ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3(refPic-srcStride, srcStride, firstPassIFDst, puWidth, puHeight+3, NULL, fracPosx, 0);
  ChromaInterpolationFilterTwoDInRawOutRaw_SSSE3(firstPassIFDst, dst, puWidth, puHeight, fracPosy);
}



void LumaInterpolationFilterOneDHorizontal_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_U32                fracPosx)
{
	EB_S32 rowCount, colCount;
	__m128i c0, c1, c2, c3; // coeffs
	__m128i a0, a1;
	__m128i b0;
	__m128i sum;


  refPic -= 3;

  PrefetchBlock(refPic, srcStride, (puWidth == 4) ? 16 : puWidth+8, (puWidth == 4) ? ((puHeight+1)&~1) : puHeight);

  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff[fracPosx]);
  c0 = _mm_packs_epi16(c0, c0);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    EB_BYTE ptr = refPic;
    EB_BYTE qtr = dst;
    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;


      sum = _mm_set1_epi16(32);

      b0 = _mm_unpacklo_epi64(a0, a1);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12)), c0));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14)), c1));
      b0 = _mm_unpacklo_epi64(_mm_srli_si128(a0, 4), _mm_srli_si128(a1, 4));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12)), c2));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14)), c3));
      sum = _mm_srai_epi16(sum, 6);
      sum = _mm_packus_epi16(sum, sum);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum, 1); qtr += dstStride;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    EB_BYTE ptr = refPic;
    EB_BYTE qtr = dst;

    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      sum = _mm_set1_epi16(32);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8)), c0));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10)), c1));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12)), c2));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14)), c3));
      sum = _mm_srai_epi16(sum, 6);
      sum = _mm_packus_epi16(sum, sum);

      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride;
    }
    while (--rowCount > 0);

    refPic += 8;
    dst += 8;

    colCount -= 8;
  }
  while (colCount > 0);
}

void LumaInterpolationFilterOneDOutRawHorizontal_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16               *dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_U32                fracPosx)
{
	EB_S32 rowCount, colCount;
	__m128i c0, c1, c2, c3; // coeffs
	__m128i a0, a1;
	__m128i b0;
	__m128i sum;
	EB_BYTE ptr;  

  refPic -= 3;

  PrefetchBlock(refPic, srcStride, (puWidth == 4) ? 16 : puWidth+8, (puWidth == 4) ? ((puHeight+1)&~1) : puHeight);


  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff[fracPosx]);
  c0 = _mm_packs_epi16(c0, c0);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    ptr = refPic;
    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;


      b0 = _mm_unpacklo_epi64(a0, a1);
      sum = _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12)), c0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14)), c1));
      b0 = _mm_unpacklo_epi64(_mm_srli_si128(a0, 4), _mm_srli_si128(a1, 4));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12)), c2));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14)), c3));

      sum = _mm_sub_epi16(sum, _mm_set1_epi16(128*64));

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
  }
  colCount = puWidth;
  do
  {
    ptr = refPic;
    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      sum = _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8)), c0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10)), c1));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12)), c2));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14)), c3));

      sum = _mm_sub_epi16(sum, _mm_set1_epi16(128*64));

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;
    }
    while (--rowCount > 0);

    refPic += 8;
    colCount -= 8;
  }
  while (colCount > 0); 
}

void LumaInterpolationFilterOneDOutRawHorizontalOut_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16               *dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_U32                fracPosx)
{
	EB_S32 rowCount, colCount;
	__m128i c0, c1, c2, c3; // coeffs
	__m128i a0, a1;
	__m128i b0;
	__m128i sum;
	EB_BYTE ptr;
	EB_S16               *qtr;
  refPic -= 3;

  PrefetchBlock(refPic, srcStride, (puWidth == 4) ? 16 : puWidth+8, (puWidth == 4) ? ((puHeight+1)&~1) : puHeight);


  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff[fracPosx]);
  c0 = _mm_packs_epi16(c0, c0);
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    ptr = refPic;
    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;
      a1 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;


      b0 = _mm_unpacklo_epi64(a0, a1);
      sum = _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12)), c0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14)), c1));
      b0 = _mm_unpacklo_epi64(_mm_srli_si128(a0, 4), _mm_srli_si128(a1, 4));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 8, 9, 9, 10, 10, 11, 11, 12)), c2));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 10, 11, 11, 12, 12, 13, 13, 14)), c3));

      sum = _mm_sub_epi16(sum, _mm_set1_epi16(128*64));

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
  }

  colCount = puWidth;
  do
  {
    ptr = refPic;
	qtr = dst;
    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)ptr); ptr += srcStride;

      sum = _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8)), c0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10)), c1));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12)), c2));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(a0, _mm_setr_epi8(6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14)), c3));

      sum = _mm_sub_epi16(sum, _mm_set1_epi16(128*64));

      _mm_storeu_si128((__m128i *)qtr, sum);qtr += puWidth;
    }
    while (--rowCount > 0);

    refPic += 8;
    dst += 8;

    colCount -= 8;
  }
  while (colCount > 0);
}

void LumaInterpolationFilterOneDVertical_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_U32                fracPosx)
{
	EB_S32 rowCount, colCount;
	__m128i c0, c1, c2, c3; // coeffs
	__m128i a0, a1, a2, a3, a4, a5, a6, a7, a8;
	__m128i b0, b1, b2, b3, b4, b5, b6, b7;
	__m128i sum;

  refPic -= 3*srcStride;

  PrefetchBlock(refPic, srcStride, puWidth, puHeight+7);

  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff[fracPosx]);
  c0 = _mm_packs_epi16(c0, c0); // Convert 16-bit coefficients to 8 bits
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    EB_BYTE ptr = refPic;
    EB_BYTE qtr = dst;

    rowCount = puHeight;
    a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a3 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a4 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a5 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a6 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;

    b0 = _mm_unpacklo_epi32(_mm_setzero_si128(), a0);
    b1 = _mm_unpacklo_epi32(a1, a2);
    b2 = _mm_unpacklo_epi32(a3, a4);
    b3 = _mm_unpacklo_epi32(a5, a6);
    b0 = _mm_unpacklo_epi64(b0, b1);
    b1 = _mm_unpacklo_epi64(b2, b3);

    do
    {
      sum = _mm_set1_epi16(32);

      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(4, 8, 5, 9, 6, 10, 7, 11, 8, 12, 9, 13, 10, 14, 11, 15)), c0));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b1, _mm_setr_epi8(4, 8, 5, 9, 6, 10, 7, 11, 8, 12, 9, 13, 10, 14, 11, 15)), c2));

      a7 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a8 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b2 = _mm_unpacklo_epi32(a7, a8);
      b0 = _mm_alignr_epi8(b1, b0, 8);
      b1 = _mm_alignr_epi8(b2, b1, 8);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(4, 8, 5, 9, 6, 10, 7, 11, 8, 12, 9, 13, 10, 14, 11, 15)), c1));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b1, _mm_setr_epi8(4, 8, 5, 9, 6, 10, 7, 11, 8, 12, 9, 13, 10, 14, 11, 15)), c3));
      sum = _mm_srai_epi16(sum, 6);
      sum = _mm_packus_epi16(sum, sum);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum, 1); qtr += dstStride;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    EB_BYTE ptr = refPic;
    EB_BYTE qtr = dst;

    rowCount = puHeight;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi8(a0, a1);
    b1 = _mm_unpacklo_epi8(a1, a2);
    b2 = _mm_unpacklo_epi8(a2, a3);
    b3 = _mm_unpacklo_epi8(a3, a4);
    b4 = _mm_unpacklo_epi8(a4, a5);
    b5 = _mm_unpacklo_epi8(a5, a6);
    do
    {
      sum = _mm_set1_epi16(32);
      a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a8 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b6 = _mm_unpacklo_epi8(a6, a7);
      b7 = _mm_unpacklo_epi8(a7, a8);

      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b0, c0));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b2, c1));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b4, c2));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b6, c3));

      sum = _mm_srai_epi16(sum, 6);
      sum = _mm_packus_epi16(sum, sum);

      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride;

      sum = _mm_set1_epi16(32);
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b1, c0));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b3, c1));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b5, c2));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b7, c3));
      sum = _mm_srai_epi16(sum, 6);
      sum = _mm_packus_epi16(sum, sum);

      _mm_storel_epi64((__m128i *)qtr, sum); qtr += dstStride;

      b0 = b2;
      b1 = b3;
      b2 = b4;
      b3 = b5;
      b4 = b6;
      b5 = b7;
      a6 = a8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    refPic += 8;
    dst += 8;

    colCount -= 8;
  }
  while (colCount > 0);
}

void LumaInterpolationFilterOneDOutRawVertical_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16*               dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_U32                fracPosx)
{
	EB_S32 rowCount, colCount;
	__m128i c0, c1, c2, c3; // coeffs
	__m128i a0, a1, a2, a3, a4, a5, a6, a7, a8;
	__m128i b0, b1, b2, b3, b4, b5, b6, b7;
	__m128i sum;
	EB_BYTE ptr;

  refPic -= 3*srcStride;

  PrefetchBlock(refPic, srcStride, puWidth, puHeight+7);

  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff[fracPosx]);
  c0 = _mm_packs_epi16(c0, c0); // Convert 16-bit coefficients to 8 bits
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    rowCount = puHeight;
    ptr = refPic;
    a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a3 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a4 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a5 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
    a6 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;

    b0 = _mm_unpacklo_epi32(_mm_setzero_si128(), a0);
    b1 = _mm_unpacklo_epi32(a1, a2);
    b2 = _mm_unpacklo_epi32(a3, a4);
    b3 = _mm_unpacklo_epi32(a5, a6);
    b0 = _mm_unpacklo_epi64(b0, b1);
    b1 = _mm_unpacklo_epi64(b2, b3);

    do
    {

      sum = _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(4, 8, 5, 9, 6, 10, 7, 11, 8, 12, 9, 13, 10, 14, 11, 15)), c0);
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b1, _mm_setr_epi8(4, 8, 5, 9, 6, 10, 7, 11, 8, 12, 9, 13, 10, 14, 11, 15)), c2));

      a7 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      a8 = _mm_cvtsi32_si128(*(EB_U32 *)ptr); ptr += srcStride;
      b2 = _mm_unpacklo_epi32(a7, a8);
      b0 = _mm_alignr_epi8(b1, b0, 8);
      b1 = _mm_alignr_epi8(b2, b1, 8);

      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b0, _mm_setr_epi8(4, 8, 5, 9, 6, 10, 7, 11, 8, 12, 9, 13, 10, 14, 11, 15)), c1));
      sum = _mm_add_epi16(sum, _mm_maddubs_epi16(_mm_shuffle_epi8(b1, _mm_setr_epi8(4, 8, 5, 9, 6, 10, 7, 11, 8, 12, 9, 13, 10, 14, 11, 15)), c3));
      sum = _mm_sub_epi16(sum, _mm_set1_epi16(128*64));

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    refPic += 4;
  }

  colCount = puWidth;
  do
  {
    ptr = refPic;
    rowCount = puHeight;
    a0 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a1 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a2 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a3 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a4 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a5 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    a6 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
    b0 = _mm_unpacklo_epi8(a0, a1);
    b1 = _mm_unpacklo_epi8(a1, a2);
    b2 = _mm_unpacklo_epi8(a2, a3);
    b3 = _mm_unpacklo_epi8(a3, a4);
    b4 = _mm_unpacklo_epi8(a4, a5);
    b5 = _mm_unpacklo_epi8(a5, a6);
    do
    {
      a7 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      a8 = _mm_loadl_epi64((__m128i *)ptr); ptr += srcStride;
      b6 = _mm_unpacklo_epi8(a6, a7);
      b7 = _mm_unpacklo_epi8(a7, a8);

      sum = _mm_maddubs_epi16(b0, c0);
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b2, c1));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b4, c2));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b6, c3));

      sum = _mm_sub_epi16(sum, _mm_set1_epi16(128*64));
      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      sum = _mm_maddubs_epi16(b1, c0);
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b3, c1));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b5, c2));
      sum = _mm_add_epi16(sum,_mm_maddubs_epi16(b7, c3));

      sum = _mm_sub_epi16(sum, _mm_set1_epi16(128*64));

      _mm_storeu_si128((__m128i *)dst, sum);
      dst += 8;

      b0 = b2;
      b1 = b3;
      b2 = b4;
      b3 = b5;
      b4 = b6;
      b5 = b7;
      a6 = a8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    refPic += 8;
    colCount -= 8;
  }
  while (colCount > 0);
}


void LumaInterpolationFilterPosa_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDHorizontal_SSSE3(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosb_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDHorizontal_SSSE3(refPic, srcStride, dst, dstStride, puWidth, puHeight, 2);
}

void LumaInterpolationFilterPosc_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDHorizontal_SSSE3(refPic, srcStride, dst, dstStride, puWidth, puHeight, 3);
}

void LumaInterpolationFilterPosd_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDVertical_SSSE3(refPic, srcStride, dst, dstStride, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosh_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDVertical_SSSE3(refPic, srcStride, dst, dstStride, puWidth, puHeight, 2);
}

void LumaInterpolationFilterPosn_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDVertical_SSSE3(refPic, srcStride, dst, dstStride, puWidth, puHeight, 3);
}

void LumaInterpolationFilterPose_SSSE3(
                                        EB_BYTE               refPic,
                                        EB_U32                srcStride,
                                        EB_BYTE               dst,
                                        EB_U32                dstStride,
                                        EB_U32                puWidth,
                                        EB_U32                puHeight,
                                        EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, 1);
  EbHevcLumaInterpolationFilterTwoDInRaw7(firstPassIFDst, dst, dstStride, puWidth, puHeight, 1);
}


void LumaInterpolationFilterPosf_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  EB_U32 puHeight1 = puHeight + 6;
  EB_BYTE refPic1 = refPic - 3 * srcStride;
  LumaInterpolationFilterOneDOutRawHorizontal(refPic1, srcStride, firstPassIFDst, puWidth, puHeight1, 2);
  EbHevcLumaInterpolationFilterTwoDInRaw7(firstPassIFDst, dst, dstStride, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosg_SSSE3(
                                        EB_BYTE               refPic,
                                        EB_U32                srcStride,
                                        EB_BYTE               dst,
                                        EB_U32                dstStride,
                                        EB_U32                puWidth,
                                        EB_U32                puHeight,
                                        EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, 3);
  EbHevcLumaInterpolationFilterTwoDInRaw7(firstPassIFDst, dst, dstStride, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosi_SSSE3(
                                        EB_BYTE               refPic,
                                        EB_U32                srcStride,
                                        EB_BYTE               dst,
                                        EB_U32                dstStride,
                                        EB_U32                puWidth,
                                        EB_U32                puHeight,
                                        EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, 1);
  EbHevcLumaInterpolationFilterTwoDInRawM(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}



void LumaInterpolationFilterPosj_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, 2);
  EbHevcLumaInterpolationFilterTwoDInRawM(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosk_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, 3);
  EbHevcLumaInterpolationFilterTwoDInRawM(firstPassIFDst, dst, dstStride, puWidth, puHeight);
}

void LumaInterpolationFilterPosp_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, 1);
  EbHevcLumaInterpolationFilterTwoDInRaw7(firstPassIFDst, dst, dstStride, puWidth, puHeight, 3);
}

void LumaInterpolationFilterPosq_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  EB_U32 puHeight1 = puHeight + 6;
  EB_BYTE refPic1 = refPic - 2 * srcStride;
  LumaInterpolationFilterOneDOutRawHorizontal(refPic1, srcStride, firstPassIFDst, puWidth, puHeight1, 2);
  EbHevcLumaInterpolationFilterTwoDInRaw7(firstPassIFDst, dst, dstStride, puWidth, puHeight, 3);
}

void LumaInterpolationFilterPosr_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_BYTE               dst,
	EB_U32                dstStride,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, 3);
  EbHevcLumaInterpolationFilterTwoDInRaw7(firstPassIFDst, dst, dstStride, puWidth, puHeight, 3);
}


void LumaInterpolationCopyOutRaw_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16               *dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;
  EbHevcPictureCopyKernelOutRaw_SSSE3(refPic, srcStride, dst, puWidth, puHeight, 128*64);
}



void LumaInterpolationFilterPosaOutRaw_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16*               dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;
  LumaInterpolationFilterOneDOutRawHorizontal(refPic, srcStride, dst, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosbOutRaw_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16*               dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;
  //LumaInterpolationFilterOneDOutRawHorizontalOut_SSSE3(refPic, srcStride, dst, puWidth, puHeight, 2);
 LumaInterpolationFilterOneDOutRawHorizontal(refPic, srcStride, dst, puWidth, puHeight, 2);
}

void LumaInterpolationFilterPoscOutRaw_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16*               dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;
  //LumaInterpolationFilterOneDOutRawHorizontalOut_SSSE3(refPic, srcStride, dst, puWidth, puHeight, 3);
  LumaInterpolationFilterOneDOutRawHorizontal(refPic, srcStride, dst, puWidth, puHeight, 3);
}

void LumaInterpolationFilterPosdOutRaw_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16*               dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDOutRawVertical_SSSE3(refPic, srcStride, dst, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPoshOutRaw_SSSE3(
	EB_BYTE               refPic,
	EB_U32                srcStride,
	EB_S16*               dst,
	EB_U32                puWidth,
	EB_U32                puHeight,
	EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDOutRawVertical_SSSE3(refPic, srcStride, dst, puWidth, puHeight, 2);
}

void BiPredClipping_SSSE3(
                           EB_U32     puWidth,
                           EB_U32     puHeight,
                           EB_S16    *list0Src,
                           EB_S16    *list1Src,
                           EB_BYTE    dst,
                           EB_U32     dstStride,
                           EB_S32     offset)
{
  EB_U32 rowCount, colCount;

  if (puWidth & 2)
  {
    EB_BYTE q = dst;
    rowCount = puHeight;
    do
    {
      __m128i a0, a2;
      a0 = _mm_loadu_si128((__m128i *)list0Src);
      a2 = _mm_loadu_si128((__m128i *)list1Src);
      list0Src += 8;
      list1Src += 8;

      a0 = _mm_adds_epi16(a0, a2);
      a0 = _mm_adds_epi16(a0, _mm_set1_epi16((short)offset));
      a0 = _mm_srai_epi16(a0, 7);
      a0 = _mm_packus_epi16(a0, a0);

      *(EB_U16 *)q = (EB_U16)(_mm_extract_epi16(a0, 0)); q += dstStride;
      *(EB_U16 *)q = (EB_U16)(_mm_extract_epi16(a0, 1)); q += dstStride;
      *(EB_U16 *)q = (EB_U16)(_mm_extract_epi16(a0, 2)); q += dstStride;
      *(EB_U16 *)q = (EB_U16)(_mm_extract_epi16(a0, 3)); q += dstStride;
      rowCount -= 4;
    }
    while (rowCount != 0);

    puWidth -= 2;
    if (puWidth == 0)
    {
      return;
    }

    dst += 2;
  }

  if (puWidth & 4)
  {
    EB_BYTE q = dst;
    rowCount = puHeight;
    do
    {
      __m128i a0, a2;
      a0 = _mm_loadu_si128((__m128i *)list0Src);
      a2 = _mm_loadu_si128((__m128i *)list1Src);
      list0Src += 8;
      list1Src += 8;

      a0 = _mm_adds_epi16(a0, a2);
      a0 = _mm_adds_epi16(a0, _mm_set1_epi16((short)offset));
      a0 = _mm_srai_epi16(a0, 7);
      a0 = _mm_packus_epi16(a0, a0);
      *(EB_U32 *)q = _mm_extract_epi32(a0, 0); q += dstStride;
      *(EB_U32 *)q = _mm_extract_epi32(a0, 1); q += dstStride;
      rowCount -= 2;
    }
    while (rowCount != 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    dst += 4;
  }

  colCount = puWidth;
  do
  {
    EB_BYTE q = dst;
    rowCount = puHeight;
    do
    {
      __m128i a0, a1, a2, a3;
      a0 = _mm_loadu_si128((__m128i *)list0Src);
      a2 = _mm_loadu_si128((__m128i *)list1Src);
      list0Src += 8;
      list1Src += 8;
      a0 = _mm_adds_epi16(a0, a2);
      a0 = _mm_adds_epi16(a0, _mm_set1_epi16((short)offset));
      a0 = _mm_srai_epi16(a0, 7);

      a1 = _mm_loadu_si128((__m128i *)list0Src);
      a3 = _mm_loadu_si128((__m128i *)list1Src);
      list0Src += 8;
      list1Src += 8;
      a1 = _mm_adds_epi16(a1, a3);
      a1 = _mm_adds_epi16(a1, _mm_set1_epi16((short)offset));
      a1 = _mm_srai_epi16(a1, 7);

      a0 = _mm_packus_epi16(a0, a1);
      _mm_storel_epi64((__m128i *)q, a0);
      q += dstStride;
      _mm_storeh_epi64((__m128i *)q, a0);
      q += dstStride;

      rowCount -= 2;
    }
    while (rowCount != 0);

    colCount -= 8;
    dst += 8;
  }
  while (colCount != 0);
}


void BiPredClippingOnTheFly_SSSE3(
	EB_BYTE    list0Src,
	EB_U32     list0SrcStride,
	EB_BYTE    list1Src,
	EB_U32     list1SrcStride,
	EB_BYTE    dst,
	EB_U32     dstStride,
	EB_U32     puWidth,
	EB_U32     puHeight,
	EB_S32     offset,
	EB_BOOL	   isLuma)
{


	EB_U32 rowCount, colCount;
	__m128i o;
	o = (isLuma) ? _mm_set1_epi16(128 * 64) : _mm_set1_epi16(0);

	PrefetchBlock(list0Src, list0SrcStride, puWidth, puHeight);
	PrefetchBlock(list1Src, list1SrcStride, puWidth, puHeight);

	if (puWidth & 2)
	{

		EB_BYTE q = dst;
		EB_BYTE ptrSrc0 = list0Src;
		EB_BYTE ptrSrc1 = list1Src;
		rowCount = puHeight;
		__m128i a0, a2;
		a0 = _mm_setzero_si128();
		a2 = _mm_setzero_si128();
		do
		{
			a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptrSrc0, 0); ptrSrc0 += list0SrcStride;
			a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptrSrc0, 1); ptrSrc0 += list0SrcStride;
			a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptrSrc0, 2); ptrSrc0 += list0SrcStride;
			a0 = _mm_insert_epi16(a0, *(EB_U16 *)ptrSrc0, 3); ptrSrc0 += list0SrcStride;
			a0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
			a0 = _mm_slli_epi16(a0, 6);
			a0 = _mm_sub_epi16(a0, o);


			a2 = _mm_insert_epi16(a2, *(EB_U16 *)ptrSrc1, 0); ptrSrc1 += list1SrcStride;
			a2 = _mm_insert_epi16(a2, *(EB_U16 *)ptrSrc1, 1); ptrSrc1 += list1SrcStride;
			a2 = _mm_insert_epi16(a2, *(EB_U16 *)ptrSrc1, 2); ptrSrc1 += list1SrcStride;
			a2 = _mm_insert_epi16(a2, *(EB_U16 *)ptrSrc1, 3); ptrSrc1 += list1SrcStride;
			a2 = _mm_unpacklo_epi8(a2, _mm_setzero_si128());
			a2 = _mm_slli_epi16(a2, 6);
			a2 = _mm_sub_epi16(a2, o);

			a0 = _mm_adds_epi16(a0, a2);
			a0 = _mm_adds_epi16(a0, _mm_set1_epi16((short)offset));
			a0 = _mm_srai_epi16(a0, 7);
			a0 = _mm_packus_epi16(a0, a0);

			*(EB_U16 *)q = (EB_U16)(_mm_extract_epi16(a0, 0)); q += dstStride;
			*(EB_U16 *)q = (EB_U16)(_mm_extract_epi16(a0, 1)); q += dstStride;
			*(EB_U16 *)q = (EB_U16)(_mm_extract_epi16(a0, 2)); q += dstStride;
			*(EB_U16 *)q = (EB_U16)(_mm_extract_epi16(a0, 3)); q += dstStride;
			rowCount -= 4;
		} while (rowCount != 0);

		puWidth -= 2;
		if (puWidth == 0)
		{
			return;
		}

		list0Src += 2;
		list1Src += 2;
		dst += 2;
	}

	if (puWidth & 4)
	{

		EB_BYTE q = dst;
		EB_BYTE ptrSrc0 = list0Src;
		EB_BYTE ptrSrc1 = list1Src;
		rowCount = puHeight;
		do
		{
			__m128i a0, a1, a2;
			a0 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc0); ptrSrc0 += list0SrcStride;
			a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc0); ptrSrc0 += list0SrcStride;
			a0 = _mm_unpacklo_epi32(a0, a1);
			a0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
			a0 = _mm_slli_epi16(a0, 6);
			a0 = _mm_sub_epi16(a0, o);

			a2 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc1); ptrSrc1 += list1SrcStride;
			a1 = _mm_cvtsi32_si128(*(EB_U32 *)ptrSrc1); ptrSrc1 += list1SrcStride;
			a2 = _mm_unpacklo_epi32(a2, a1);
			a2 = _mm_unpacklo_epi8(a2, _mm_setzero_si128());
			a2 = _mm_slli_epi16(a2, 6);
			a2 = _mm_sub_epi16(a2, o);

			a0 = _mm_adds_epi16(a0, a2);
			a0 = _mm_adds_epi16(a0, _mm_set1_epi16((short)offset));
			a0 = _mm_srai_epi16(a0, 7);
			a0 = _mm_packus_epi16(a0, a0);
			*(EB_U32 *)q = _mm_extract_epi32(a0, 0); q += dstStride;
			*(EB_U32 *)q = _mm_extract_epi32(a0, 1); q += dstStride;
			rowCount -= 2;
		} while (rowCount != 0);

		puWidth -= 4;
		if (puWidth == 0)
		{
			return;
		}

		list0Src += 4;
		list1Src += 4;
		dst += 4;
	}


	colCount = puWidth;
	do
	{
		EB_BYTE q = dst;
		EB_BYTE ptrSrc0 = list0Src;
		EB_BYTE ptrSrc1 = list1Src;
		rowCount = puHeight;
		do
		{
			__m128i a0, a1, a2, a3;
			a0 = _mm_loadl_epi64((__m128i *)ptrSrc0);
			ptrSrc0 += list0SrcStride;
			a0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
			a0 = _mm_slli_epi16(a0, 6);
			a0 = _mm_sub_epi16(a0, o);

			a2 = _mm_loadl_epi64((__m128i *)ptrSrc1);
			ptrSrc1 += list1SrcStride;
			a2 = _mm_unpacklo_epi8(a2, _mm_setzero_si128());
			a2 = _mm_slli_epi16(a2, 6);
			a2 = _mm_sub_epi16(a2, o);

			a0 = _mm_adds_epi16(a0, a2);
			a0 = _mm_adds_epi16(a0, _mm_set1_epi16((short)offset));
			a0 = _mm_srai_epi16(a0, 7);


			a1 = _mm_loadl_epi64((__m128i *)ptrSrc0);
			ptrSrc0 += list0SrcStride;
			a1 = _mm_unpacklo_epi8(a1, _mm_setzero_si128());
			a1 = _mm_slli_epi16(a1, 6);
			a1 = _mm_sub_epi16(a1, o);
			a3 = _mm_loadl_epi64((__m128i *)ptrSrc1);
			ptrSrc1 += list1SrcStride;
			a3 = _mm_unpacklo_epi8(a3, _mm_setzero_si128());
			a3 = _mm_slli_epi16(a3, 6);
			a3 = _mm_sub_epi16(a3, o);
			a1 = _mm_adds_epi16(a1, a3);
			a1 = _mm_adds_epi16(a1, _mm_set1_epi16((short)offset));
			a1 = _mm_srai_epi16(a1, 7);

			a0 = _mm_packus_epi16(a0, a1);
			_mm_storel_epi64((__m128i *)q, a0);
			q += dstStride;
			_mm_storeh_epi64((__m128i *)q, a0);
			q += dstStride;

			rowCount -= 2;
		} while (rowCount != 0);

		colCount -= 8;
		list0Src += 8;
		list1Src += 8;
		dst += 8;
	} while (colCount != 0);
}
//Vnni code
#ifdef VNNI_SUPPORT
void EbHevcLumaInterpolationFilterTwoDInRawOutRaw7_VNNI(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy)
{
  EB_S32 rowCount, colCount;
  __m128i a0, a1, a2, a3, a4, a5, a6;
  __m128i c0, c1, c2;
  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff7[fracPosy]);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    rowCount = puHeight;

    do
    {
      __m128i sum0, sum1;
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*4));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*4));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*4));
      a0 = _mm_sub_epi16(a0, a6);

      sum0 = _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0);

      sum0 = _mm_dpwssd_epi32(sum0, _mm_unpacklo_epi16(a2, a3), c1);
      sum1 = _mm_dpwssd_epi32(sum1, _mm_unpackhi_epi16(a2, a3), c1);
      sum0 = _mm_dpwssd_epi32(sum0, _mm_unpacklo_epi16(a4, a5), c2);
      sum1 = _mm_dpwssd_epi32(sum1, _mm_unpackhi_epi16(a4, a5), c2);

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;

      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += (fracPosy == 2) ? 32 : 24;
  }

  colCount = puWidth;
  do
  {
    rowCount = puHeight;
    do
    {
      __m128i b0l, b0h, b1l, b1h, b2l, b2h;
      __m128i sum0, sum1;

      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*8));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*8));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*8));
      a0 = _mm_sub_epi16(a0, a6);

      b0l = _mm_unpacklo_epi16(a0, a1);
      b0h = _mm_unpackhi_epi16(a0, a1);
      b1l = _mm_unpacklo_epi16(a2, a3);
      b1h = _mm_unpackhi_epi16(a2, a3);
      b2l = _mm_unpacklo_epi16(a4, a5);
      b2h = _mm_unpackhi_epi16(a4, a5);

      sum0 = _mm_madd_epi16(b0l, c0);
      sum1 = _mm_madd_epi16(b0h, c0);

      sum0 = _mm_dpwssd_epi32(sum0, b1l, c1);
      sum1 = _mm_dpwssd_epi32(sum1, b1h, c1);
      sum0 = _mm_dpwssd_epi32(sum0, b2l, c2);
      sum1 = _mm_dpwssd_epi32(sum1, b2h, c2);
      
      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;

      firstPassIFDst += 8;
      rowCount--;
    }
    while (rowCount > 0);

    firstPassIFDst += (fracPosy == 2) ? 56 : 48;
    colCount -= 8;
  }
  while (colCount > 0);

}

void EbHevcLumaInterpolationFilterTwoDInRawM_VNNI(EB_S16 *firstPassIFDst, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight)
{
  EB_S32 rowCount, colCount;

  __m128i c0, c1;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i sum0, sum1;

  EB_BYTE qtr;

  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff7[2]);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);



  if (puWidth & 4)
  {
    rowCount = puHeight;
    qtr = dst;

    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*4));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*4));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*4));
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+7*4));

      sum0 = _mm_set1_epi32(257<<11);
      sum1 = _mm_set1_epi32(257<<11);

      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);
      sum0 = _mm_dpwssd_epi32(sum0, _mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_dpwssd_epi32(sum1, _mm_unpackhi_epi16(a0, a1), c0);
      sum0 = _mm_dpwssd_epi32(sum0, _mm_unpacklo_epi16(a2, a3), c1);
      sum1 = _mm_dpwssd_epi32(sum1, _mm_unpackhi_epi16(a2, a3), c1);

      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 1); qtr += dstStride;
      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += 32;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    qtr = dst;

    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*8));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*8));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*8));
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+7*8));

      sum0 = _mm_set1_epi32(257<<11);
      sum1 = _mm_set1_epi32(257<<11);
      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);
      sum0 = _mm_dpwssd_epi32(sum0, _mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_dpwssd_epi32(sum1, _mm_unpackhi_epi16(a0, a1), c0);
      sum0 = _mm_dpwssd_epi32(sum0, _mm_unpacklo_epi16(a2, a3), c1);
      sum1 = _mm_dpwssd_epi32(sum1, _mm_unpackhi_epi16(a2, a3), c1);

      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      _mm_storel_epi64((__m128i *)qtr, sum0); qtr += dstStride;
      firstPassIFDst += 8;
    }
    while (--rowCount > 0);

    firstPassIFDst += 56;
    dst += 8;
    colCount -= 8;
  }
  while (colCount > 0);
}

void EbHevcLumaInterpolationFilterTwoDInRawOutRawM_VNNI(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight)
{
  EB_S32 rowCount, colCount;

  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i c0, c1;
  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff7[2]);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);

  if (puWidth & 4)
  {
    rowCount = puHeight;

    do
    {
      __m128i sum0, sum1;
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*4));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*4));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*4));
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+7*4));

      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);
      sum0 = _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0);
      sum0 = _mm_dpwssd_epi32(sum0, _mm_unpacklo_epi16(a2, a3), c1);
      sum1 = _mm_dpwssd_epi32(sum1, _mm_unpackhi_epi16(a2, a3), c1);

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += 32;
  }

  colCount = puWidth;
  do
  {
    rowCount = puHeight;
    do
    {
      __m128i sum0, sum1;
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*8));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*8));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*8));
      a7 = _mm_loadu_si128((__m128i *)(firstPassIFDst+7*8));

      a0 = _mm_add_epi16(a0, a7);
      a1 = _mm_add_epi16(a1, a6);
      a2 = _mm_add_epi16(a2, a5);
      a3 = _mm_add_epi16(a3, a4);
      sum0 = _mm_madd_epi16(_mm_unpacklo_epi16(a0, a1), c0);
      sum1 = _mm_madd_epi16(_mm_unpackhi_epi16(a0, a1), c0);
      sum0 = _mm_dpwssd_epi32(sum0, _mm_unpacklo_epi16(a2, a3), c1);
      sum1 = _mm_dpwssd_epi32(sum1, _mm_unpackhi_epi16(a2, a3), c1);

      sum0 = _mm_srai_epi32(sum0, 6);
      sum1 = _mm_srai_epi32(sum1, 6);
      sum0 = _mm_packs_epi32(sum0, sum1);

      _mm_storeu_si128((__m128i *)dst, sum0);
      dst += 8;
      firstPassIFDst += 8;
    }
    while (--rowCount > 0);

    firstPassIFDst += 56;
    colCount -= 8;
  }
  while (colCount > 0);
}

void EbHevcLumaInterpolationFilterTwoDInRaw7_VNNI(EB_S16 *firstPassIFDst, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy)
{
  EB_S32 rowCount, colCount;
  __m128i c0, c1, c2;
  __m128i a0, a1, a2, a3, a4, a5, a6;
  __m128i sum0 , sum1;
  __m128i b0l, b0h, b1l, b1h, b2l, b2h;
  EB_BYTE qtr;
  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff7[fracPosy]);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);


  if (puWidth & 4)
  {
    rowCount = puHeight;

    qtr = dst;

    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*4));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*4));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*4));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*4));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*4));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*4));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*4));
      a0 = _mm_sub_epi16(a0, a6);

      sum0 = _mm_set1_epi32(257<<11);
      sum1 = _mm_set1_epi32(257<<11);


      b0l = _mm_unpacklo_epi16(a0, a1);
      b0h = _mm_unpackhi_epi16(a0, a1);
      b1l = _mm_unpacklo_epi16(a2, a3);
      b1h = _mm_unpackhi_epi16(a2, a3);
      b2l = _mm_unpacklo_epi16(a4, a5);
      b2h = _mm_unpackhi_epi16(a4, a5);

      sum0 = _mm_dpwssd_epi32(sum0, b0l, c0);
      sum1 = _mm_dpwssd_epi32(sum1, b0h, c0);
      sum0 = _mm_dpwssd_epi32(sum0, b1l, c1);
      sum1 = _mm_dpwssd_epi32(sum1, b1h, c1);
      sum0 = _mm_dpwssd_epi32(sum0, b2l, c2);
      sum1 = _mm_dpwssd_epi32(sum1, b2h, c2);

      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 0); qtr += dstStride;
      *(EB_U32 *)qtr = _mm_extract_epi32(sum0, 1); qtr += dstStride;

      firstPassIFDst += 8;
      rowCount -= 2;
    }
    while (rowCount > 0);

    puWidth -= 4;
    if (puWidth == 0)
    {
      return;
    }

    firstPassIFDst += (fracPosy == 2) ? 32 : 24;
    dst += 4;
  }

  colCount = puWidth;
  do
  {
    EB_BYTE qtr = dst;

    rowCount = puHeight;
    do
    {
      a0 = _mm_loadu_si128((__m128i *)(firstPassIFDst+0*8));
      a1 = _mm_loadu_si128((__m128i *)(firstPassIFDst+1*8));
      a2 = _mm_loadu_si128((__m128i *)(firstPassIFDst+2*8));
      a3 = _mm_loadu_si128((__m128i *)(firstPassIFDst+3*8));
      a4 = _mm_loadu_si128((__m128i *)(firstPassIFDst+4*8));
      a5 = _mm_loadu_si128((__m128i *)(firstPassIFDst+5*8));
      a6 = _mm_loadu_si128((__m128i *)(firstPassIFDst+6*8));
      a0 = _mm_sub_epi16(a0, a6);

      sum0 = _mm_set1_epi32(257<<11);
      sum1 = _mm_set1_epi32(257<<11);

      b0l = _mm_unpacklo_epi16(a0, a1);
      b0h = _mm_unpackhi_epi16(a0, a1);
      b1l = _mm_unpacklo_epi16(a2, a3);
      b1h = _mm_unpackhi_epi16(a2, a3);
      b2l = _mm_unpacklo_epi16(a4, a5);
      b2h = _mm_unpackhi_epi16(a4, a5);

      sum0 = _mm_dpwssd_epi32(sum0, b0l, c0);
      sum1 = _mm_dpwssd_epi32(sum1, b0h, c0);
      sum0 = _mm_dpwssd_epi32(sum0, b1l, c1);
      sum1 = _mm_dpwssd_epi32(sum1, b1h, c1);
      sum0 = _mm_dpwssd_epi32(sum0, b2l, c2);
      sum1 = _mm_dpwssd_epi32(sum1, b2h, c2);

      sum0 = _mm_srai_epi32(sum0, 12);
      sum1 = _mm_srai_epi32(sum1, 12);
      sum0 = _mm_packs_epi32(sum0, sum1);
      sum0 = _mm_packus_epi16(sum0, sum0);

      _mm_storel_epi64((__m128i *)qtr, sum0); qtr += dstStride;

      firstPassIFDst += 8;
      rowCount--;
    }
    while (rowCount > 0);

    firstPassIFDst += (fracPosy == 2) ? 56 : 48;
    dst += 8;
    colCount -= 8;
  }
  while (colCount > 0);
}
#endif

void LumaInterpolationFilterPosnOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  (void)firstPassIFDst;

  LumaInterpolationFilterOneDOutRawVertical_SSSE3(refPic, srcStride, dst, puWidth, puHeight, 3);
}

void LumaInterpolationFilterPoseOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, 1);
  EbHevcLumaInterpolationFilterTwoDInRawOutRaw7(firstPassIFDst, dst, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosfOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  EB_U32 puHeight1 = puHeight + 6;
  EB_BYTE refPic1 = refPic - 3 * srcStride;
  LumaInterpolationFilterOneDOutRawHorizontal(refPic1, srcStride, firstPassIFDst, puWidth, puHeight1, 2);
  EbHevcLumaInterpolationFilterTwoDInRawOutRaw7(firstPassIFDst, dst, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosgOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, 3);
  EbHevcLumaInterpolationFilterTwoDInRawOutRaw7(firstPassIFDst, dst, puWidth, puHeight, 1);
}

void LumaInterpolationFilterPosiOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, 1);
  EbHevcLumaInterpolationFilterTwoDInRawOutRawM_SSSE3(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPosjOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, 2);
  EbHevcLumaInterpolationFilterTwoDInRawOutRawM_SSSE3(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPoskOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-3*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+7, 3);
  EbHevcLumaInterpolationFilterTwoDInRawOutRawM_SSSE3(firstPassIFDst, dst, puWidth, puHeight);
}

void LumaInterpolationFilterPospOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, 1);
  EbHevcLumaInterpolationFilterTwoDInRawOutRaw7(firstPassIFDst, dst, puWidth, puHeight, 3);
}

void LumaInterpolationFilterPosqOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  EB_U32 puHeight1 = puHeight + 6;
  EB_BYTE refPic1 = refPic - 2 * srcStride;
  LumaInterpolationFilterOneDOutRawHorizontal(refPic1, srcStride, firstPassIFDst, puWidth, puHeight1, 2);
  EbHevcLumaInterpolationFilterTwoDInRawOutRaw7(firstPassIFDst, dst, puWidth, puHeight, 3);
}

void LumaInterpolationFilterPosrOutRaw_SSSE3(
                                              EB_BYTE               refPic,
                                              EB_U32                srcStride,
                                              EB_S16*               dst,
                                              EB_U32                puWidth,
                                              EB_U32                puHeight,
                                              EB_S16               *firstPassIFDst)
{
  LumaInterpolationFilterOneDOutRawHorizontal(refPic-2*srcStride, srcStride, firstPassIFDst, puWidth, puHeight+6, 3);
  EbHevcLumaInterpolationFilterTwoDInRawOutRaw7(firstPassIFDst, dst, puWidth, puHeight, 3);
}
