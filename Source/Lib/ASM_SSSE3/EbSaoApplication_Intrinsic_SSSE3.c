/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
//#include <x86intrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>

#include "EbSaoApplication_SSSE3.h"

static __m128i _mm_loadh_epi64(__m128i x, __m128i *p)
{
  return _mm_castpd_si128(_mm_loadh_pd(_mm_castsi128_pd(x), (double *)p));
}

static void _mm_storeh_epi64(__m128i * p, __m128i x)
{
  _mm_storeh_pd((double *)p, _mm_castsi128_pd(x));
}

EB_ERRORTYPE SAOApplyBO_BT_SSSE3(
                                EB_U8                           *reconSamplePtr,
                                EB_U32                           reconStride,
                                EB_U32                           saoBandPosition,
                                EB_S8                           *saoOffsetPtr,
                                EB_U32                           lcuHeight,
                                EB_U32                           lcuWidth)
{
  EB_S32 rowCount, colCount;
  __m128i band;
  __m128i offsets;
  EB_U8 *ptr;
  
  band = _mm_cvtsi32_si128(saoBandPosition);
  band = _mm_unpacklo_epi8(band, band);
  band = _mm_unpacklo_epi16(band, band);
  band = _mm_shuffle_epi32(band, 0x00);
  
  offsets = _mm_cvtsi32_si128(*(EB_U32 *)saoOffsetPtr);
  
  if (lcuWidth & 8)
  {
    rowCount = lcuHeight;
    ptr = reconSamplePtr;
    
    do
    {
      __m128i x0, x1;
      x0 = _mm_loadl_epi64(    (__m128i *)ptr); ptr += reconStride;
      x0 = _mm_loadh_epi64(x0, (__m128i *)ptr); ptr -= reconStride;
      
      x1 = _mm_srli_epi16(x0, 3);
      x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
      x1 = _mm_sub_epi8(x1, band);
      x1 = _mm_and_si128(x1, _mm_set1_epi8(31));
      x0 = _mm_adds_epi8(x0, _mm_and_si128(_mm_shuffle_epi8(offsets, x1), _mm_cmplt_epi8(x1, _mm_set1_epi8(4))));
      x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
      
      _mm_storel_epi64((__m128i *)ptr, x0); ptr += reconStride;
      _mm_storeh_epi64((__m128i *)ptr, x0); ptr += reconStride;
      
      rowCount -= 2;
    }
    while (rowCount > 0);
    
    lcuWidth -= 8;
    if (lcuWidth == 0)
    {
      return EB_ErrorNone;
    }
    reconSamplePtr += 8;
  }
  
  colCount = lcuWidth;
  do
  {
    ptr = reconSamplePtr;
    rowCount = lcuHeight;
    do
    {
      __m128i x0, x1;
      
      x0 = _mm_loadu_si128((__m128i *)ptr);
      
      x1 = _mm_srli_epi16(x0, 3);
      x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
      x1 = _mm_sub_epi8(x1, band);
      x1 = _mm_and_si128(x1, _mm_set1_epi8(31));
      x0 = _mm_adds_epi8(x0, _mm_and_si128(_mm_shuffle_epi8(offsets, x1), _mm_cmplt_epi8(x1, _mm_set1_epi8(4))));
      x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
      
      _mm_storeu_si128((__m128i *)ptr, x0);
      
      ptr += reconStride;
    }
    while (--rowCount);
    
    reconSamplePtr += 16;
    colCount -= 16;
  }
  while (colCount > 0);
  
  return EB_ErrorNone;
}

static __m128i SAOEdgeProcess(__m128i x0, __m128i x1, __m128i x2, __m128i offsets)
{
  __m128i c0, c1, c2;
  c1 = _mm_sub_epi8(_mm_cmplt_epi8(x0, x1), _mm_cmpgt_epi8(x0, x1));
  c2 = _mm_sub_epi8(_mm_cmplt_epi8(x0, x2), _mm_cmpgt_epi8(x0, x2));
  c0 = _mm_add_epi8(c1, c2);
  
  x0 = _mm_adds_epi8(x0, _mm_shuffle_epi8(offsets, _mm_add_epi8(c0, _mm_set1_epi8(2))));

  return x0;
}

