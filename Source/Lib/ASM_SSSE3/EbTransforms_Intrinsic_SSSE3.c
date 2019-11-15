/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"

#include "EbTransforms_SSSE3.h"

#include <emmintrin.h>
#include <tmmintrin.h>


#define SSSE3/// __SSSE3__



#ifdef __cplusplus
extern "C" const EB_S16 EbHevccoeff_tbl[48*8];
extern "C" const EB_S16 EbHevccoeff_tbl2[48*8];
#else
extern const EB_S16 EbHevccoeff_tbl[48*8];
extern const EB_S16 EbHevccoeff_tbl2[48*8];
#endif


// Reverse order of 16-bit elements within 128-bit vector
// This can be done more efficiently with _mm_shuffle_epi8 but requires SSSE3
static __m128i reverse_epi16(__m128i x)
{
#ifdef SSSE3/// __SSSE3__
  return _mm_shuffle_epi8(x, _mm_setr_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1));
#else
  x = _mm_shuffle_epi32(x, 0x1b); // 00011011
  x = _mm_shufflelo_epi16(x, 0xb1); // 10110001
  x = _mm_shufflehi_epi16(x, 0xb1);
  return x;
#endif
}


// transpose 16x16 block of data
static void transpose16(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride)
{
  EB_U32 i, j;
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 2; j++)
    {
      __m128i a0, a1, a2, a3, a4, a5, a6, a7;
      __m128i b0, b1, b2, b3, b4, b5, b6, b7;

      a0 = _mm_loadu_si128((const __m128i *)(src + (8*i+0)*src_stride + 8*j));
      a1 = _mm_loadu_si128((const __m128i *)(src + (8*i+1)*src_stride + 8*j));
      a2 = _mm_loadu_si128((const __m128i *)(src + (8*i+2)*src_stride + 8*j));
      a3 = _mm_loadu_si128((const __m128i *)(src + (8*i+3)*src_stride + 8*j));
      a4 = _mm_loadu_si128((const __m128i *)(src + (8*i+4)*src_stride + 8*j));
      a5 = _mm_loadu_si128((const __m128i *)(src + (8*i+5)*src_stride + 8*j));
      a6 = _mm_loadu_si128((const __m128i *)(src + (8*i+6)*src_stride + 8*j));
      a7 = _mm_loadu_si128((const __m128i *)(src + (8*i+7)*src_stride + 8*j));

      b0 = _mm_unpacklo_epi16(a0, a4);
      b1 = _mm_unpacklo_epi16(a1, a5);
      b2 = _mm_unpacklo_epi16(a2, a6);
      b3 = _mm_unpacklo_epi16(a3, a7);
      b4 = _mm_unpackhi_epi16(a0, a4);
      b5 = _mm_unpackhi_epi16(a1, a5);
      b6 = _mm_unpackhi_epi16(a2, a6);
      b7 = _mm_unpackhi_epi16(a3, a7);

      a0 = _mm_unpacklo_epi16(b0, b2);
      a1 = _mm_unpacklo_epi16(b1, b3);
      a2 = _mm_unpackhi_epi16(b0, b2);
      a3 = _mm_unpackhi_epi16(b1, b3);
      a4 = _mm_unpacklo_epi16(b4, b6);
      a5 = _mm_unpacklo_epi16(b5, b7);
      a6 = _mm_unpackhi_epi16(b4, b6);
      a7 = _mm_unpackhi_epi16(b5, b7);

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      b2 = _mm_unpacklo_epi16(a2, a3);
      b3 = _mm_unpackhi_epi16(a2, a3);
      b4 = _mm_unpacklo_epi16(a4, a5);
      b5 = _mm_unpackhi_epi16(a4, a5);
      b6 = _mm_unpacklo_epi16(a6, a7);
      b7 = _mm_unpackhi_epi16(a6, a7);

      _mm_storeu_si128((__m128i *)(dst + (8*j+0)*dst_stride + 8*i), b0);
      _mm_storeu_si128((__m128i *)(dst + (8*j+1)*dst_stride + 8*i), b1);
      _mm_storeu_si128((__m128i *)(dst + (8*j+2)*dst_stride + 8*i), b2);
      _mm_storeu_si128((__m128i *)(dst + (8*j+3)*dst_stride + 8*i), b3);
      _mm_storeu_si128((__m128i *)(dst + (8*j+4)*dst_stride + 8*i), b4);
      _mm_storeu_si128((__m128i *)(dst + (8*j+5)*dst_stride + 8*i), b5);
      _mm_storeu_si128((__m128i *)(dst + (8*j+6)*dst_stride + 8*i), b6);
      _mm_storeu_si128((__m128i *)(dst + (8*j+7)*dst_stride + 8*i), b7);
    }
  }
}

static void transpose16Partial(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 pattern)
{
  EB_U32 j;
  EB_U32 numRows = 2 - (pattern & 1);

  do
  {
    for (j = 0; j < 2; j++)
    {
      __m128i a0, a1, a2, a3, a4, a5, a6, a7;
      __m128i b0, b1, b2, b3, b4, b5, b6, b7;

      a0 = _mm_loadu_si128((const __m128i *)(src + (0)*src_stride + 8*j));
      a1 = _mm_loadu_si128((const __m128i *)(src + (1)*src_stride + 8*j));
      a2 = _mm_loadu_si128((const __m128i *)(src + (2)*src_stride + 8*j));
      a3 = _mm_loadu_si128((const __m128i *)(src + (3)*src_stride + 8*j));
      a4 = _mm_loadu_si128((const __m128i *)(src + (4)*src_stride + 8*j));
      a5 = _mm_loadu_si128((const __m128i *)(src + (5)*src_stride + 8*j));
      a6 = _mm_loadu_si128((const __m128i *)(src + (6)*src_stride + 8*j));
      a7 = _mm_loadu_si128((const __m128i *)(src + (7)*src_stride + 8*j));

      b0 = _mm_unpacklo_epi16(a0, a4);
      b1 = _mm_unpacklo_epi16(a1, a5);
      b2 = _mm_unpacklo_epi16(a2, a6);
      b3 = _mm_unpacklo_epi16(a3, a7);
      b4 = _mm_unpackhi_epi16(a0, a4);
      b5 = _mm_unpackhi_epi16(a1, a5);
      b6 = _mm_unpackhi_epi16(a2, a6);
      b7 = _mm_unpackhi_epi16(a3, a7);

      a0 = _mm_unpacklo_epi16(b0, b2);
      a1 = _mm_unpacklo_epi16(b1, b3);
      a2 = _mm_unpackhi_epi16(b0, b2);
      a3 = _mm_unpackhi_epi16(b1, b3);
      a4 = _mm_unpacklo_epi16(b4, b6);
      a5 = _mm_unpacklo_epi16(b5, b7);
      a6 = _mm_unpackhi_epi16(b4, b6);
      a7 = _mm_unpackhi_epi16(b5, b7);

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      b2 = _mm_unpacklo_epi16(a2, a3);
      b3 = _mm_unpackhi_epi16(a2, a3);
      b4 = _mm_unpacklo_epi16(a4, a5);
      b5 = _mm_unpackhi_epi16(a4, a5);
      b6 = _mm_unpacklo_epi16(a6, a7);
      b7 = _mm_unpackhi_epi16(a6, a7);

      _mm_storeu_si128((__m128i *)(dst + (8*j+0)*dst_stride), b0);
      _mm_storeu_si128((__m128i *)(dst + (8*j+1)*dst_stride), b1);
      _mm_storeu_si128((__m128i *)(dst + (8*j+2)*dst_stride), b2);
      _mm_storeu_si128((__m128i *)(dst + (8*j+3)*dst_stride), b3);
      _mm_storeu_si128((__m128i *)(dst + (8*j+4)*dst_stride), b4);
      _mm_storeu_si128((__m128i *)(dst + (8*j+5)*dst_stride), b5);
      _mm_storeu_si128((__m128i *)(dst + (8*j+6)*dst_stride), b6);
      _mm_storeu_si128((__m128i *)(dst + (8*j+7)*dst_stride), b7);
    }

    src += 8*src_stride;
    dst += 8;
  }
  while (--numRows);
}

