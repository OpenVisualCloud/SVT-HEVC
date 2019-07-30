/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include <emmintrin.h>
#include "smmintrin.h"

#define SAO_EO_TYPES 4
#define SAO_EO_CATEGORIES 4
#define SAO_BO_INTERVALS 32


static EB_S16/*EB_U16*/ maskTable[8][8] =
{
  { -1, 0, 0, 0, 0, 0, 0, 0 },
  { -1, -1, 0, 0, 0, 0, 0, 0 },
  { -1, -1, -1, 0, 0, 0, 0, 0 },
  { -1, -1, -1, -1, 0, 0, 0, 0 },
  { -1, -1, -1, -1, -1, 0, 0, 0 },
  { -1, -1, -1, -1, -1, -1, 0, 0 },
  { -1, -1, -1, -1, -1, -1, -1, 0 },
  { -1, -1, -1, -1, -1, -1, -1, -1 }
};


static void countEdge(__m128i *eoDiff, __m128i *eoCount, EB_BYTE ptr, EB_S32 offset, __m128i x0, __m128i diff, __m128i mask)
{
  __m128i x1, x2;
  __m128i c1, c2;
  __m128i cat, select;
  
  x1 = _mm_loadu_si128((__m128i *)(ptr + offset));
  x2 = _mm_loadu_si128((__m128i *)(ptr - offset));
  x1 = _mm_xor_si128(x1, _mm_set1_epi8(-128));
  x2 = _mm_xor_si128(x2, _mm_set1_epi8(-128));
  
  c1 = _mm_sub_epi8(_mm_cmplt_epi8(x0, x1), _mm_cmpgt_epi8(x0, x1));
  c2 = _mm_sub_epi8(_mm_cmplt_epi8(x0, x2), _mm_cmpgt_epi8(x0, x2));
  cat = _mm_add_epi8(c1, c2);
  cat = _mm_and_si128(cat, mask);
  
  select = _mm_cmpeq_epi8(cat, _mm_set1_epi8(-2));
  eoCount[0] = _mm_sub_epi8(eoCount[0], select);
  eoDiff[0] = _mm_add_epi64(eoDiff[0], _mm_sad_epu8(_mm_and_si128(diff, select), _mm_setzero_si128()));
  
  select = _mm_cmpeq_epi8(cat, _mm_set1_epi8(-1));
  eoCount[1] = _mm_sub_epi8(eoCount[1], select);
  eoDiff[1] = _mm_add_epi64(eoDiff[1], _mm_sad_epu8(_mm_and_si128(diff, select), _mm_setzero_si128()));
  
  select = _mm_cmpeq_epi8(cat, _mm_set1_epi8(1));
  eoCount[2] = _mm_sub_epi8(eoCount[2], select);
  eoDiff[2] = _mm_add_epi64(eoDiff[2], _mm_sad_epu8(_mm_and_si128(diff, select), _mm_setzero_si128()));
  
  select = _mm_cmpeq_epi8(cat, _mm_set1_epi8(2));
  eoCount[3] = _mm_sub_epi8(eoCount[3], select);
  eoDiff[3] = _mm_add_epi64(eoDiff[3], _mm_sad_epu8(_mm_and_si128(diff, select), _mm_setzero_si128()));
}


static void countBand(EB_S32 *boDiff, EB_U16 *boCount, EB_S32 cat01, EB_S32 diff01, EB_U8 *validSample)
{
    EB_U8 cat0 = (EB_U8)cat01;
    EB_U8 cat1 = (EB_U8)(cat01 >> 8);
    EB_S8 diff0 = (EB_S8)diff01;
    EB_S8 diff1 = (EB_S8)(diff01 >> 8);

    if (validSample[0])
    {
        boCount[cat0]++;
        boDiff[cat0] += diff0;
    }

    if (validSample[1])
    {
        boCount[cat1]++;
        boDiff[cat1] += diff1;
    }
}