EB_ERRORTYPE SAOApplyEO_0_BT_SSSE3(
                          EB_U8                           *reconSamplePtr,
                          EB_U32                           reconStride,
                          EB_U8                           *temporalBufferLeft,
                          EB_S8                           *saoOffsetPtr,
                          EB_U32                           lcuHeight,
                          EB_U32                           lcuWidth  )
{
  __m128i offsets;
  EB_S32 rowCount, colCount, rowCountInner;
  
  offsets = _mm_loadl_epi64((__m128i *)saoOffsetPtr); // 0 1 x 2 3 x ...
  
  rowCount = lcuHeight;
  do
  {
    __m128i left;
    EB_BYTE basePtr = reconSamplePtr;
    
    left = _mm_loadu_si128((__m128i *)temporalBufferLeft); temporalBufferLeft += 16;
    left = _mm_xor_si128(left, _mm_set1_epi8(-128));
    
    colCount = lcuWidth;
    
    if (colCount & 8)
    {
      EB_BYTE ptr = basePtr;
      EB_U32 shift;

      rowCountInner = 16;
      if (rowCountInner > rowCount)
      {
        rowCountInner = rowCount;
      }
      shift = 16 - rowCountInner;

      do
      {
        __m128i x0, x1, x2;
        
        x0 = _mm_loadl_epi64((__m128i *)ptr);
        x1 = _mm_loadl_epi64((__m128i *)(ptr+1));
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));
        x2 = _mm_slli_si128(x0, 1);
        x2 = _mm_or_si128(x2, _mm_and_si128(left, _mm_setr_epi8(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)));
        left = _mm_srli_si128(left, 1);
        left = _mm_or_si128(left, _mm_and_si128(_mm_slli_si128(x0, 8), _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1)));
        
        x0 = SAOEdgeProcess(x0, x1, x2, offsets);
        
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        _mm_storel_epi64((__m128i *)ptr, x0);
        ptr += reconStride;
      }
      while (--rowCountInner);

      colCount -= 8;
      basePtr += 8;
      
      if (shift & 8)
      {
        left = _mm_srli_si128(left, 8);
      }
      if (shift & 4)
      {
        left = _mm_srli_si128(left, 4);
      }
    }
    
    while (colCount > 0)
    {
      EB_BYTE ptr = basePtr;
      EB_U32 shift;
      
      rowCountInner = 16;
      if (rowCountInner > rowCount)
      {
        rowCountInner = rowCount;
      }
      shift = 16 - rowCountInner;
      
      do
      {
        __m128i x0, x1, x2;
        
        x0 = _mm_loadu_si128((__m128i *)ptr);
        x1 = _mm_loadu_si128((__m128i *)(ptr+1));
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));
        x2 = _mm_slli_si128(x0, 1);
        x2 = _mm_or_si128(x2, _mm_and_si128(left, _mm_setr_epi8(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)));
        left = _mm_srli_si128(left, 1);
        left = _mm_or_si128(left, _mm_and_si128(x0, _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1)));
        
        x0 = SAOEdgeProcess(x0, x1, x2, offsets);

        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        _mm_storeu_si128((__m128i *)ptr, x0);
        ptr += reconStride;
      }
      while (--rowCountInner);
      
      colCount -= 16;
      basePtr += 16;
      
      if (shift & 8)
      {
        left = _mm_srli_si128(left, 8);
      }
      if (shift & 4)
      {
        left = _mm_srli_si128(left, 4);
      }
    }
    
    rowCount -= 16;
    reconSamplePtr += 16 * reconStride;
  }
  while (rowCount > 0);
  
  return EB_ErrorNone;
}

