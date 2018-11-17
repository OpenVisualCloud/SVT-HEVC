/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"

#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
// Note: _mm_extract_epi32 & _mm_extract_epi64 are SSE4 functions


EB_EXTERN void Luma4SampleEdgeDLFCore_SSSE3(
	EB_U8                 *edgeStartSample,
	EB_U32                 reconLumaPicStride,
	EB_BOOL                isVerticalEdge,
	EB_S32                 tc,
	EB_S32                 beta)
{
  
  __m128i x0, x1, x2, x3;
  __m128i d;
  __m128i e;
  __m128i d32;
  __m128i tcx, tcx2, tcx10;
  __m128i x3r;
  __m128i f0, f1, f2;
  __m128i y0, y1, y2, y3;
  __m128i c0, c1, c2, c3;
  __m128i d2;
  __m128i z0, z1;
  __m128i dl;
  __m128i a0, a1, b0, b1;

  int tm;
  int strongFilter;

  // Load sample values
  if (isVerticalEdge)
  {
    __m128i a0 = _mm_loadl_epi64((__m128i *)(edgeStartSample-4+0*reconLumaPicStride)); // 00 01 02 03 04 05 06 07
    __m128i a1 = _mm_loadl_epi64((__m128i *)(edgeStartSample-4+1*reconLumaPicStride)); // 10 11 12 13 14 15 16 17
    __m128i a2 = _mm_loadl_epi64((__m128i *)(edgeStartSample-4+2*reconLumaPicStride)); // 20 21 22 23 24 25 26 27
    __m128i a3 = _mm_loadl_epi64((__m128i *)(edgeStartSample-4+3*reconLumaPicStride)); // 30 31 32 33 34 35 36 37
    __m128i b0, b1;
    b0 = _mm_unpacklo_epi8(a0, a1); // 00 10 01 11 02 12 03 13 04 14 05 15 06 16 07 17
    b1 = _mm_unpacklo_epi8(a2, a3); // 20 30 21 31 22 32 23 33 24 34 25 35 26 36 27 37
    a0 = _mm_unpacklo_epi16(b0, b1); // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
    a1 = _mm_unpackhi_epi16(b0, b1); // 04 14 24 34 05 15 25 35 06 16 26 36 07 17 27 37
    a1 = _mm_shuffle_epi32(a1, 0x1b); // 07 17 27 37 06 16 26 36 05 15 25 35 04 14 24 34
    b0 = _mm_unpacklo_epi32(a0, a1); // 00 10 20 30 07 17 27 37 ...
    b1 = _mm_unpackhi_epi32(a0, a1);
    x0 = _mm_unpacklo_epi8(b0, _mm_setzero_si128());
    x1 = _mm_unpackhi_epi8(b0, _mm_setzero_si128());
    x2 = _mm_unpacklo_epi8(b1, _mm_setzero_si128());
    x3 = _mm_unpackhi_epi8(b1, _mm_setzero_si128());
  }
  else
  {
    __m128i a0 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSample-4*reconLumaPicStride));
    __m128i a1 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSample-3*reconLumaPicStride));
    __m128i a2 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSample-2*reconLumaPicStride));
    __m128i a3 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSample-1*reconLumaPicStride));
    __m128i a4 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSample+0*reconLumaPicStride));
    __m128i a5 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSample+1*reconLumaPicStride));
    __m128i a6 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSample+2*reconLumaPicStride));
    __m128i a7 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSample+3*reconLumaPicStride));
    a0 = _mm_unpacklo_epi32(a0, a7);
    a1 = _mm_unpacklo_epi32(a1, a6);
    a2 = _mm_unpacklo_epi32(a2, a5);
    a3 = _mm_unpacklo_epi32(a3, a4);
    x0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
    x1 = _mm_unpacklo_epi8(a1, _mm_setzero_si128());
    x2 = _mm_unpacklo_epi8(a2, _mm_setzero_si128());
    x3 = _mm_unpacklo_epi8(a3, _mm_setzero_si128());
  }

   

  
  // x0: p3 q3
  // x1: p2 q2
  // x2: p1 q1
  // x3: p0 q0
  
  // d: dp0 in lane 0, dp3 in lane 3, dq0 in lane 4, dq3 in lane 7
  
  d = _mm_sub_epi16(x1, x2);
  d = _mm_sub_epi16(d, x2);
  d = _mm_add_epi16(d, x3);
  
  // Absolute value
  d = _mm_abs_epi16(d);
  
  // Need sum of lanes 0, 3, 4, 7
  // e: d0=dp0+dq0 in lanes 0 and 4, d3=dp3+dq3 in lanes 3 and 7
  e = _mm_add_epi16(d, _mm_shuffle_epi32(d, 0x4e)); // 0x4e = 01001110
  if (_mm_extract_epi16(e, 0) + _mm_extract_epi16(e, 3) >= beta)
  {
    return;
  }
  
  d32 = d;
  d32 = _mm_shufflelo_epi16(d32, 0xcc); // 0xcc = 11001100
  d32 = _mm_shufflehi_epi16(d32, 0xcc);
  d32 = _mm_madd_epi16(d32, _mm_set1_epi16(1));
  // d: dp=dp0+dp3 in lanes 0 and 1, dq=dq0+dq3 in lanes 2 and 3 (note: 32-bit lanes)
  
  tm = 0;
  tm += tc;
  tm += tc << 16;
  tcx = _mm_cvtsi32_si128(tm);
  tcx = _mm_unpacklo_epi16(tcx, tcx);
  tcx = _mm_shuffle_epi32(tcx, 0x50); // 01010000
  
  x3r = _mm_shuffle_epi32(x3, 0x4e); // 0x4e = 01001110 (swap 64-bit lanes)

  f0 = _mm_max_epi16(_mm_sub_epi16(x0, x3), _mm_sub_epi16(x3, x0));
  f0 = _mm_add_epi16(f0, _mm_shuffle_epi32(f0, 0x4e));
  f0 = _mm_cmplt_epi16(f0, _mm_set1_epi16((short)(beta>>3)));
  
  f1 = _mm_max_epi16(_mm_sub_epi16(x3, x3r), _mm_sub_epi16(x3r, x3));
  f1 = _mm_cmplt_epi16(f1, _mm_set1_epi16((short)((5*tc+1)>>1)));
  
  f2 = _mm_cmplt_epi16(_mm_add_epi16(e, e), _mm_set1_epi16((short)(beta >> 2)));
  
  f0 = _mm_and_si128(f0, f1);
  f0 = _mm_and_si128(f0, f2);
  
  
  y0 = x0;
  y1 = x1;
  y2 = x2;
  y3 = x3;
  strongFilter = (_mm_movemask_epi8(f0) & 0xc3) == 0xc3;
  
  if (strongFilter)
  {
    tcx2 = _mm_add_epi16(tcx, tcx);
    
    // q0 = p1 + 2p0 + 2q0 + 2q1 + q2
    // q1 = p0 + q0 + q1 + q2
    // q2 = p0 + q0 + q1 + 3q2 + 2q3
    
    c0 = _mm_add_epi16(y0, y1); // q2 + q3
    c1 = _mm_add_epi16(y1, y2); // q1 + q2
    c2 = _mm_add_epi16(y2, y3); // q0 + q1
    c3 = _mm_add_epi16(y3, x3r); // p0 + q0

    // y1 = c3 + c1 + 2c0
    // y2 = c3 + c1
    // y3 = c3 + c1 + c2 + rev(c2)

    y2 = _mm_add_epi16(c3, c1);
    c2 = _mm_add_epi16(c2, _mm_shuffle_epi32(c2, 0x4e));
    
    y1 = _mm_add_epi16(y2, c0);
    y1 = _mm_add_epi16(y1, c0);
    y3 = _mm_add_epi16(y2, c2);
    
    y1 = _mm_add_epi16(y1, _mm_set1_epi16(4));
    y2 = _mm_add_epi16(y2, _mm_set1_epi16(2));
    y3 = _mm_add_epi16(y3, _mm_set1_epi16(4));
    
    y1 = _mm_srai_epi16(y1, 3);
    y2 = _mm_srai_epi16(y2, 2);
    y3 = _mm_srai_epi16(y3, 3);
    
    y1 = _mm_max_epi16(y1, _mm_sub_epi16(x1, tcx2));
    y1 = _mm_min_epi16(y1, _mm_add_epi16(x1, tcx2));
    y2 = _mm_max_epi16(y2, _mm_sub_epi16(x2, tcx2));
    y2 = _mm_min_epi16(y2, _mm_add_epi16(x2, tcx2));
    y3 = _mm_max_epi16(y3, _mm_sub_epi16(x3, tcx2));
    y3 = _mm_min_epi16(y3, _mm_add_epi16(x3, tcx2));
  }
  else
  {
    dl = _mm_mullo_epi16(x3, _mm_set1_epi16(9));
    dl = _mm_sub_epi16(dl, _mm_mullo_epi16(x2, _mm_set1_epi16(3)));
    dl = _mm_sub_epi16(_mm_shuffle_epi32(dl, 0x4e), dl);
    dl = _mm_add_epi16(dl, _mm_setr_epi16(8, 8, 8, 8, 7, 7, 7, 7));
    dl = _mm_srai_epi16(dl, 4);
    
    tcx10 = _mm_mullo_epi16(tcx, _mm_set1_epi16(10));
    
    tcx = _mm_and_si128(tcx, _mm_cmplt_epi16(dl, tcx10));
    tcx = _mm_and_si128(tcx, _mm_cmpgt_epi16(dl, _mm_sub_epi16(_mm_setzero_si128(), tcx10)));
    
    dl = _mm_min_epi16(dl, tcx);
    dl = _mm_max_epi16(dl, _mm_sub_epi16(_mm_setzero_si128(), tcx));
    
    y3 = _mm_add_epi16(y3, dl);
    

    tcx = _mm_srai_epi16(tcx, 1);

    tcx = _mm_and_si128(tcx, _mm_cmplt_epi32(d32, _mm_set1_epi32(3*beta>>4))); // side threshold
    
    d2 = _mm_sub_epi16(_mm_avg_epu16(x3, x1), x2);
    d2 = _mm_add_epi16(d2, dl);
    d2 = _mm_srai_epi16(d2, 1);
    d2 = _mm_min_epi16(d2, tcx);
    d2 = _mm_max_epi16(d2, _mm_sub_epi16(_mm_setzero_si128(), tcx));
    
    y2 = _mm_add_epi16(y2, d2);
  }
  
  // Store sample values
  z0 = _mm_packus_epi16(y0, y1);
  z1 = _mm_packus_epi16(y2, y3);
  if (isVerticalEdge)
  {
    a0 = _mm_unpacklo_epi32(z0, z1); // 00 10 20 30 02 12 22 32 07 17 27 37 05 15 25 35
    a1 = _mm_unpackhi_epi32(z0, z1); // 01 11 21 31 03 13 23 33 06 16 26 36 04 14 24 34
    b0 = _mm_unpacklo_epi32(a0, a1); // 00 10 20 30 01 11 21 31 02 12 22 32 03 13 23 33
    b1 = _mm_unpackhi_epi32(a0, a1); // 07 17 27 37 ...
    b1 = _mm_shuffle_epi32(b1, 0x1b); // 04 14 24 34
    a0 = _mm_unpacklo_epi8(b0, b1); // 00 04 10 14
    a1 = _mm_unpackhi_epi8(b0, b1); // 02 06 12 16
    b0 = _mm_unpacklo_epi8(a0, a1); // 00 02 04 06
    b1 = _mm_unpackhi_epi8(a0, a1); //
    a0 = _mm_unpacklo_epi8(b0, b1);
    a1 = _mm_unpackhi_epi8(b0, b1);
    *(EB_U64 *)(edgeStartSample-4+0*reconLumaPicStride) = _mm_extract_epi64(a0, 0);
    *(EB_U64 *)(edgeStartSample-4+1*reconLumaPicStride) = _mm_extract_epi64(a0, 1);
    *(EB_U64 *)(edgeStartSample-4+2*reconLumaPicStride) = _mm_extract_epi64(a1, 0);
    *(EB_U64 *)(edgeStartSample-4+3*reconLumaPicStride) = _mm_extract_epi64(a1, 1);
  }
  else
  {
    *(EB_U32 *)(edgeStartSample-3*reconLumaPicStride) = _mm_extract_epi32(z0, 2);
    *(EB_U32 *)(edgeStartSample-2*reconLumaPicStride) = _mm_extract_epi32(z1, 0);
    *(EB_U32 *)(edgeStartSample-1*reconLumaPicStride) = _mm_extract_epi32(z1, 2);
    *(EB_U32 *)(edgeStartSample+0*reconLumaPicStride) = _mm_extract_epi32(z1, 3);
    *(EB_U32 *)(edgeStartSample+1*reconLumaPicStride) = _mm_extract_epi32(z1, 1);
    *(EB_U32 *)(edgeStartSample+2*reconLumaPicStride) = _mm_extract_epi32(z0, 3);
  }

}