static EB_U32 transpose16Check0s(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride)
{
  EB_U32 i, j;
  EB_U32 zeroPattern = 0;
  EB_U32 result = 0;

  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 2; j++)
    {
      __m128i a0, a1, a2, a3, a4, a5, a6, a7;
      __m128i b0, b1, b2, b3, b4, b5, b6, b7;
      __m128i c0;

      a0 = _mm_loadu_si128((const __m128i *)(src + (8*i+0)*src_stride + 8*j));
      a1 = _mm_loadu_si128((const __m128i *)(src + (8*i+1)*src_stride + 8*j));
      a2 = _mm_loadu_si128((const __m128i *)(src + (8*i+2)*src_stride + 8*j));
      a3 = _mm_loadu_si128((const __m128i *)(src + (8*i+3)*src_stride + 8*j));
      a4 = _mm_loadu_si128((const __m128i *)(src + (8*i+4)*src_stride + 8*j));
      a5 = _mm_loadu_si128((const __m128i *)(src + (8*i+5)*src_stride + 8*j));
      a6 = _mm_loadu_si128((const __m128i *)(src + (8*i+6)*src_stride + 8*j));
      a7 = _mm_loadu_si128((const __m128i *)(src + (8*i+7)*src_stride + 8*j));

      c0 = _mm_or_si128(a0, a4);
      c0 = _mm_or_si128(c0, a1);
      c0 = _mm_or_si128(c0, a5);
      c0 = _mm_or_si128(c0, a2);
      c0 = _mm_or_si128(c0, a6);
      c0 = _mm_or_si128(c0, a3);
      c0 = _mm_or_si128(c0, a7);

      c0 = _mm_cmpeq_epi8(c0, _mm_setzero_si128());

      zeroPattern = 2 * zeroPattern + ((_mm_movemask_epi8(c0)+1) >> 16); // add a '1' bit if all zeros

      b0 = _mm_unpacklo_epi16(a0, a4);
      b1 = _mm_unpacklo_epi16(a1, a5);
      b2 = _mm_unpacklo_epi16(a2, a6);
      b3 = _mm_unpacklo_epi16(a3, a7);
      b4 = _mm_unpackhi_epi16(a0, a4);
      b5 = _mm_unpackhi_epi16(a1, a5);
      b6 = _mm_unpackhi_epi16(a2, a6);
      b7 = _mm_unpackhi_epi16(a3, a7);

      a0 = _mm_unpacklo_epi16(b0, b2);
      a1 = _mm_unpacklo_epi16(b1, b3);
      a2 = _mm_unpackhi_epi16(b0, b2);
      a3 = _mm_unpackhi_epi16(b1, b3);
      a4 = _mm_unpacklo_epi16(b4, b6);
      a5 = _mm_unpacklo_epi16(b5, b7);
      a6 = _mm_unpackhi_epi16(b4, b6);
      a7 = _mm_unpackhi_epi16(b5, b7);

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      b2 = _mm_unpacklo_epi16(a2, a3);
      b3 = _mm_unpackhi_epi16(a2, a3);
      b4 = _mm_unpacklo_epi16(a4, a5);
      b5 = _mm_unpackhi_epi16(a4, a5);
      b6 = _mm_unpacklo_epi16(a6, a7);
      b7 = _mm_unpackhi_epi16(a6, a7);

      _mm_storeu_si128((__m128i *)(dst + (8*j+0)*dst_stride + 8*i), b0);
      _mm_storeu_si128((__m128i *)(dst + (8*j+1)*dst_stride + 8*i), b1);
      _mm_storeu_si128((__m128i *)(dst + (8*j+2)*dst_stride + 8*i), b2);
      _mm_storeu_si128((__m128i *)(dst + (8*j+3)*dst_stride + 8*i), b3);
      _mm_storeu_si128((__m128i *)(dst + (8*j+4)*dst_stride + 8*i), b4);
      _mm_storeu_si128((__m128i *)(dst + (8*j+5)*dst_stride + 8*i), b5);
      _mm_storeu_si128((__m128i *)(dst + (8*j+6)*dst_stride + 8*i), b6);
      _mm_storeu_si128((__m128i *)(dst + (8*j+7)*dst_stride + 8*i), b7);
    }
  }

  if ((zeroPattern & 3) == 3) result |= 1; // can do half transforms 1st pass
  if ((zeroPattern & 5) == 5) result |= 2; // can do half rows 1st pass, and half transforms 2nd pass
  return result;
}

// 16-point forward transform (16 rows)
static void transform16(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_S16 shift)
{
  EB_U32 i;
  __m128i s0 = _mm_cvtsi32_si128(shift);
  __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
  const __m128i *coeff32 = (const __m128i *)EbHevccoeff_tbl;

  for (i = 0; i < 16; i++)
  {
    __m128i x0, x1;
    __m128i y0, y1;
    __m128i a0, a1, a2, a3;
    __m128i b0, b1, b2, b3;

    y0 = _mm_loadu_si128((const __m128i *)(src+i*src_stride+0x00));
    y1 = _mm_loadu_si128((const __m128i *)(src+i*src_stride+0x08));


    // 16-point butterfly
    y1 = reverse_epi16(y1);

    x0 = _mm_add_epi16(y0, y1);
    x1 = _mm_sub_epi16(y0, y1);

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

    b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
    b1 = _mm_sra_epi32(_mm_add_epi32(a1, o0), s0);
    b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);
    b3 = _mm_sra_epi32(_mm_add_epi32(a3, o0), s0);

    x0 = _mm_packs_epi32(b0, b1);
    x1 = _mm_packs_epi32(b2, b3);

    y0 = _mm_unpacklo_epi16(x0, x1);
    y1 = _mm_unpackhi_epi16(x0, x1);

    _mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x00), y0);
    _mm_storeu_si128((__m128i *)(dst+i*dst_stride+0x08), y1);
  }
}

// 16-point inverse transform
static void invTransform16(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift, EB_U32 numRows)
{
  __m128i s0 = _mm_cvtsi32_si128(shift);
  __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
  const __m128i *coeff32 = (const __m128i *)EbHevccoeff_tbl2;

  do
  {
    __m128i x0, x1;
    __m128i a0, a1, a2, a3;
    __m128i b0, b1, b2, b3;
    x0 = _mm_loadu_si128((const __m128i *)(src+0x00)); // 00 01 02 03 04 05 06 07
    x1 = _mm_loadu_si128((const __m128i *)(src+0x08)); // 08 09 0a 0b 0c 0d 0e 0f

#ifdef SSSE3/// __SSSE3__
    x0 = _mm_shuffle_epi8(x0, _mm_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15));
    x1 = _mm_shuffle_epi8(x1, _mm_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15));
#else
    x0 = _mm_shufflelo_epi16(x0, 0xd8); // 00 02 01 03 04 06 05 07
    x1 = _mm_shufflelo_epi16(x1, 0xd8); // 08 0a 09 0b 0c 0e 0d 0f
    x0 = _mm_shufflehi_epi16(x0, 0xd8);
    x1 = _mm_shufflehi_epi16(x1, 0xd8);