EB_ERRORTYPE SAOApplyEO_90_BT_SSSE3(
                           EB_U8                           *reconSamplePtr,
                           EB_U32                           reconStride,
                           EB_U8                           *temporalBufferUpper,
                           EB_S8                           *saoOffsetPtr,
                           EB_U32                           lcuHeight,
                           EB_U32                           lcuWidth  )
{
  EB_BYTE ptr;
  EB_S32 rowCount, colCount;
  __m128i offsets;
  __m128i x0, x1, x2;

  offsets = _mm_loadl_epi64((__m128i *)saoOffsetPtr); // 0 1 x 2 3 x ...

  if (lcuWidth & 8)
  {
    ptr = reconSamplePtr;
    rowCount = lcuHeight;
    
    x2 = _mm_loadl_epi64((__m128i *)temporalBufferUpper); temporalBufferUpper += 8;
    x2 = _mm_loadh_epi64(x2, (__m128i *)ptr); ptr += reconStride;
    x2 = _mm_xor_si128(x2, _mm_set1_epi8(-128));
    do
    {
      x1 = _mm_loadl_epi64((__m128i *)ptr);
      x1 = _mm_loadh_epi64(x1, (__m128i *)(ptr + reconStride));
      x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));
      
      x0 = _mm_or_si128(_mm_slli_si128(x1, 8), _mm_srli_si128(x2, 8));
     
      x0 = SAOEdgeProcess(x0, x1, x2, offsets);

      x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
      
      _mm_storel_epi64((__m128i *)(ptr - reconStride), x0);
      _mm_storeh_epi64((__m128i *)ptr, x0);
      ptr += reconStride;
      ptr += reconStride;
      
      x2 = x1;
      rowCount -= 2;
    }
    while (rowCount > 0);
    
    lcuWidth -= 8;
    if (lcuWidth == 0)
    {
      return EB_ErrorNone;
    }
    reconSamplePtr += 8;
  }
  
  colCount = lcuWidth;
  do
  {
    ptr = reconSamplePtr;
    
    x2 = _mm_loadu_si128((__m128i *)temporalBufferUpper); temporalBufferUpper += 16;
    x0 = _mm_loadu_si128((__m128i *)ptr);
    x2 = _mm_xor_si128(x2, _mm_set1_epi8(-128));
    x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));

    rowCount = lcuHeight;
    do
    {
      __m128i r0;
      x1 = _mm_loadu_si128((__m128i *)(ptr + reconStride));
      x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));

      r0 = SAOEdgeProcess(x0, x1, x2, offsets);
      r0 = _mm_xor_si128(r0, _mm_set1_epi8(-128));

      _mm_storeu_si128((__m128i *)ptr, r0);
      ptr += reconStride;
      
      x2 = x0;
      x0 = x1;
    }
    while (--rowCount);
    
    colCount -= 16;
    reconSamplePtr += 16;
  }
  while (colCount > 0);

  return EB_ErrorNone;
}

