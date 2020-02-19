#include "EbMcp_SSSE3.h"
#include "EbDefinitions.h"

#include "emmintrin.h"

#include "immintrin.h"

#ifndef NON_AVX512_SUPPORT



const EB_S16 EbHevcLumaFilterCoeff1[4][8] =
{
  { 0, 0,  0, 64,  0,  0, 0,  0},
  {-1, 4,-10, 58, 17, -5, 1,  0},
  {-1, 4,-11, 40, 40,-11, 4, -1},
  { 0, 1, -5, 17, 58,-10, 4, -1}
};

static const EB_S16 EbHevcLumaFilterCoeff7[4][8] =
  {
    { 0, 0,  0, 64,  0,  0, 0,  0},
    {-1, 4,-10, 58, 17, -5, 1,  0},
    {-1, 4,-11, 40, 40,-11, 4, -1},
    { 1, -5, 17, 58,-10, 4, -1, 0}
  };

void LumaInterpolationFilterOneDOutRawHorizontal_AVX512(
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

  c0 = _mm_loadu_si128((__m128i *)EbHevcLumaFilterCoeff1[fracPosx]);
  c0 = _mm_packs_epi16(c0, c0);
  __m128i ct = _mm_srli_epi64(c0, 32);
  __m512i cc0 = _mm512_broadcastd_epi32(c0);
  __m512i cc1 = _mm512_broadcastd_epi32(ct);  
  c0 = _mm_unpacklo_epi16(c0, c0);
  c3 = _mm_shuffle_epi32(c0, 0xff);
  c2 = _mm_shuffle_epi32(c0, 0xaa);
  c1 = _mm_shuffle_epi32(c0, 0x55);
  c0 = _mm_shuffle_epi32(c0, 0x00);
  __m512i b1 = _mm512_set_epi8(10, 9, 8, 7, 9, 8, 7, 6, 8, 7, 6, 5, 7, 6, 5, 4, 6, 5, 4, 3, 5, 4, 3, 2, 4, 3, 2, 1, 3, 2, 1, 0, 10, 9, 8, 7, 9, 8, 7, 6, 8, 7, 6, 5, 7, 6, 5, 4, 6, 5, 4, 3, 5, 4, 3, 2, 4, 3, 2, 1, 3, 2, 1, 0);
  __m512i b2 = _mm512_set_epi8(14, 13, 12, 11, 13, 12, 11, 10, 12, 11, 10, 9, 11, 10, 9, 8, 10, 9, 8, 7, 9, 8, 7, 6, 8, 7, 6, 5, 7, 6, 5, 4, 14, 13, 12, 11, 13, 12, 11, 10, 12, 11, 10, 9, 11, 10, 9, 8, 10, 9, 8, 7, 9, 8, 7, 6, 8, 7, 6, 5, 7, 6, 5, 4);
  

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
  int rowLoop = puHeight >>1 ;//divide by 2 
  int evenRow = puHeight & 1;
  do
  {
    ptr = refPic;
   // rowCount = puHeight;
   int rowCount = rowLoop ;//divide by 2 
   do
    {
		__m512i a1 = _mm512_broadcast_i32x4(_mm_loadu_si128((__m128i*)(ptr)));
		__m256i b0 = _mm256_broadcast_i32x4(_mm_loadu_si128((__m128i*)(ptr + srcStride))); ptr += 2 * srcStride;
		__m512i	s1 = _mm512_inserti64x4(a1, b0, 1);
		__m512i	sh2 = _mm512_shuffle_epi8(s1, b1);
		__m512i sh3 = _mm512_shuffle_epi8(s1, b2);
		__m512i sum00 = _mm512_setzero_si512();
		__m512i sum0 = _mm512_dpbusds_epi32(sum00, sh2, cc0);
		__m512i sum1 = _mm512_dpbusds_epi32(sum0, sh3, cc1);
		__m512i f1 = _mm512_packs_epi32(sum1,sum1);//
		__m512i f2 = _mm512_permutexvar_epi64( _mm512_setr_epi64(0x0, 0x0000000000000002, 0x0000000000000004, 0x0000000000000006, 0x0, 0x0002000200020002, 0x0004000400040004, 0x0006000600060006), f1);
		f2 = _mm512_sub_epi16(f2, _mm512_set1_epi16(128 * 64));
		_mm256_storeu_si256((__m256i*)dst, _mm512_castsi512_si256(f2));
    dst += 16;
	  rowCount = rowCount - 1;
    }
    while (rowCount > 0); 

    if (evenRow)
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
    
    refPic += 8;
    colCount -= 8;
  }
  while (colCount > 0);
}

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