static void updateValidSamples(EB_U8 *validSample, EB_U8 size, EB_S32 colCount, EB_S32 colCountDiv2)
{
    if (!validSample)
        return;

    EB_MEMSET(validSample, 1, size);
    if (colCountDiv2 == 0)
    {
        validSample[0] = (colCount & 1) ? 1 : 0;
        validSample[1] = 0;
    }
    else if (colCountDiv2 < 0)
        EB_MEMSET(validSample, 0, size);
}

EB_EXTERN EB_ERRORTYPE GatherSaoStatisticsLcu_BT_SSE2(
    EB_U8                   *inputSamplePtr,        // input parameter, source Picture Ptr
    EB_U32                   inputStride,           // input parameter, source stride
    EB_U8                   *reconSamplePtr,        // input parameter, deblocked Picture Ptr
    EB_U32                   reconStride,           // input parameter, deblocked stride
    EB_U32                   lcuWidth,              // input parameter, LCU width
    EB_U32                   lcuHeight,             // input parameter, LCU height
    EB_S32                  *boDiff,                // output parameter, used to store Band Offset diff, boDiff[SAO_BO_INTERVALS]
    EB_U16                  *boCount,										// output parameter, used to store Band Offset count, boCount[SAO_BO_INTERVALS]
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],     // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1])    // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
{
    EB_S32 colCount, colCountDiv2, rowCount;
#define EPIL16EXACTBYTES 2
    EB_U8 validSample[EPIL16EXACTBYTES];
    EB_U8 resetSize = sizeof(validSample[0]) * EPIL16EXACTBYTES;
    EB_S32 i, j;

    __m128i eoDiffX[SAO_EO_TYPES][SAO_EO_CATEGORIES];
    __m128i eoCountX[SAO_EO_TYPES][SAO_EO_CATEGORIES];

    lcuWidth -= 2;
    lcuHeight -= 2;
    inputSamplePtr += inputStride + 1;
    reconSamplePtr += reconStride + 1;

    colCount = lcuWidth;

    for (i = 0; i < SAO_BO_INTERVALS / 8; i++)
    {
        _mm_storeu_si128((__m128i *)(boCount + 8 * i), _mm_setzero_si128());
    }
    for (i = 0; i < SAO_BO_INTERVALS / 4; i++)
    {
        _mm_storeu_si128((__m128i *)(boDiff + 4 * i), _mm_setzero_si128());
    }

    for (i = 0; i < SAO_EO_TYPES; i++)
    {
        for (j = 0; j < SAO_EO_CATEGORIES; j++)
        {
            eoDiffX[i][j] = _mm_setzero_si128();
            eoCountX[i][j] = _mm_setzero_si128();
        }
    }

    do
    {
        __m128i mask = _mm_setzero_si128();
        EB_BYTE ptr = reconSamplePtr;
        EB_BYTE qtr = inputSamplePtr;
        EB_S32 idx;

        rowCount = lcuHeight;

        idx = (colCount >> 1) - 1;
        if (idx > 7)
        {
            idx = 7;
        }

        if (idx >= 0)
            mask = _mm_loadu_si128((__m128i *)maskTable[idx]);
        do
        {
            __m128i x0, y0;
            __m128i cat, diff;
            x0 = _mm_loadu_si128((__m128i *)ptr);
            y0 = _mm_loadu_si128((__m128i *)qtr);

            // Band offset
            cat = _mm_srli_epi16(_mm_and_si128(x0, _mm_set1_epi8(-8)), 3);
            cat = _mm_and_si128(cat, mask);
            x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
            y0 = _mm_xor_si128(y0, _mm_set1_epi8(-128));
            diff = _mm_subs_epi8(y0, x0);
            diff = _mm_and_si128(diff, mask);

            // Conditionally add the valid samples when counting BO diffs and counts.
            // Because when executing the right edge (colCount != 16) of a LCU, the redundant
            // samples will be loaded for calculation with the valid ones, as the use SSE2
            // intrinsics typically need to handle __m128i data type (128 bits, or 16 bytes).
            //
            // Note: the redundant samples wouldn't cause memory out of bound access, because
            // the contents of the memory (page frame and page table are ready, although some
            // bits the thread shouldn't touch) are loaded into the intrinsic temp variables,
            // and later into XMM registers of CPU for calculation.
            colCountDiv2 = colCount >> 1;

            // Note: can not use variable as the 2nd argument of _mm_extract_epi16(), whose
            // selector must be an integer constant in the range 0..7. So handle the bi-bytes
            // one by one...
            updateValidSamples(validSample, resetSize, colCount, colCountDiv2);
            colCountDiv2--;
            countBand(boDiff, boCount, _mm_extract_epi16(cat, 0), _mm_extract_epi16(diff, 0), validSample);

            updateValidSamples(validSample, resetSize, colCount, colCountDiv2);
            colCountDiv2--;
            countBand(boDiff, boCount, _mm_extract_epi16(cat, 1), _mm_extract_epi16(diff, 1), validSample);

            updateValidSamples(validSample, resetSize, colCount, colCountDiv2);
            colCountDiv2--;
            countBand(boDiff, boCount, _mm_extract_epi16(cat, 2), _mm_extract_epi16(diff, 2), validSample);

            updateValidSamples(validSample, resetSize, colCount, colCountDiv2);
            colCountDiv2--;
            countBand(boDiff, boCount, _mm_extract_epi16(cat, 3), _mm_extract_epi16(diff, 3), validSample);

            updateValidSamples(validSample, resetSize, colCount, colCountDiv2);
            colCountDiv2--;
            countBand(boDiff, boCount, _mm_extract_epi16(cat, 4), _mm_extract_epi16(diff, 4), validSample);

            updateValidSamples(validSample, resetSize, colCount, colCountDiv2);
            colCountDiv2--;
            countBand(boDiff, boCount, _mm_extract_epi16(cat, 5), _mm_extract_epi16(diff, 5), validSample);

            updateValidSamples(validSample, resetSize, colCount, colCountDiv2);
            colCountDiv2--;
            countBand(boDiff, boCount, _mm_extract_epi16(cat, 6), _mm_extract_epi16(diff, 6), validSample);

            updateValidSamples(validSample, resetSize, colCount, colCountDiv2);
            colCountDiv2--;
            countBand(boDiff, boCount, _mm_extract_epi16(cat, 7), _mm_extract_epi16(diff, 7), validSample);

            // Edge offset

            // Add 128 to difference to make it an unsigned integer to allow use of _mm_sad_epu8 intrinsic
            // This difference will be subtracted from the end result
            diff = _mm_xor_si128(diff, _mm_set1_epi8(-128));

            countEdge(eoDiffX[0], eoCountX[0], ptr, 1, x0, diff, mask);
            countEdge(eoDiffX[1], eoCountX[1], ptr, reconStride, x0, diff, mask);
            countEdge(eoDiffX[2], eoCountX[2], ptr, reconStride + 1, x0, diff, mask);
            countEdge(eoDiffX[3], eoCountX[3], ptr, reconStride - 1, x0, diff, mask);

            ptr += reconStride;
            qtr += inputStride;
        } while (--rowCount);

        reconSamplePtr += 16;
        inputSamplePtr += 16;

        colCount -= 16;
    } while (colCount > 0);

    for (i = 0; i < SAO_EO_TYPES; i++)
    {
        for (j = 0; j < SAO_EO_CATEGORIES; j++)
        {
            __m128i x0;
            EB_U32 *p;
            EB_U16/*EB_U32*/ count;

            // Note: accumulation of counts over 8 bits is ok since the maximum count is 62*4 = 248
            x0 = _mm_sad_epu8(eoCountX[i][j], _mm_setzero_si128());
            count = (EB_U16)(_mm_extract_epi32(x0, 0) + _mm_extract_epi32(x0, 2));
            eoCount[i][j] = count;

            // Note: subtracting 128 that was previously added in main loop
            p = (EB_U32 *)&eoDiffX[i][j];
            eoDiff[i][j] = p[0] + p[2] - 128 * count;
        }
    }

    return EB_ErrorNone;
}