EB_EXTERN void Chroma2SampleEdgeDLFCore_SSSE3(
	EB_BYTE                edgeStartSampleCb,
	EB_BYTE                edgeStartSampleCr,
	EB_U32                 reconChromaPicStride,
	EB_BOOL                isVerticalEdge,
	EB_U8                  cbTc,
	EB_U8                  crTc)
{
  __m128i x0, x1, x2, x3;
  __m128i a0, a1, a2, a3, a4, a5, a6, a7;
  __m128i lim;

  lim = _mm_cvtsi32_si128(cbTc + (crTc << 16));
  lim = _mm_unpacklo_epi16(lim, lim);

  if (isVerticalEdge)
  {
    a0 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCb+0*reconChromaPicStride-2));
    a1 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCb+1*reconChromaPicStride-2));
    a2 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCr+0*reconChromaPicStride-2));
    a3 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCr+1*reconChromaPicStride-2));

    a0 = _mm_unpacklo_epi16(a0, a2);
    a1 = _mm_unpacklo_epi16(a1, a3);
    
    a0 = _mm_unpacklo_epi8(a0, a1);
    
    a0 = _mm_shufflelo_epi16(a0, 0xd8); // 11011000
    a0 = _mm_shufflehi_epi16(a0, 0xd8); // 11011000
    
    x0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
    x1 = _mm_srli_si128(x0, 8);
    x2 = _mm_unpackhi_epi8(a0, _mm_setzero_si128());
    x3 = _mm_srli_si128(x2, 8);

    x0 = _mm_sub_epi16(x0, x3);
    x0 = _mm_srai_epi16(x0, 2);
    x3 = _mm_sub_epi16(x2, x1);
    x0 = _mm_add_epi16(x0, x3);
    x0 = _mm_add_epi16(x0, _mm_set1_epi16(1));
    x0 = _mm_srai_epi16(x0, 1);
    x0 = _mm_min_epi16(x0, lim);
    x0 = _mm_max_epi16(x0, _mm_sub_epi16(_mm_setzero_si128(), lim));
    
    
    x1 = _mm_add_epi16(x1, x0);
    x2 = _mm_sub_epi16(x2, x0);
    x1 = _mm_unpacklo_epi16(x1, x2);
    x1 = _mm_packus_epi16(x1, x1);
    
    *(EB_U16 *)(edgeStartSampleCb-1+0*reconChromaPicStride) = (EB_U16)(_mm_extract_epi16(x1, 0));
    *(EB_U16 *)(edgeStartSampleCb-1+1*reconChromaPicStride) = (EB_U16)(_mm_extract_epi16(x1, 1));
    *(EB_U16 *)(edgeStartSampleCr-1+0*reconChromaPicStride) = (EB_U16)(_mm_extract_epi16(x1, 2));
    *(EB_U16 *)(edgeStartSampleCr-1+1*reconChromaPicStride) = (EB_U16)(_mm_extract_epi16(x1, 3));
  }
  else
  {
    a0 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCb-2*reconChromaPicStride));
    a1 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCb-1*reconChromaPicStride));
    a2 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCb+0*reconChromaPicStride));
    a3 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCb+1*reconChromaPicStride));
    
    a4 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCr-2*reconChromaPicStride));
    a0 = _mm_unpacklo_epi16(a0, a4);
    a5 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCr-1*reconChromaPicStride));
    a1 = _mm_unpacklo_epi16(a1, a5);
    a6 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCr+0*reconChromaPicStride));
    a2 = _mm_unpacklo_epi16(a2, a6);
    a7 = _mm_cvtsi32_si128(*(EB_U32 *)(edgeStartSampleCr+1*reconChromaPicStride));
    a3 = _mm_unpacklo_epi16(a3, a7);
    
    x0 = _mm_unpacklo_epi8(a0, _mm_setzero_si128());
    x1 = _mm_unpacklo_epi8(a1, _mm_setzero_si128());
    x2 = _mm_unpacklo_epi8(a2, _mm_setzero_si128());
    x3 = _mm_unpacklo_epi8(a3, _mm_setzero_si128());
    
    x0 = _mm_sub_epi16(x0, x3);
    x3 = _mm_sub_epi16(x2, x1);
    x0 = _mm_srai_epi16(x0, 2);
    x0 = _mm_add_epi16(x0, x3);
    x0 = _mm_add_epi16(x0, _mm_set1_epi16(1));
    x0 = _mm_srai_epi16(x0, 1);
    x0 = _mm_min_epi16(x0, lim);
    x0 = _mm_max_epi16(x0, _mm_sub_epi16(_mm_setzero_si128(), lim));
    
    
    x1 = _mm_add_epi16(x1, x0);
    x2 = _mm_sub_epi16(x2, x0);
    
    x1 = _mm_packus_epi16(x1, x2);
    *(EB_U16 *)(edgeStartSampleCb-1*reconChromaPicStride) = (EB_U16)(_mm_extract_epi16(x1, 0));
    *(EB_U16 *)(edgeStartSampleCb+0*reconChromaPicStride) = (EB_U16)(_mm_extract_epi16(x1, 4));
    *(EB_U16 *)(edgeStartSampleCr-1*reconChromaPicStride) = (EB_U16)(_mm_extract_epi16(x1, 1));
    *(EB_U16 *)(edgeStartSampleCr+0*reconChromaPicStride) = (EB_U16)(_mm_extract_epi16(x1, 5));
  }
}