#endif

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]); // 00 02
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[2])); // 04 06
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[4])); // 08 0a
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[6])); // 0c 0e

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[3]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[5]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[7]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[8]);
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[10]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[12]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[11]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[13]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

    a0 = _mm_add_epi32(a0, o0);
    a1 = _mm_add_epi32(a1, o0);

    b0 = _mm_add_epi32(a0, a2);
    b1 = _mm_add_epi32(a1, a3);
    b2 = _mm_sub_epi32(a0, a2);
    b3 = _mm_sub_epi32(a1, a3);

    a0 = b0;
    a1 = b1;
    a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
    a3 = _mm_shuffle_epi32(b2, 0x1b);

    a0 = _mm_sra_epi32(a0, s0);
    a1 = _mm_sra_epi32(a1, s0);
    a2 = _mm_sra_epi32(a2, s0);
    a3 = _mm_sra_epi32(a3, s0);

    x0 = _mm_packs_epi32(a0, a1);
    x1 = _mm_packs_epi32(a2, a3);

    _mm_storeu_si128((__m128i *)(dst+0x00), x0);
    _mm_storeu_si128((__m128i *)(dst+0x08), x1);

    src += src_stride;
    dst += dst_stride;
  }
  while (--numRows);
}

// 16-point inverse transform
static void invTransform16Half(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift, EB_U32 numRows)
{
  __m128i s0 = _mm_cvtsi32_si128(shift);
  __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
  const __m128i *coeff32 = (const __m128i *)EbHevccoeff_tbl2;

  do
  {
    __m128i x0, x1;
    __m128i a0, a1, a2, a3;
    __m128i b0, b1, b2, b3;
    x0 = _mm_loadu_si128((const __m128i *)(src+0x00)); // 00 01 02 03 04 05 06 07

#ifdef SSSE3/// __SSSE3__
    x0 = _mm_shuffle_epi8(x0, _mm_setr_epi8(0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15));
#else
    x0 = _mm_shufflelo_epi16(x0, 0xd8); // 00 02 01 03 04 06 05 07
    x0 = _mm_shufflehi_epi16(x0, 0xd8);
#endif

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]); // 00 02
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[2])); // 04 06

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[3]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[8]);
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[10]));

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[11]));

    a0 = _mm_add_epi32(a0, o0);
    a1 = _mm_add_epi32(a1, o0);

    b0 = _mm_add_epi32(a0, a2);
    b1 = _mm_add_epi32(a1, a3);
    b2 = _mm_sub_epi32(a0, a2);
    b3 = _mm_sub_epi32(a1, a3);

    a0 = b0;
    a1 = b1;
    a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
    a3 = _mm_shuffle_epi32(b2, 0x1b);

    a0 = _mm_sra_epi32(a0, s0);
    a1 = _mm_sra_epi32(a1, s0);
    a2 = _mm_sra_epi32(a2, s0);
    a3 = _mm_sra_epi32(a3, s0);

    x0 = _mm_packs_epi32(a0, a1);
    x1 = _mm_packs_epi32(a2, a3);

    _mm_storeu_si128((__m128i *)(dst+0x00), x0);
    _mm_storeu_si128((__m128i *)(dst+0x08), x1);

    src += src_stride;
    dst += dst_stride;
  }
  while (--numRows);
}


static void invTransform16Partial(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift, EB_U32 pattern)
{
  EB_U32 numRows = 16 - 4 * (pattern & 2);
  if (pattern & 1)
  {
    invTransform16Half(src, src_stride, dst, dst_stride, shift, numRows);
  }
  else
  {
    invTransform16(src, src_stride, dst, dst_stride, shift, numRows);
  }
}

// inverse 16x16 transform
void PFinvTransform16x16_SSSE3(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_S16 *intermediate, EB_U32 addshift)
{

  EB_U32 pattern = transpose16Check0s(src, src_stride, intermediate, 16);
  invTransform16Partial(intermediate, 16, dst, dst_stride, 7, pattern);

  pattern >>= 1;
  transpose16Partial(dst, dst_stride, intermediate, 16, pattern);
  invTransform16Partial(intermediate, 16, dst, dst_stride, 12-addshift, pattern);

}

// transpose 32x32 block of data
static void transpose32(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride)
{
  EB_U32 i, j;
  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      __m128i a0, a1, a2, a3, a4, a5, a6, a7;
      __m128i b0, b1, b2, b3, b4, b5, b6, b7;

      a0 = _mm_loadu_si128((const __m128i *)(src + (8*i+0)*src_stride + 8*j));
      a1 = _mm_loadu_si128((const __m128i *)(src + (8*i+1)*src_stride + 8*j));
      a2 = _mm_loadu_si128((const __m128i *)(src + (8*i+2)*src_stride + 8*j));
      a3 = _mm_loadu_si128((const __m128i *)(src + (8*i+3)*src_stride + 8*j));
      a4 = _mm_loadu_si128((const __m128i *)(src + (8*i+4)*src_stride + 8*j));
      a5 = _mm_loadu_si128((const __m128i *)(src + (8*i+5)*src_stride + 8*j));
      a6 = _mm_loadu_si128((const __m128i *)(src + (8*i+6)*src_stride + 8*j));
      a7 = _mm_loadu_si128((const __m128i *)(src + (8*i+7)*src_stride + 8*j));

      b0 = _mm_unpacklo_epi16(a0, a4);
      b1 = _mm_unpacklo_epi16(a1, a5);
      b2 = _mm_unpacklo_epi16(a2, a6);
      b3 = _mm_unpacklo_epi16(a3, a7);
      b4 = _mm_unpackhi_epi16(a0, a4);
      b5 = _mm_unpackhi_epi16(a1, a5);
      b6 = _mm_unpackhi_epi16(a2, a6);
      b7 = _mm_unpackhi_epi16(a3, a7);

      a0 = _mm_unpacklo_epi16(b0, b2);
      a1 = _mm_unpacklo_epi16(b1, b3);
      a2 = _mm_unpackhi_epi16(b0, b2);
      a3 = _mm_unpackhi_epi16(b1, b3);
      a4 = _mm_unpacklo_epi16(b4, b6);
      a5 = _mm_unpacklo_epi16(b5, b7);
      a6 = _mm_unpackhi_epi16(b4, b6);
      a7 = _mm_unpackhi_epi16(b5, b7);

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      b2 = _mm_unpacklo_epi16(a2, a3);
      b3 = _mm_unpackhi_epi16(a2, a3);
      b4 = _mm_unpacklo_epi16(a4, a5);
      b5 = _mm_unpackhi_epi16(a4, a5);
      b6 = _mm_unpacklo_epi16(a6, a7);
      b7 = _mm_unpackhi_epi16(a6, a7);

      _mm_storeu_si128((__m128i *)(dst + (8*j+0)*dst_stride + 8*i), b0);
      _mm_storeu_si128((__m128i *)(dst + (8*j+1)*dst_stride + 8*i), b1);
      _mm_storeu_si128((__m128i *)(dst + (8*j+2)*dst_stride + 8*i), b2);
      _mm_storeu_si128((__m128i *)(dst + (8*j+3)*dst_stride + 8*i), b3);
      _mm_storeu_si128((__m128i *)(dst + (8*j+4)*dst_stride + 8*i), b4);
      _mm_storeu_si128((__m128i *)(dst + (8*j+5)*dst_stride + 8*i), b5);
      _mm_storeu_si128((__m128i *)(dst + (8*j+6)*dst_stride + 8*i), b6);
      _mm_storeu_si128((__m128i *)(dst + (8*j+7)*dst_stride + 8*i), b7);
    }
  }
}