EB_ERRORTYPE SAOApplyEO_135_BT_SSSE3(
                                    EB_U8                           *reconSamplePtr,
                                    EB_U32                           reconStride,
                                    EB_U8                           *temporalBufferLeft,
                                    EB_U8                           *temporalBufferUpper,
                                    EB_S8                           *saoOffsetPtr,
                                    EB_U32                           lcuHeight,         
                                    EB_U32                           lcuWidth  )
{
  __m128i offsets;
  EB_S32 rowCount, colCount, rowCountInner;
  EB_U32 /*EB_S32*/ colIdx;
  
  EB_U8 bufferAbove[MAX_LCU_SIZE];
  
  colIdx = 0;

  temporalBufferUpper--;
  
  if (lcuWidth & 8)
  {
    __m128i x0 = _mm_loadl_epi64((__m128i *)temporalBufferUpper);
    x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
    _mm_storel_epi64((__m128i *)bufferAbove, x0);
    colIdx += 8;
  }
  while (colIdx < lcuWidth)
  {
    __m128i x0 = _mm_loadu_si128((__m128i *)(temporalBufferUpper + colIdx));
    x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
    _mm_storeu_si128((__m128i *)(bufferAbove + colIdx), x0);
    colIdx += 16;
  }
  
  offsets = _mm_loadl_epi64((__m128i *)saoOffsetPtr); // 0 1 x 2 3 x ...
  
  rowCount = lcuHeight;
  do
  {
    __m128i left, up;
    EB_BYTE basePtr = reconSamplePtr;
    EB_BYTE upPtr = bufferAbove;
    
    left = _mm_loadu_si128((__m128i *)temporalBufferLeft); temporalBufferLeft += 16;
    left = _mm_xor_si128(left, _mm_set1_epi8(-128));
    
    colCount = lcuWidth;
    
    if (colCount & 8)
    {
      EB_BYTE ptr = basePtr;
      EB_U32 shift;
      
      rowCountInner = 16;
      if (rowCountInner > rowCount)
      {
        rowCountInner = rowCount;
      }
      shift = 16 - rowCountInner;
      
      up = _mm_loadl_epi64((__m128i *)upPtr);

      do
      {
        __m128i x0, x1, x2;
        
        x0 = _mm_loadl_epi64((__m128i *)ptr);
        x1 = _mm_loadl_epi64((__m128i *)(ptr+1+reconStride));
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));
        
        x2 = up;
        
        up = _mm_slli_si128(x0, 1);
        up = _mm_or_si128(up, _mm_and_si128(left, _mm_setr_epi8(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)));
        
        left = _mm_srli_si128(left, 1);
        left = _mm_or_si128(left, _mm_and_si128(_mm_slli_si128(x0,8), _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1)));
        
        x0 = SAOEdgeProcess(x0, x1, x2, offsets);
        
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        _mm_storel_epi64((__m128i *)ptr, x0);
        ptr += reconStride;
      }
      while (--rowCountInner);
      
      _mm_storel_epi64((__m128i *)upPtr, up);

      colCount -= 8;
      basePtr += 8;
      upPtr += 8;
      
      if (shift & 8)
      {
        left = _mm_srli_si128(left, 8);
      }
      if (shift & 4)
      {
        left = _mm_srli_si128(left, 4);
      }
    }
    
    while (colCount > 0)
    {
      EB_BYTE ptr = basePtr;
      EB_U32 shift;
      
      rowCountInner = 16;
      if (rowCountInner > rowCount)
      {
        rowCountInner = rowCount;
      }
      shift = 16 - rowCountInner;
      
      up = _mm_loadu_si128((__m128i *)upPtr);
      
      do
      {
        __m128i x0, x1, x2;
        
        x0 = _mm_loadu_si128((__m128i *)ptr);
        x1 = _mm_loadu_si128((__m128i *)(ptr+1+reconStride));
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));
        
        x2 = up;
        
        up = _mm_slli_si128(x0, 1);
        up = _mm_or_si128(up, _mm_and_si128(left, _mm_setr_epi8(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)));
        
        left = _mm_srli_si128(left, 1);
        left = _mm_or_si128(left, _mm_and_si128(x0, _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1)));
        
        x0 = SAOEdgeProcess(x0, x1, x2, offsets);
        
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        _mm_storeu_si128((__m128i *)ptr, x0);
        ptr += reconStride;
      }
      while (--rowCountInner);
      
      _mm_storeu_si128((__m128i *)upPtr, up);
      
      colCount -= 16;
      basePtr += 16;
      upPtr += 16;
      
      if (shift & 8)
      {
        left = _mm_srli_si128(left, 8);
      }
      if (shift & 4)
      {
        left = _mm_srli_si128(left, 4);
      }
    }
    
    rowCount -= 16;
    reconSamplePtr += 16 * reconStride;
  }
  while (rowCount > 0);
  
  return EB_ErrorNone;
}