EB_EXTERN EB_ERRORTYPE GatherSaoStatisticsLcu_OnlyEo_90_45_135_BT_SSE2(
	EB_U8                   *inputSamplePtr,        // input parameter, source Picture Ptr
	EB_U32                   inputStride,           // input parameter, source stride
	EB_U8                   *reconSamplePtr,        // input parameter, deblocked Picture Ptr
	EB_U32                   reconStride,           // input parameter, deblocked stride
	EB_U32                   lcuWidth,              // input parameter, LCU width
	EB_U32                   lcuHeight,             // input parameter, LCU height
	EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES+1],     // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
	EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES+1])    // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
{
  EB_S32 colCount, rowCount;
  EB_S32 i, j;
  
  __m128i eoDiffX[SAO_EO_TYPES][SAO_EO_CATEGORIES];
  __m128i eoCountX[SAO_EO_TYPES][SAO_EO_CATEGORIES];
  
  lcuWidth -= 2;
  lcuHeight -= 2;
  inputSamplePtr += inputStride + 1;
  reconSamplePtr += reconStride + 1;
  
  colCount = lcuWidth;
  
  for (i = 1; i < SAO_EO_TYPES; i++)
  {
    for (j = 0; j < SAO_EO_CATEGORIES; j++)
    {
      eoDiffX[i][j] = _mm_setzero_si128();
      eoCountX[i][j] = _mm_setzero_si128();
    }
  }
  
  do
  {
    __m128i mask;
    EB_BYTE ptr = reconSamplePtr;
    EB_BYTE qtr = inputSamplePtr;
    EB_S32 idx;
    
    rowCount = lcuHeight;
    
    idx = (colCount >> 1) - 1;
	mask = (idx >= 0 && idx < 8) ? _mm_loadu_si128((__m128i *)maskTable[idx]) : _mm_loadu_si128((__m128i *)maskTable[7]);
    do
    {
      __m128i x0, y0;
      __m128i diff;
      x0 = _mm_loadu_si128((__m128i *)ptr);
      y0 = _mm_loadu_si128((__m128i *)qtr);
      
      x0 = _mm_xor_si128(x0, _mm_set1_epi8(-128));
      y0 = _mm_xor_si128(y0, _mm_set1_epi8(-128));
      diff = _mm_subs_epi8(y0, x0);
      diff = _mm_and_si128(diff, mask);

      // Edge offset
      
      // Add 128 to difference to make it an unsigned integer to allow use of _mm_sad_epu8 intrinsic
      // This difference will be subtracted from the end result
      diff = _mm_xor_si128(diff, _mm_set1_epi8(-128));
      
      countEdge(eoDiffX[1], eoCountX[1], ptr, reconStride, x0, diff, mask);
      countEdge(eoDiffX[2], eoCountX[2], ptr, reconStride+1, x0, diff, mask);
      countEdge(eoDiffX[3], eoCountX[3], ptr, reconStride-1, x0, diff, mask);
      
      ptr += reconStride;
      qtr += inputStride;
    }
    while (--rowCount);
    
    reconSamplePtr += 16;
    inputSamplePtr += 16;
    
    colCount -= 16;
  }
  while (colCount > 0);
  
  for (i = 1; i < SAO_EO_TYPES; i++)
  {
    for (j = 0; j < SAO_EO_CATEGORIES; j++)
    {
      __m128i x0;
      EB_U32 *p;
      EB_U16/*EB_U32*/ count;
      
      // Note: accumulation of counts over 8 bits is ok since the maximum count is 62*4 = 248
      x0 = _mm_sad_epu8(eoCountX[i][j], _mm_setzero_si128());
      count =(EB_U16)(_mm_extract_epi32(x0, 0) + _mm_extract_epi32(x0, 2));
      eoCount[i][j] = count;
      
      // Note: subtracting 128 that was previously added in main loop
      p = (EB_U32 *)&eoDiffX[i][j];
      eoDiff[i][j] = p[0] + p[2] - 128 * count;
    }
  }
  
  return EB_ErrorNone;
}