static void transpose32Partial(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 pattern)
{
  EB_U32 j;
  EB_U32 numRows = 4 - (pattern & 3);

  do
  {
    for (j = 0; j < 4; j++)
    {
      __m128i a0, a1, a2, a3, a4, a5, a6, a7;
      __m128i b0, b1, b2, b3, b4, b5, b6, b7;

      a0 = _mm_loadu_si128((const __m128i *)(src + (0)*src_stride + 8*j));
      a1 = _mm_loadu_si128((const __m128i *)(src + (1)*src_stride + 8*j));
      a2 = _mm_loadu_si128((const __m128i *)(src + (2)*src_stride + 8*j));
      a3 = _mm_loadu_si128((const __m128i *)(src + (3)*src_stride + 8*j));
      a4 = _mm_loadu_si128((const __m128i *)(src + (4)*src_stride + 8*j));
      a5 = _mm_loadu_si128((const __m128i *)(src + (5)*src_stride + 8*j));
      a6 = _mm_loadu_si128((const __m128i *)(src + (6)*src_stride + 8*j));
      a7 = _mm_loadu_si128((const __m128i *)(src + (7)*src_stride + 8*j));

      b0 = _mm_unpacklo_epi16(a0, a4);
      b1 = _mm_unpacklo_epi16(a1, a5);
      b2 = _mm_unpacklo_epi16(a2, a6);
      b3 = _mm_unpacklo_epi16(a3, a7);
      b4 = _mm_unpackhi_epi16(a0, a4);
      b5 = _mm_unpackhi_epi16(a1, a5);
      b6 = _mm_unpackhi_epi16(a2, a6);
      b7 = _mm_unpackhi_epi16(a3, a7);

      a0 = _mm_unpacklo_epi16(b0, b2);
      a1 = _mm_unpacklo_epi16(b1, b3);
      a2 = _mm_unpackhi_epi16(b0, b2);
      a3 = _mm_unpackhi_epi16(b1, b3);
      a4 = _mm_unpacklo_epi16(b4, b6);
      a5 = _mm_unpacklo_epi16(b5, b7);
      a6 = _mm_unpackhi_epi16(b4, b6);
      a7 = _mm_unpackhi_epi16(b5, b7);

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      b2 = _mm_unpacklo_epi16(a2, a3);
      b3 = _mm_unpackhi_epi16(a2, a3);
      b4 = _mm_unpacklo_epi16(a4, a5);
      b5 = _mm_unpackhi_epi16(a4, a5);
      b6 = _mm_unpacklo_epi16(a6, a7);
      b7 = _mm_unpackhi_epi16(a6, a7);

      _mm_storeu_si128((__m128i *)(dst + (8*j+0)*dst_stride), b0);
      _mm_storeu_si128((__m128i *)(dst + (8*j+1)*dst_stride), b1);
      _mm_storeu_si128((__m128i *)(dst + (8*j+2)*dst_stride), b2);
      _mm_storeu_si128((__m128i *)(dst + (8*j+3)*dst_stride), b3);
      _mm_storeu_si128((__m128i *)(dst + (8*j+4)*dst_stride), b4);
      _mm_storeu_si128((__m128i *)(dst + (8*j+5)*dst_stride), b5);
      _mm_storeu_si128((__m128i *)(dst + (8*j+6)*dst_stride), b6);
      _mm_storeu_si128((__m128i *)(dst + (8*j+7)*dst_stride), b7);
    }

    src += 8 * src_stride;
    dst += 8;
  }
  while (--numRows);
}

static EB_U32 transpose32Check0s(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride)
{
  EB_U32 i, j;
  EB_U32 zeroPattern = 0;
  EB_U32 result = 0;

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      __m128i a0, a1, a2, a3, a4, a5, a6, a7;
      __m128i b0, b1, b2, b3, b4, b5, b6, b7;
      __m128i c0;

      a0 = _mm_loadu_si128((const __m128i *)(src + (8*i+0)*src_stride + 8*j));
      a1 = _mm_loadu_si128((const __m128i *)(src + (8*i+1)*src_stride + 8*j));
      a2 = _mm_loadu_si128((const __m128i *)(src + (8*i+2)*src_stride + 8*j));
      a3 = _mm_loadu_si128((const __m128i *)(src + (8*i+3)*src_stride + 8*j));
      a4 = _mm_loadu_si128((const __m128i *)(src + (8*i+4)*src_stride + 8*j));
      a5 = _mm_loadu_si128((const __m128i *)(src + (8*i+5)*src_stride + 8*j));
      a6 = _mm_loadu_si128((const __m128i *)(src + (8*i+6)*src_stride + 8*j));
      a7 = _mm_loadu_si128((const __m128i *)(src + (8*i+7)*src_stride + 8*j));

      c0 = _mm_or_si128(a0, a4);
      c0 = _mm_or_si128(c0, a1);
      c0 = _mm_or_si128(c0, a5);
      c0 = _mm_or_si128(c0, a2);
      c0 = _mm_or_si128(c0, a6);
      c0 = _mm_or_si128(c0, a3);
      c0 = _mm_or_si128(c0, a7);

      c0 = _mm_cmpeq_epi8(c0, _mm_setzero_si128());

      zeroPattern = 2 * zeroPattern + ((_mm_movemask_epi8(c0)+1) >> 16); // add a '1' bit if all zeros

      b0 = _mm_unpacklo_epi16(a0, a4);
      b1 = _mm_unpacklo_epi16(a1, a5);
      b2 = _mm_unpacklo_epi16(a2, a6);
      b3 = _mm_unpacklo_epi16(a3, a7);
      b4 = _mm_unpackhi_epi16(a0, a4);
      b5 = _mm_unpackhi_epi16(a1, a5);
      b6 = _mm_unpackhi_epi16(a2, a6);
      b7 = _mm_unpackhi_epi16(a3, a7);

      a0 = _mm_unpacklo_epi16(b0, b2);
      a1 = _mm_unpacklo_epi16(b1, b3);
      a2 = _mm_unpackhi_epi16(b0, b2);
      a3 = _mm_unpackhi_epi16(b1, b3);
      a4 = _mm_unpacklo_epi16(b4, b6);
      a5 = _mm_unpacklo_epi16(b5, b7);
      a6 = _mm_unpackhi_epi16(b4, b6);
      a7 = _mm_unpackhi_epi16(b5, b7);

      b0 = _mm_unpacklo_epi16(a0, a1);
      b1 = _mm_unpackhi_epi16(a0, a1);
      b2 = _mm_unpacklo_epi16(a2, a3);
      b3 = _mm_unpackhi_epi16(a2, a3);
      b4 = _mm_unpacklo_epi16(a4, a5);
      b5 = _mm_unpackhi_epi16(a4, a5);
      b6 = _mm_unpacklo_epi16(a6, a7);
      b7 = _mm_unpackhi_epi16(a6, a7);

      _mm_storeu_si128((__m128i *)(dst + (8*j+0)*dst_stride + 8*i), b0);
      _mm_storeu_si128((__m128i *)(dst + (8*j+1)*dst_stride + 8*i), b1);
      _mm_storeu_si128((__m128i *)(dst + (8*j+2)*dst_stride + 8*i), b2);
      _mm_storeu_si128((__m128i *)(dst + (8*j+3)*dst_stride + 8*i), b3);
      _mm_storeu_si128((__m128i *)(dst + (8*j+4)*dst_stride + 8*i), b4);
      _mm_storeu_si128((__m128i *)(dst + (8*j+5)*dst_stride + 8*i), b5);
      _mm_storeu_si128((__m128i *)(dst + (8*j+6)*dst_stride + 8*i), b6);
      _mm_storeu_si128((__m128i *)(dst + (8*j+7)*dst_stride + 8*i), b7);
    }
  }

  if ((zeroPattern & 0xfff) == 0xfff) result |= 3;
  else if ((zeroPattern & 0xff) == 0xff) result |= 2;
  else if ((zeroPattern & 0xf) == 0xf) result |= 1;

  if ((zeroPattern & 0x7777) == 0x7777) result |= 3*4;
  else if ((zeroPattern & 0x3333) == 0x3333) result |= 2*4;
  else if ((zeroPattern & 0x1111) == 0x1111) result |= 1*4;

  return result;
}

