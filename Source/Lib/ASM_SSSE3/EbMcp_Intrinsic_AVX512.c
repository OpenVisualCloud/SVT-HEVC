#include "EbMcp_SSSE3.h"
#include "EbDefinitions.h"

#include "emmintrin.h"

#include "immintrin.h"

#ifndef NON_AVX512_SUPPORT

static const EB_S16 EbHevcLumaFilterCoeff7[4][8] =
  {
    { 0, 0,  0, 64,  0,  0, 0,  0},
    {-1, 4,-10, 58, 17, -5, 1,  0},
    {-1, 4,-11, 40, 40,-11, 4, -1},
    { 1, -5, 17, 58,-10, 4, -1, 0}
  };

void EbHevcLumaInterpolationFilterTwoDInRaw7_AVX512(EB_S16 *firstPassIFDst, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy)
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

void EbHevcLumaInterpolationFilterTwoDInRawOutRaw7_AVX512(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_U32 fracPosy)
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

void EbHevcLumaInterpolationFilterTwoDInRawM_AVX512(EB_S16 *firstPassIFDst, EB_BYTE dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight)
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

void EbHevcLumaInterpolationFilterTwoDInRawOutRawM_AVX512(EB_S16 *firstPassIFDst, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight)
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

#endif