EB_ERRORTYPE SAOApplyEO_45_BT_SSSE3(
                                    EB_U8                           *reconSamplePtr,
                                    EB_U32                           reconStride,
                                    EB_U8                           *temporalBufferLeft,
                                    EB_U8                           *temporalBufferUpper,
                                    EB_S8                           *saoOffsetPtr,
                                    EB_U32                           lcuHeight,
                                    EB_U32                           lcuWidth  )
{
  __m128i offsets;
  EB_S32 rowCount, colCount, rowCountInner;
  EB_U32 /*EB_S32*/ colIdx;
  __m128i down;

  EB_U8 bufferAbove[MAX_LCU_SIZE];
  
  colIdx = 0;


  temporalBufferUpper++;
  temporalBufferLeft++;
  
  if (lcuWidth & 8)
  {
    __m128i x0 = _mm_loadl_epi64((__m128i *)temporalBufferUpper);
    x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
    _mm_storel_epi64((__m128i *)bufferAbove, x0);
    colIdx += 8;
  }
  while (colIdx < lcuWidth)
  {
    __m128i x0 = _mm_loadu_si128((__m128i *)(temporalBufferUpper + colIdx));
    x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
    _mm_storeu_si128((__m128i *)(bufferAbove + colIdx), x0);
    colIdx += 16;
  }
  
  offsets = _mm_loadl_epi64((__m128i *)saoOffsetPtr); // 0 1 x 2 3 x ...
  
  rowCount = lcuHeight;
  do
  {
    __m128i left, up;
    EB_BYTE basePtr = reconSamplePtr;
    EB_BYTE upPtr = bufferAbove;
    
    left = _mm_loadu_si128((__m128i *)temporalBufferLeft); temporalBufferLeft += 16;
    left = _mm_xor_si128(left, _mm_set1_epi8(-128));
    
    colCount = lcuWidth;
    
    if (colCount & 8)
    {
      EB_BYTE ptr = basePtr;
      EB_U32 shift;
      
      rowCountInner = 16;
      if (rowCountInner > rowCount)
      {
        rowCountInner = rowCount;
      }
      shift = 16 - rowCountInner;
      
      up = _mm_loadl_epi64((__m128i *)upPtr);
      down = _mm_loadl_epi64((__m128i *)ptr);
      down = _mm_xor_si128(down, _mm_set1_epi8(-128));

      do
      {
        __m128i x0, x1, x2;
        
        x0 = down;
        x1 = _mm_loadl_epi64((__m128i *)(ptr+reconStride));
        x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));
        down = x1;
        
        x2 = up;
        
        x1 = _mm_slli_si128(x1, 1);
        x1 = _mm_or_si128(x1, _mm_and_si128(left, _mm_setr_epi8(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)));
        
        left = _mm_srli_si128(left, 1);
        left = _mm_or_si128(left, _mm_and_si128(_mm_slli_si128(down,8), _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1)));
        
        up = _mm_loadl_epi64((__m128i *)(ptr + 1));
        up = _mm_xor_si128(up, _mm_set1_epi8(-128));
        
        x0 = SAOEdgeProcess(x0, x1, x2, offsets);
        
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        _mm_storel_epi64((__m128i *)ptr, x0);
        ptr += reconStride;
      }
      while (--rowCountInner);
      
      _mm_storel_epi64((__m128i *)upPtr, up);
      
      colCount -= 8;
      basePtr += 8;
      upPtr += 8;
      
      if (shift & 8)
      {
        left = _mm_srli_si128(left, 8);
      }
      if (shift & 4)
      {
        left = _mm_srli_si128(left, 4);
      }
    }
    
    while (colCount > 0)
    {
      EB_BYTE ptr = basePtr;
      EB_U32 shift;
      
      rowCountInner = 16;
      if (rowCountInner > rowCount)
      {
        rowCountInner = rowCount;
      }
      shift = 16 - rowCountInner;
      
      up = _mm_loadu_si128((__m128i *)upPtr);
      down = _mm_loadu_si128((__m128i *)ptr);
      down = _mm_xor_si128(down, _mm_set1_epi8(-128));
      
      do
      {
        __m128i x0, x1, x2;
        
        x0 = down;
        x1 = _mm_loadu_si128((__m128i *)(ptr+reconStride));
        x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));
        down = x1;
        
        x2 = up;
        
        x1 = _mm_slli_si128(x1, 1);
        x1 = _mm_or_si128(x1, _mm_and_si128(left, _mm_setr_epi8(-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)));
        
        left = _mm_srli_si128(left, 1);
        left = _mm_or_si128(left, _mm_and_si128(down, _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1)));

        up = _mm_loadu_si128((__m128i *)(ptr + 1));
        up = _mm_xor_si128(up, _mm_set1_epi8(-128));
        
        x0 = SAOEdgeProcess(x0, x1, x2, offsets);
        
        x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
        _mm_storeu_si128((__m128i *)ptr, x0);
        ptr += reconStride;
      }
      while (--rowCountInner);
      
      _mm_storeu_si128((__m128i *)upPtr, up);
      
      colCount -= 16;
      basePtr += 16;
      upPtr += 16;
      
      if (shift & 8)
      {
        left = _mm_srli_si128(left, 8);
      }
      if (shift & 4)
      {
        left = _mm_srli_si128(left, 4);
      }
    }
    
    rowCount -= 16;
    reconSamplePtr += 16 * reconStride;
  }
  while (rowCount > 0);
  
  return EB_ErrorNone;
}