// 32-point forward transform (32 rows)
static void transform32(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift)
{
  __m128i s0 = _mm_cvtsi32_si128(shift);
  __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
  const __m128i *coeff32 = (const __m128i *)EbHevccoeff_tbl;

  EB_U32 numRows = 32;
  do
  {
    __m128i x0, x1, x2, x3;
    __m128i y0, y1, y2, y3;
    __m128i a0, a1, a2, a3, a4, a5, a6, a7;
    __m128i b0, b1, b2, b3, b4, b5, b6, b7;

    x0 = _mm_loadu_si128((const __m128i *)(src+0x00));
    x1 = _mm_loadu_si128((const __m128i *)(src+0x08));
    x2 = _mm_loadu_si128((const __m128i *)(src+0x10));
    x3 = _mm_loadu_si128((const __m128i *)(src+0x18));


    // 32-point butterfly
    x2 = reverse_epi16(x2);
    x3 = reverse_epi16(x3);

    y0 = _mm_add_epi16(x0, x3);
    y1 = _mm_add_epi16(x1, x2);

    y2 = _mm_sub_epi16(x0, x3);
    y3 = _mm_sub_epi16(x1, x2);

    // 16-point butterfly
    y1 = reverse_epi16(y1);

    x0 = _mm_add_epi16(y0, y1);
    x1 = _mm_sub_epi16(y0, y1);


    x2 = y2;
    x3 = y3;

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2]));
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4]));
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6]));

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]);
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14]));

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]);
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44]));

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));

    b0 = _mm_sra_epi32(_mm_add_epi32(a0, o0), s0);
    b1 = _mm_sra_epi32(_mm_add_epi32(a1, o0), s0);
    b2 = _mm_sra_epi32(_mm_add_epi32(a2, o0), s0);
    b3 = _mm_sra_epi32(_mm_add_epi32(a3, o0), s0);
    b4 = _mm_sra_epi32(_mm_add_epi32(a4, o0), s0);
    b5 = _mm_sra_epi32(_mm_add_epi32(a5, o0), s0);
    b6 = _mm_sra_epi32(_mm_add_epi32(a6, o0), s0);
    b7 = _mm_sra_epi32(_mm_add_epi32(a7, o0), s0);

    x0 = _mm_packs_epi32(b0, b1);
    x1 = _mm_packs_epi32(b2, b3);
    x2 = _mm_packs_epi32(b4, b5);
    x3 = _mm_packs_epi32(b6, b7);

    y0 = _mm_unpacklo_epi16(x0, x1);
    y1 = _mm_unpackhi_epi16(x0, x1);
    y2 = x2;
    y3 = x3;
    x0 = _mm_unpacklo_epi16(y0, y2);
    x1 = _mm_unpackhi_epi16(y0, y2);
    x2 = _mm_unpacklo_epi16(y1, y3);
    x3 = _mm_unpackhi_epi16(y1, y3);

    _mm_storeu_si128((__m128i *)(dst+0x00), x0);
    _mm_storeu_si128((__m128i *)(dst+0x08), x1);
    _mm_storeu_si128((__m128i *)(dst+0x10), x2);
    _mm_storeu_si128((__m128i *)(dst+0x18), x3);

    src += src_stride;
    dst += dst_stride;
  }
  while (--numRows);
}

// 32-point inverse transform (32 rows)
static void invTransform32(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift, EB_U32 numRows)
{
  __m128i s0 = _mm_cvtsi32_si128(shift);
  __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
  const __m128i *coeff32 = (const __m128i *)EbHevccoeff_tbl2;

  do
  {
    __m128i x0, x1, x2, x3;
#ifndef SSSE3/// __SSSE3__
    __m128i y0, y1, y2, y3;
#endif
    __m128i a0, a1, a2, a3, a4, a5, a6, a7;
    __m128i b0, b1, b2, b3, b4, b5, b6, b7;
    x0 = _mm_loadu_si128((const __m128i *)(src+0x00)); // 00 01 02 03 04 05 06 07
    x1 = _mm_loadu_si128((const __m128i *)(src+0x08)); // 08 09 0a 0b 0c 0d 0e 0f
    x2 = _mm_loadu_si128((const __m128i *)(src+0x10)); // 10 11 12 13 14 15 16 17
    x3 = _mm_loadu_si128((const __m128i *)(src+0x18)); // 18 19 1a 1b 1c 1d 1e 1f

#ifdef SSSE3/// __SSSE3__
    x0 = _mm_shuffle_epi8(x0, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 00 04 02 06 01 03 05 07
    x1 = _mm_shuffle_epi8(x1, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 08 0c 0a 0e 09 0b 0d 0f
    x2 = _mm_shuffle_epi8(x2, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 10 14 12 16 11 13 15 17
    x3 = _mm_shuffle_epi8(x3, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 18 1c 1a 1e 19 1b 1d 1f

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[2]));
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[4]));
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[6]));

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[3]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[5]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[7]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[8]);
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[12]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[14]));

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[13]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[15]));

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[16]);
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[20]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[24]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[28]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[32]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[36]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44]));

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[21]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[25]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[29]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[33]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[37]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[22]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[26]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[30]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[34]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[38]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[23]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[27]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[31]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[35]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[39]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));

#else
    y0 = _mm_unpacklo_epi16(x0, x1); // 00 08 01 09 02 0a 03 0b
    y1 = _mm_unpackhi_epi16(x0, x1); // 04 0c 05 0d 06 0e 07 0f
    y2 = _mm_unpacklo_epi16(x2, x3); // 10 18
    y3 = _mm_unpackhi_epi16(x2, x3); // 24 2c

    x0 = _mm_unpacklo_epi16(y0, y1); // 00 04 08 0c 01 05 09 0d
    x1 = _mm_unpackhi_epi16(y0, y1); // 02 06 0a 0e 03 07 0b 0f
    x2 = _mm_unpacklo_epi16(y2, y3); // 10 14
    x3 = _mm_unpackhi_epi16(y2, y3); // 12 16

    y0 = _mm_unpacklo_epi64(x0, x2); // 00 04 08 0c 10 14 18 1c
    y1 = _mm_unpacklo_epi64(x1, x3); // 02 06 0a 0e 12 16 1a 1e
    y2 = _mm_unpackhi_epi16(x0, x1); // 01 03 05 07 09 0b 0d 0f
    y3 = _mm_unpackhi_epi16(x2, x3); // 11 13 15 17 19 1b 1d 1f

    x0 = y0;
    x1 = y1;
    x2 = y2;
    x3 = y3;

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]); // 00 04
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2])); // 08 0c
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4])); // 10 14
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[6])); // 18 1c

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[7]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]); // 02 06
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10])); // 0a 0e
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12])); // 12 16
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[14])); // 1a 1e

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[15]));

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]); // 01 03
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20])); // 05 07
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24])); // 09 0b
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28])); // 0d 0f
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32])); // 11 13
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36])); // 15 17
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[40])); // 19 1b
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[44])); // 1d 1f

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[41]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[45]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[42]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[46]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xaa), coeff32[43]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0xff), coeff32[47]));
#endif

    a0 = _mm_add_epi32(a0, o0);
    a1 = _mm_add_epi32(a1, o0);

    b0 = _mm_add_epi32(a0, a2);
    b1 = _mm_add_epi32(a1, a3);
    b2 = _mm_sub_epi32(a0, a2);
    b3 = _mm_sub_epi32(a1, a3);

    a0 = b0;
    a1 = b1;
    a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
    a3 = _mm_shuffle_epi32(b2, 0x1b);

    b0 = _mm_add_epi32(a0, a4);
    b1 = _mm_add_epi32(a1, a5);
    b2 = _mm_add_epi32(a2, a6);
    b3 = _mm_add_epi32(a3, a7);
    b4 = _mm_sub_epi32(a0, a4);
    b5 = _mm_sub_epi32(a1, a5);
    b6 = _mm_sub_epi32(a2, a6);
    b7 = _mm_sub_epi32(a3, a7);

    a0 = _mm_sra_epi32(b0, s0);
    a1 = _mm_sra_epi32(b1, s0);
    a2 = _mm_sra_epi32(b2, s0);
    a3 = _mm_sra_epi32(b3, s0);
    a4 = _mm_sra_epi32(_mm_shuffle_epi32(b7, 0x1b), s0);
    a5 = _mm_sra_epi32(_mm_shuffle_epi32(b6, 0x1b), s0);
    a6 = _mm_sra_epi32(_mm_shuffle_epi32(b5, 0x1b), s0);
    a7 = _mm_sra_epi32(_mm_shuffle_epi32(b4, 0x1b), s0);

    x0 = _mm_packs_epi32(a0, a1);
    x1 = _mm_packs_epi32(a2, a3);
    x2 = _mm_packs_epi32(a4, a5);
    x3 = _mm_packs_epi32(a6, a7);

    _mm_storeu_si128((__m128i *)(dst+0x00), x0);
    _mm_storeu_si128((__m128i *)(dst+0x08), x1);
    _mm_storeu_si128((__m128i *)(dst+0x10), x2);
    _mm_storeu_si128((__m128i *)(dst+0x18), x3);

    src += src_stride;
    dst += dst_stride;
  }
  while (--numRows);
}

static void invTransform32ThreeQuarter(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift, EB_U32 numRows)
{
  __m128i s0 = _mm_cvtsi32_si128(shift);
  __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
  const __m128i *coeff32 = (const __m128i *)EbHevccoeff_tbl2;

  do
  {
    __m128i x0, x1, x2, x3;
#ifndef SSSE3/// __SSSE3__
    __m128i y0, y1, y2, y3;
#endif
    __m128i a0, a1, a2, a3, a4, a5, a6, a7;
    __m128i b0, b1, b2, b3, b4, b5, b6, b7;
    x0 = _mm_loadu_si128((const __m128i *)(src+0x00)); // 00 01 02 03 04 05 06 07
    x1 = _mm_loadu_si128((const __m128i *)(src+0x08)); // 08 09 0a 0b 0c 0d 0e 0f
    x2 = _mm_loadu_si128((const __m128i *)(src+0x10)); // 10 11 12 13 14 15 16 17
    x3 = _mm_setzero_si128();

#ifdef SSSE3/// __SSSE3__
    x0 = _mm_shuffle_epi8(x0, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 00 04 02 06 01 03 05 07
    x1 = _mm_shuffle_epi8(x1, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 08 0c 0a 0e 09 0b 0d 0f
    x2 = _mm_shuffle_epi8(x2, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 10 14 12 16 11 13 15 17

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[2]));
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[4]));

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[3]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[5]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[8]);
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[12]));

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[13]));

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[16]);
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[20]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[24]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[28]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[32]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[36]));

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[21]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[25]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[29]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[33]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[37]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[22]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[26]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[30]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[34]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[38]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[23]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[27]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[31]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[35]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[39]));

#else
    y0 = _mm_unpacklo_epi16(x0, x1); // 00 08 01 09 02 0a 03 0b
    y1 = _mm_unpackhi_epi16(x0, x1); // 04 0c 05 0d 06 0e 07 0f
    y2 = _mm_unpacklo_epi16(x2, x3); // 10 18
    y3 = _mm_unpackhi_epi16(x2, x3); // 24 2c

    x0 = _mm_unpacklo_epi16(y0, y1); // 00 04 08 0c 01 05 09 0d
    x1 = _mm_unpackhi_epi16(y0, y1); // 02 06 0a 0e 03 07 0b 0f
    x2 = _mm_unpacklo_epi16(y2, y3); // 10 14
    x3 = _mm_unpackhi_epi16(y2, y3); // 12 16

    y0 = _mm_unpacklo_epi64(x0, x2); // 00 04 08 0c 10 14 18 1c
    y1 = _mm_unpacklo_epi64(x1, x3); // 02 06 0a 0e 12 16 1a 1e
    y2 = _mm_unpackhi_epi16(x0, x1); // 01 03 05 07 09 0b 0d 0f
    y3 = _mm_unpackhi_epi16(x2, x3); // 11 13 15 17 19 1b 1d 1f

    x0 = y0;
    x1 = y1;
    x2 = y2;
    x3 = y3;

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]); // 00 04
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2])); // 08 0c
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[4])); // 10 14

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[5]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]); // 02 06
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10])); // 0a 0e
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[12])); // 12 16

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[13]));

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]); // 01 03
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20])); // 05 07
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24])); // 09 0b
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28])); // 0d 0f
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[32])); // 11 13
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[36])); // 15 17

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[33]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[37]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[34]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[38]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x00), coeff32[35]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x3, 0x55), coeff32[39]));
#endif

    a0 = _mm_add_epi32(a0, o0);
    a1 = _mm_add_epi32(a1, o0);

    b0 = _mm_add_epi32(a0, a2);
    b1 = _mm_add_epi32(a1, a3);
    b2 = _mm_sub_epi32(a0, a2);
    b3 = _mm_sub_epi32(a1, a3);

    a0 = b0;
    a1 = b1;
    a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
    a3 = _mm_shuffle_epi32(b2, 0x1b);

    b0 = _mm_add_epi32(a0, a4);
    b1 = _mm_add_epi32(a1, a5);
    b2 = _mm_add_epi32(a2, a6);
    b3 = _mm_add_epi32(a3, a7);
    b4 = _mm_sub_epi32(a0, a4);
    b5 = _mm_sub_epi32(a1, a5);
    b6 = _mm_sub_epi32(a2, a6);
    b7 = _mm_sub_epi32(a3, a7);

    a0 = _mm_sra_epi32(b0, s0);
    a1 = _mm_sra_epi32(b1, s0);
    a2 = _mm_sra_epi32(b2, s0);
    a3 = _mm_sra_epi32(b3, s0);
    a4 = _mm_sra_epi32(_mm_shuffle_epi32(b7, 0x1b), s0);
    a5 = _mm_sra_epi32(_mm_shuffle_epi32(b6, 0x1b), s0);
    a6 = _mm_sra_epi32(_mm_shuffle_epi32(b5, 0x1b), s0);
    a7 = _mm_sra_epi32(_mm_shuffle_epi32(b4, 0x1b), s0);

    x0 = _mm_packs_epi32(a0, a1);
    x1 = _mm_packs_epi32(a2, a3);
    x2 = _mm_packs_epi32(a4, a5);
    x3 = _mm_packs_epi32(a6, a7);

    _mm_storeu_si128((__m128i *)(dst+0x00), x0);
    _mm_storeu_si128((__m128i *)(dst+0x08), x1);
    _mm_storeu_si128((__m128i *)(dst+0x10), x2);
    _mm_storeu_si128((__m128i *)(dst+0x18), x3);

    src += src_stride;
    dst += dst_stride;
  }
  while (--numRows);
}

static void invTransform32Half(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift, EB_U32 numRows)
{
  __m128i s0 = _mm_cvtsi32_si128(shift);
  __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
  const __m128i *coeff32 = (const __m128i *)EbHevccoeff_tbl2;

  do
  {
    __m128i x0, x1, x2, x3;
#ifndef SSSE3/// __SSSE3__
    __m128i y0, y1, y2;
#endif
    __m128i a0, a1, a2, a3, a4, a5, a6, a7;
    __m128i b0, b1, b2, b3, b4, b5, b6, b7;
    x0 = _mm_loadu_si128((const __m128i *)(src+0x00)); // 00 01 02 03 04 05 06 07
    x1 = _mm_loadu_si128((const __m128i *)(src+0x08)); // 08 09 0a 0b 0c 0d 0e 0f
    x2 = _mm_setzero_si128();
    x3 = _mm_setzero_si128();

#ifdef SSSE3/// __SSSE3__
    x0 = _mm_shuffle_epi8(x0, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 00 04 02 06 01 03 05 07
    x1 = _mm_shuffle_epi8(x1, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 08 0c 0a 0e 09 0b 0d 0f

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[2]));

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[3]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[8]);
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10]));

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[16]);
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[20]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[24]));
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[28]));

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[21]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[25]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[29]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[22]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[26]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[30]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[23]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xaa), coeff32[27]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0xff), coeff32[31]));

#else
    y0 = _mm_unpacklo_epi16(x0, x1); // 00 08 01 09 02 0a 03 0b
    y1 = _mm_unpackhi_epi16(x0, x1); // 04 0c 05 0d 06 0e 07 0f

    x0 = _mm_unpacklo_epi16(y0, y1); // 00 04 08 0c 01 05 09 0d
    x1 = _mm_unpackhi_epi16(y0, y1); // 02 06 0a 0e 03 07 0b 0f

    y0 = _mm_unpacklo_epi64(x0, x2); // 00 04 08 0c 10 14 18 1c
    y1 = _mm_unpacklo_epi64(x1, x3); // 02 06 0a 0e 12 16 1a 1e
    y2 = _mm_unpackhi_epi16(x0, x1); // 01 03 05 07 09 0b 0d 0f

    x0 = y0;
    x1 = y1;
    x2 = y2;

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]); // 00 04
    a0 = _mm_add_epi32(a0, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[2])); // 08 0c

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);
    a1 = _mm_add_epi32(a1, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[3]));

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]); // 02 06
    a2 = _mm_add_epi32(a2, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[10])); // 0a 0e

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);
    a3 = _mm_add_epi32(a3, _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x55), coeff32[11]));

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]); // 01 03
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20])); // 05 07
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[24])); // 09 0b
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[28])); // 0d 0f

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[25]));
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[29]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[26]));
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[30]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xaa), coeff32[27]));
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0xff), coeff32[31]));
#endif

    a0 = _mm_add_epi32(a0, o0);
    a1 = _mm_add_epi32(a1, o0);

    b0 = _mm_add_epi32(a0, a2);
    b1 = _mm_add_epi32(a1, a3);
    b2 = _mm_sub_epi32(a0, a2);
    b3 = _mm_sub_epi32(a1, a3);

    a0 = b0;
    a1 = b1;
    a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
    a3 = _mm_shuffle_epi32(b2, 0x1b);

    b0 = _mm_add_epi32(a0, a4);
    b1 = _mm_add_epi32(a1, a5);
    b2 = _mm_add_epi32(a2, a6);
    b3 = _mm_add_epi32(a3, a7);
    b4 = _mm_sub_epi32(a0, a4);
    b5 = _mm_sub_epi32(a1, a5);
    b6 = _mm_sub_epi32(a2, a6);
    b7 = _mm_sub_epi32(a3, a7);

    a0 = _mm_sra_epi32(b0, s0);
    a1 = _mm_sra_epi32(b1, s0);
    a2 = _mm_sra_epi32(b2, s0);
    a3 = _mm_sra_epi32(b3, s0);
    a4 = _mm_sra_epi32(_mm_shuffle_epi32(b7, 0x1b), s0);
    a5 = _mm_sra_epi32(_mm_shuffle_epi32(b6, 0x1b), s0);
    a6 = _mm_sra_epi32(_mm_shuffle_epi32(b5, 0x1b), s0);
    a7 = _mm_sra_epi32(_mm_shuffle_epi32(b4, 0x1b), s0);

    x0 = _mm_packs_epi32(a0, a1);
    x1 = _mm_packs_epi32(a2, a3);
    x2 = _mm_packs_epi32(a4, a5);
    x3 = _mm_packs_epi32(a6, a7);

    _mm_storeu_si128((__m128i *)(dst+0x00), x0);
    _mm_storeu_si128((__m128i *)(dst+0x08), x1);
    _mm_storeu_si128((__m128i *)(dst+0x10), x2);
    _mm_storeu_si128((__m128i *)(dst+0x18), x3);

    src += src_stride;
    dst += dst_stride;
  }
  while (--numRows);
}

static void invTransform32Quarter(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift, EB_U32 numRows)
{
  __m128i s0 = _mm_cvtsi32_si128(shift);
  __m128i o0 = _mm_set1_epi32(1 << (shift - 1));
  const __m128i *coeff32 = (const __m128i *)EbHevccoeff_tbl2;

  do
  {
    __m128i x0, x1, x2, x3;
#ifndef SSSE3/// __SSSE3__
    __m128i y0, y1;
#endif
    __m128i a0, a1, a2, a3, a4, a5, a6, a7;
    __m128i b0, b1, b2, b3, b4, b5, b6, b7;
    x0 = _mm_loadu_si128((const __m128i *)(src+0x00)); // 00 01 02 03 04 05 06 07

#ifdef SSSE3/// __SSSE3__
    x0 = _mm_shuffle_epi8(x0, _mm_setr_epi8(0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15)); // 00 04 02 06 01 03 05 07

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]);

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[8]);

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x55), coeff32[9]);

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[16]);
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[20]));

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[21]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[22]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xaa), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x0, 0xff), coeff32[23]));
#else
    y0 = _mm_unpacklo_epi16(x0, x0); // 00 08 01 09 02 0a 03 0b
    y1 = _mm_unpackhi_epi16(x0, x0); // 04 0c 05 0d 06 0e 07 0f

    x0 = _mm_unpacklo_epi16(y0, y1); // 00 04 08 0c 01 05 09 0d
    x1 = _mm_unpackhi_epi16(y0, y1); // 02 06 0a 0e 03 07 0b 0f

    y0 = _mm_unpacklo_epi64(x0, x0); // 00 04 08 0c 10 14 18 1c
    y1 = _mm_unpacklo_epi64(x1, x1); // 02 06 0a 0e 12 16 1a 1e
    x2 = _mm_unpackhi_epi16(x0, x1); // 01 03 05 07 09 0b 0d 0f

    x0 = y0;
    x1 = y1;

    a0 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[0]); // 00 04

    a1 = _mm_madd_epi16(_mm_shuffle_epi32(x0, 0x00), coeff32[1]);

    a2 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[8]); // 02 06

    a3 = _mm_madd_epi16(_mm_shuffle_epi32(x1, 0x00), coeff32[9]);

    a4 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[16]); // 01 03
    a4 = _mm_add_epi32(a4, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[20])); // 05 07

    a5 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[17]);
    a5 = _mm_add_epi32(a5, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[21]));

    a6 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[18]);
    a6 = _mm_add_epi32(a6, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[22]));

    a7 = _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x00), coeff32[19]);
    a7 = _mm_add_epi32(a7, _mm_madd_epi16(_mm_shuffle_epi32(x2, 0x55), coeff32[23]));
#endif

    a0 = _mm_add_epi32(a0, o0);
    a1 = _mm_add_epi32(a1, o0);

    b0 = _mm_add_epi32(a0, a2);
    b1 = _mm_add_epi32(a1, a3);
    b2 = _mm_sub_epi32(a0, a2);
    b3 = _mm_sub_epi32(a1, a3);

    a0 = b0;
    a1 = b1;
    a2 = _mm_shuffle_epi32(b3, 0x1b); // 00011011
    a3 = _mm_shuffle_epi32(b2, 0x1b);

    b0 = _mm_add_epi32(a0, a4);
    b1 = _mm_add_epi32(a1, a5);
    b2 = _mm_add_epi32(a2, a6);
    b3 = _mm_add_epi32(a3, a7);
    b4 = _mm_sub_epi32(a0, a4);
    b5 = _mm_sub_epi32(a1, a5);
    b6 = _mm_sub_epi32(a2, a6);
    b7 = _mm_sub_epi32(a3, a7);

    a0 = _mm_sra_epi32(b0, s0);
    a1 = _mm_sra_epi32(b1, s0);
    a2 = _mm_sra_epi32(b2, s0);
    a3 = _mm_sra_epi32(b3, s0);
    a4 = _mm_sra_epi32(_mm_shuffle_epi32(b7, 0x1b), s0);
    a5 = _mm_sra_epi32(_mm_shuffle_epi32(b6, 0x1b), s0);
    a6 = _mm_sra_epi32(_mm_shuffle_epi32(b5, 0x1b), s0);
    a7 = _mm_sra_epi32(_mm_shuffle_epi32(b4, 0x1b), s0);

    x0 = _mm_packs_epi32(a0, a1);
    x1 = _mm_packs_epi32(a2, a3);
    x2 = _mm_packs_epi32(a4, a5);
    x3 = _mm_packs_epi32(a6, a7);

    _mm_storeu_si128((__m128i *)(dst+0x00), x0);
    _mm_storeu_si128((__m128i *)(dst+0x08), x1);
    _mm_storeu_si128((__m128i *)(dst+0x10), x2);
    _mm_storeu_si128((__m128i *)(dst+0x18), x3);

    src += src_stride;
    dst += dst_stride;
  }
  while (--numRows);
}

static void invTransform32Partial(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift, EB_U32 pattern)
{
  EB_U32 numRows = 32 - 2 * (pattern & 12);

  switch (pattern & 3)
  {
    case 3:
    invTransform32Quarter(src, src_stride, dst, dst_stride, shift, numRows);
      break;
    case 2:
    invTransform32Half(src, src_stride, dst, dst_stride, shift, numRows);
      break;
    case 1:
      invTransform32ThreeQuarter(src, src_stride, dst, dst_stride, shift, numRows);
      break;
    default:
    invTransform32(src, src_stride, dst, dst_stride, shift, numRows);
      break;
  }
}

// inverse 32x32 transform
void PFinvTransform32x32_SSSE3(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_S16 *intermediate, EB_U32 addshift)
{

  EB_U32 pattern = transpose32Check0s(src, src_stride, intermediate, 32);
  invTransform32Partial(intermediate, 32, dst, dst_stride, 7, pattern);

  pattern >>= 2;
  transpose32Partial(dst, dst_stride, intermediate, 32, pattern);
  invTransform32Partial(intermediate, 32, dst, dst_stride, 12-addshift, pattern);

}

void QuantizeInvQuantize4x4_SSE3(
    EB_S16          *coeff,
    const EB_U32     coeffStride,
    EB_S16          *quantCoeff,
    EB_S16          *reconCoeff,
    const EB_U32     qFunc,
    const EB_U32     q_offset,
    const EB_S32     shiftedQBits,
    const EB_S32     shiftedFFunc,
    const EB_S32     iq_offset,
    const EB_S32     shiftNum,
    const EB_U32     areaSize,
    EB_U32          *nonzerocoeff)
{
    int row;

    __m128i q = _mm_set1_epi16((EB_S16)qFunc);
    __m128i o = _mm_set1_epi32(q_offset);
    __m128i s = _mm_cvtsi32_si128(shiftedQBits);

    __m128i iq = _mm_set1_epi16((EB_S16)shiftedFFunc);
    __m128i io = _mm_set1_epi32(iq_offset);
    __m128i is = _mm_cvtsi32_si128(shiftNum);

    __m128i z = _mm_setzero_si128();

    (void)areaSize;

    row = 0;
    do
    {
        __m128i a0, a1;
        __m128i b0, b1;
        __m128i x0 = _mm_loadl_epi64((__m128i *)(coeff + coeffStride*row + 0));
        __m128i x1 = _mm_loadl_epi64((__m128i *)(coeff + coeffStride*row + coeffStride));
        __m128i y = _mm_unpacklo_epi64(x0, x1);
        __m128i x;

        x = _mm_abs_epi16(y);

        a0 = _mm_mullo_epi16(x, q);
        a1 = _mm_mulhi_epi16(x, q);

        b0 = _mm_unpacklo_epi16(a0, a1);
        b1 = _mm_unpackhi_epi16(a0, a1);

        b0 = _mm_add_epi32(b0, o);
        b1 = _mm_add_epi32(b1, o);

        b0 = _mm_sra_epi32(b0, s);
        b1 = _mm_sra_epi32(b1, s);

        x = _mm_packs_epi32(b0, b1);

        z = _mm_sub_epi16(z, _mm_cmpgt_epi16(x, _mm_setzero_si128()));

        x = _mm_sign_epi16(x, y);
        _mm_storel_epi64((__m128i *)(quantCoeff + coeffStride*row + 0), x);
        _mm_storel_epi64((__m128i *)(quantCoeff + coeffStride*row + coeffStride), _mm_srli_si128(x, 8));

		__m128i zer = _mm_setzero_si128();
		__m128i cmp = _mm_cmpeq_epi16(x, zer);
		int msk = _mm_movemask_epi8(cmp);

		if (msk != 0xFFFF)
		{
        a0 = _mm_mullo_epi16(x, iq);
        a1 = _mm_mulhi_epi16(x, iq);

        b0 = _mm_unpacklo_epi16(a0, a1);
        b1 = _mm_unpackhi_epi16(a0, a1);

        b0 = _mm_add_epi32(b0, io);
        b1 = _mm_add_epi32(b1, io);

        b0 = _mm_sra_epi32(b0, is);
        b1 = _mm_sra_epi32(b1, is);

        x = _mm_packs_epi32(b0, b1);
        _mm_storel_epi64((__m128i *)(reconCoeff + coeffStride*row + 0), x);
        _mm_storel_epi64((__m128i *)(reconCoeff + coeffStride*row + coeffStride), _mm_srli_si128(x, 8));
		}
		else{
			_mm_storel_epi64((__m128i *)(reconCoeff + coeffStride*row + 0), zer);
			_mm_storel_epi64((__m128i *)(reconCoeff + coeffStride*row + coeffStride), zer);
		}
        row += 2;
    } while (row < 4);

    z = _mm_sad_epu8(z, _mm_srli_si128(z, 7));
    *nonzerocoeff = _mm_cvtsi128_si32(z);
}
