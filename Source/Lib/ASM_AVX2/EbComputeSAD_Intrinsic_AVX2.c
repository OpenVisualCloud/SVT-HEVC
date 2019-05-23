/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbComputeSAD_AVX2.h"
#include "EbDefinitions.h"
#include "immintrin.h"

#ifndef _mm256_setr_m128i
#define _mm256_setr_m128i(/* __m128i */ hi, /* __m128i */ lo) \
    _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 0x1)
#endif

#define UPDATE_BEST(s, k, offset) \
  temSum1 = _mm_extract_epi32(s, k); \
  if (temSum1 < lowSum) { \
    lowSum = temSum1; \
    xBest = j + offset + k; \
    yBest = i; \
  }

/*******************************************************************************
 * Requirement: width   = 4, 8, 16, 24, 32, 48 or 64
 * Requirement: height <= 64
 * Requirement: height % 2 = 0 when width = 4 or 8
*******************************************************************************/
void SadLoopKernel_AVX2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width,                          // input parameter, block width (N)
    EB_U64 *bestSad,
    EB_S16 *xSearchCenter,
    EB_S16 *ySearchCenter,
    EB_U32  srcStrideRaw,                   // input parameter, source stride (no line skipping)
    EB_S16 searchAreaWidth,
    EB_S16 searchAreaHeight)
{
  EB_S16 xBest = *xSearchCenter, yBest = *ySearchCenter;
  EB_U32 lowSum = 0xffffff;
  EB_U32 temSum1 = 0;
  EB_S16 i, j;
  EB_U32 k, l;
  EB_U32 leftover = searchAreaWidth & 7;
  const EB_U8 *pRef, *pSrc;
  __m128i s0, s1, s2, s3, s4, s5, s6, s8 = _mm_set1_epi32(-1);
  __m256i ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7, ss8;

  if (leftover) {
    for (k=0; k<leftover; k++) {
      s8 = _mm_slli_si128(s8, 2);
    }
  }

  switch (width) {
  case 4:

    if (!(height % 4)) {
      EB_U32 srcStrideT = 3 * srcStride;
      EB_U32 refStrideT = 3 * refStride;
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss5 = _mm256_setzero_si256();
          for (k=0; k<height; k+=4) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef))), _mm_loadu_si128((__m128i*)(pRef + 2 * refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + refStride))), _mm_loadu_si128((__m128i*)(pRef + refStrideT)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_unpacklo_epi64(_mm_cvtsi32_si128(*(EB_U32 *)pSrc), _mm_cvtsi32_si128(*(EB_U32 *)(pSrc + srcStride)))), _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(EB_U32 *)(pSrc + 2 * srcStride)), _mm_cvtsi32_si128(*(EB_U32 *)(pSrc + srcStrideT))), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            pSrc += srcStride << 2;
            pRef += refStride << 2;
          }
          ss3 = _mm256_adds_epu16(ss3, ss5);
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss5 = _mm256_setzero_si256();
          for (k=0; k<height; k+=4) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)), _mm_loadu_si128((__m128i*)(pRef + 2 * refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + refStride))), _mm_loadu_si128((__m128i*)(pRef + refStrideT)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_unpacklo_epi64(_mm_cvtsi32_si128(*(EB_U32 *)pSrc), _mm_cvtsi32_si128(*(EB_U32 *)(pSrc + srcStride)))), _mm_unpacklo_epi64(_mm_cvtsi32_si128(*(EB_U32 *)(pSrc + 2 * srcStride)), _mm_cvtsi32_si128(*(EB_U32 *)(pSrc + srcStrideT))), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            pSrc += srcStride << 2;
            pRef += refStride << 2;
          }
          ss3 = _mm256_adds_epu16(ss3, ss5);
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }
        ref += srcStrideRaw;
      }
    }
    else {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          s3 = _mm_setzero_si128();
          for (k=0; k<height; k+=2) {
            s0 = _mm_loadu_si128((__m128i*)pRef);
            s1 = _mm_loadu_si128((__m128i*)(pRef+refStride));
            s2 = _mm_cvtsi32_si128(*(EB_U32 *)pSrc);
            s5 = _mm_cvtsi32_si128(*(EB_U32 *)(pSrc+srcStride));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
            pSrc += srcStride << 1;
            pRef += refStride << 1;
          }
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          s3 = _mm_setzero_si128();
          for (k=0; k<height; k+=2) {
            s0 = _mm_loadu_si128((__m128i*)pRef);
            s1 = _mm_loadu_si128((__m128i*)(pRef+refStride));
            s2 = _mm_cvtsi32_si128(*(EB_U32 *)pSrc);
            s5 = _mm_cvtsi32_si128(*(EB_U32 *)(pSrc+srcStride));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
            pSrc += srcStride << 1;
            pRef += refStride << 1;
          }
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }
        ref += srcStrideRaw;
      }
    }

    break;

  case 8:
    if (!(height % 4)) {
      EB_U32 srcStrideT = 3 * srcStride;
      EB_U32 refStrideT = 3 * refStride;
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=4) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)), _mm_loadu_si128((__m128i*)(pRef + 2 * refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + refStride))), _mm_loadu_si128((__m128i*)(pRef + refStrideT)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)pSrc), _mm_loadl_epi64((__m128i*)(pSrc + srcStride)))), _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)(pSrc + 2 * srcStride)), _mm_loadl_epi64((__m128i*)(pSrc + srcStrideT))), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride << 2;
            pRef += refStride << 2;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=4) {
            ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)),             _mm_loadu_si128((__m128i*)(pRef+2*refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + refStride))), _mm_loadu_si128((__m128i*)(pRef + refStrideT)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)pSrc), _mm_loadl_epi64((__m128i*)(pSrc + srcStride)))), _mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)(pSrc + 2 * srcStride)), _mm_loadl_epi64((__m128i*)(pSrc + srcStrideT))), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride << 2;
            pRef += refStride << 2;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }
        ref += srcStrideRaw;
      }
    }
    else {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          s3 = s4 = _mm_setzero_si128();
          for (k=0; k<height; k+=2) {
            s0 = _mm_loadu_si128((__m128i*)pRef);
            s1 = _mm_loadu_si128((__m128i*)(pRef+refStride));
            s2 = _mm_loadl_epi64((__m128i*)pSrc);
            s5 = _mm_loadl_epi64((__m128i*)(pSrc+srcStride));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));
            pSrc += srcStride << 1;
            pRef += refStride << 1;
          }
          s3 = _mm_adds_epu16(s3, s4);
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          s3 = s4 = _mm_setzero_si128();
          for (k=0; k<height; k+=2) {
            s0 = _mm_loadu_si128((__m128i*)pRef);
            s1 = _mm_loadu_si128((__m128i*)(pRef+refStride));
            s2 = _mm_loadl_epi64((__m128i*)pSrc);
            s5 = _mm_loadl_epi64((__m128i*)(pSrc+srcStride));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s1, s5, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s1, s5, 5));
            pSrc += srcStride << 1;
            pRef += refStride << 1;
          }
          s3 = _mm_adds_epu16(s3, s4);
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }
        ref += srcStrideRaw;
      }
    }
    break;

  case 16:
    if (height <= 16) {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)), _mm_loadu_si128((__m128i*)(pRef + refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + 8))), _mm_loadu_si128((__m128i*)(pRef + refStride + 8)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pSrc)), _mm_loadu_si128((__m128i*)(pSrc + srcStride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += 2 * srcStride;
            pRef += 2 * refStride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)), _mm_loadu_si128((__m128i*)(pRef + refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + 8))), _mm_loadu_si128((__m128i*)(pRef + refStride + 8)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pSrc)), _mm_loadu_si128((__m128i*)(pSrc + srcStride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += 2 * srcStride;
            pRef += 2 * refStride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s3 = _mm_or_si128(s3, s8);
          s3 = _mm_minpos_epu16(s3);
          temSum1 = _mm_extract_epi16(s3, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s3, 1));
            yBest = i;
          }
        }
        ref += srcStrideRaw;
      }
    }
    else if (height <= 32) {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)), _mm_loadu_si128((__m128i*)(pRef + refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + 8))), _mm_loadu_si128((__m128i*)(pRef + refStride + 8)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pSrc)), _mm_loadu_si128((__m128i*)(pSrc + srcStride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += 2 * srcStride;
            pRef += 2 * refStride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss3, 0);
          s5 = _mm256_extracti128_si256(ss3, 1);
          s4 = _mm_minpos_epu16(s3);
          s6 = _mm_minpos_epu16(s5);
          s4 = _mm_unpacklo_epi16(s4, s4);
          s4 = _mm_unpacklo_epi32(s4, s4);
          s4 = _mm_unpacklo_epi64(s4, s4);
          s6 = _mm_unpacklo_epi16(s6, s6);
          s6 = _mm_unpacklo_epi32(s6, s6);
          s6 = _mm_unpacklo_epi64(s6, s6);
          s3 = _mm_sub_epi16(s3, s4);
          s5 = _mm_adds_epu16(s5, s3);
          s5 = _mm_sub_epi16(s5, s6);
          s5 = _mm_minpos_epu16(s5);
          temSum1  = _mm_extract_epi16(s5, 0);
          temSum1 += _mm_extract_epi16(s4, 0);
          temSum1 += _mm_extract_epi16(s6, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s5, 1));
            yBest = i;
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)), _mm_loadu_si128((__m128i*)(pRef + refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + 8))), _mm_loadu_si128((__m128i*)(pRef + refStride + 8)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pSrc)), _mm_loadu_si128((__m128i*)(pSrc + srcStride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += 2 * srcStride;
            pRef += 2 * refStride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss3, 0);
          s5 = _mm256_extracti128_si256(ss3, 1);
          s3 = _mm_or_si128(s3, s8);
          s5 = _mm_or_si128(s5, s8);
          s4 = _mm_minpos_epu16(s3);
          s6 = _mm_minpos_epu16(s5);
          s4 = _mm_unpacklo_epi16(s4, s4);
          s4 = _mm_unpacklo_epi32(s4, s4);
          s4 = _mm_unpacklo_epi64(s4, s4);
          s6 = _mm_unpacklo_epi16(s6, s6);
          s6 = _mm_unpacklo_epi32(s6, s6);
          s6 = _mm_unpacklo_epi64(s6, s6);
          s3 = _mm_sub_epi16(s3, s4);
          s5 = _mm_adds_epu16(s5, s3);
          s5 = _mm_sub_epi16(s5, s6);
          s5 = _mm_minpos_epu16(s5);
          temSum1  = _mm_extract_epi16(s5, 0);
          temSum1 += _mm_extract_epi16(s4, 0);
          temSum1 += _mm_extract_epi16(s6, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s5, 1));
            yBest = i;
          }
        }
        ref += srcStrideRaw;
      }
    }
    else {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)), _mm_loadu_si128((__m128i*)(pRef + refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + 8))), _mm_loadu_si128((__m128i*)(pRef + refStride + 8)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pSrc)), _mm_loadu_si128((__m128i*)(pSrc + srcStride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += 2 * srcStride;
            pRef += 2 * refStride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss0 = _mm256_adds_epu16(ss3, ss5);
          s0 = _mm_adds_epu16(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s3, 0, 4);
              UPDATE_BEST(s3, 1, 4);
              UPDATE_BEST(s3, 2, 4);
              UPDATE_BEST(s3, 3, 4);
            }
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k+=2) {
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pRef)), _mm_loadu_si128((__m128i*)(pRef + refStride)), 0x1);
			ss1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + 8))), _mm_loadu_si128((__m128i*)(pRef + refStride + 8)), 0x1);
			ss2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pSrc)), _mm_loadu_si128((__m128i*)(pSrc + srcStride)), 0x1);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += 2 * srcStride;
            pRef += 2 * refStride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss0 = _mm256_adds_epu16(ss3, ss5);
          s0 = _mm_adds_epu16(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  temSum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (temSum1 < lowSum) {
                    lowSum = temSum1;
                    xBest = (EB_S16)(j + leftover - k);
                    yBest = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += srcStrideRaw;
      }
    }
    break;

  case 24:
    if (height <= 16) {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s5 = _mm256_extracti128_si256(ss5, 0);
          s4 = _mm_minpos_epu16(s3);
          s6 = _mm_minpos_epu16(s5);
          s4 = _mm_unpacklo_epi16(s4, s4);
          s4 = _mm_unpacklo_epi32(s4, s4);
          s4 = _mm_unpacklo_epi64(s4, s4);
          s6 = _mm_unpacklo_epi16(s6, s6);
          s6 = _mm_unpacklo_epi32(s6, s6);
          s6 = _mm_unpacklo_epi64(s6, s6);
          s3 = _mm_sub_epi16(s3, s4);
          s5 = _mm_adds_epu16(s5, s3);
          s5 = _mm_sub_epi16(s5, s6);
          s5 = _mm_minpos_epu16(s5);
          temSum1  = _mm_extract_epi16(s5, 0);
          temSum1 += _mm_extract_epi16(s4, 0);
          temSum1 += _mm_extract_epi16(s6, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s5, 1));
            yBest = i;
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          s3 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
          s5 = _mm256_extracti128_si256(ss5, 0);
          s3 = _mm_or_si128(s3, s8);
          s5 = _mm_or_si128(s5, s8);
          s4 = _mm_minpos_epu16(s3);
          s6 = _mm_minpos_epu16(s5);
          s4 = _mm_unpacklo_epi16(s4, s4);
          s4 = _mm_unpacklo_epi32(s4, s4);
          s4 = _mm_unpacklo_epi64(s4, s4);
          s6 = _mm_unpacklo_epi16(s6, s6);
          s6 = _mm_unpacklo_epi32(s6, s6);
          s6 = _mm_unpacklo_epi64(s6, s6);
          s3 = _mm_sub_epi16(s3, s4);
          s5 = _mm_adds_epu16(s5, s3);
          s5 = _mm_sub_epi16(s5, s6);
          s5 = _mm_minpos_epu16(s5);
          temSum1  = _mm_extract_epi16(s5, 0);
          temSum1 += _mm_extract_epi16(s4, 0);
          temSum1 += _mm_extract_epi16(s6, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s5, 1));
            yBest = i;
          }
        }
        ref += srcStrideRaw;
      }
    }
    else {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          s3 = _mm256_extracti128_si256(ss3, 0);
          s4 = _mm256_extracti128_si256(ss3, 1);
          s5 = _mm256_extracti128_si256(ss5, 0);
          s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), s5);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
              s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
              s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
              s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
              s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
              s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
              s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), s2);
              s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), s5);
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s3, 0, 4);
              UPDATE_BEST(s3, 1, 4);
              UPDATE_BEST(s3, 2, 4);
              UPDATE_BEST(s3, 3, 4);
            }
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          s3 = _mm256_extracti128_si256(ss3, 0);
          s4 = _mm256_extracti128_si256(ss3, 1);
          s5 = _mm256_extracti128_si256(ss5, 0);
          s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), s5);
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              s0 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
              s3 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
              s1 = _mm_unpacklo_epi16(s4, _mm_setzero_si128());
              s4 = _mm_unpackhi_epi16(s4, _mm_setzero_si128());
              s2 = _mm_unpacklo_epi16(s5, _mm_setzero_si128());
              s5 = _mm_unpackhi_epi16(s5, _mm_setzero_si128());
              s0 = _mm_add_epi32(_mm_add_epi32(s0, s1), s2);
              s3 = _mm_add_epi32(_mm_add_epi32(s3, s4), s5);
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  temSum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (temSum1 < lowSum) {
                    lowSum = temSum1;
                    xBest = (EB_S16)(j + leftover - k);
                    yBest = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += srcStrideRaw;
      }
    }
    break;

  case 32:
    if (height <= 16) {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss3, 0);
          s5 = _mm256_extracti128_si256(ss3, 1);
          s4 = _mm_minpos_epu16(s3);
          s6 = _mm_minpos_epu16(s5);
          s4 = _mm_unpacklo_epi16(s4, s4);
          s4 = _mm_unpacklo_epi32(s4, s4);
          s4 = _mm_unpacklo_epi64(s4, s4);
          s6 = _mm_unpacklo_epi16(s6, s6);
          s6 = _mm_unpacklo_epi32(s6, s6);
          s6 = _mm_unpacklo_epi64(s6, s6);
          s3 = _mm_sub_epi16(s3, s4);
          s5 = _mm_adds_epu16(s5, s3);
          s5 = _mm_sub_epi16(s5, s6);
          s5 = _mm_minpos_epu16(s5);
          temSum1  = _mm_extract_epi16(s5, 0);
          temSum1 += _mm_extract_epi16(s4, 0);
          temSum1 += _mm_extract_epi16(s6, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s5, 1));
            yBest = i;
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss3 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss3, 0);
          s5 = _mm256_extracti128_si256(ss3, 1);
          s3 = _mm_or_si128(s3, s8);
          s5 = _mm_or_si128(s5, s8);
          s4 = _mm_minpos_epu16(s3);
          s6 = _mm_minpos_epu16(s5);
          s4 = _mm_unpacklo_epi16(s4, s4);
          s4 = _mm_unpacklo_epi32(s4, s4);
          s4 = _mm_unpacklo_epi64(s4, s4);
          s6 = _mm_unpacklo_epi16(s6, s6);
          s6 = _mm_unpacklo_epi32(s6, s6);
          s6 = _mm_unpacklo_epi64(s6, s6);
          s3 = _mm_sub_epi16(s3, s4);
          s5 = _mm_adds_epu16(s5, s3);
          s5 = _mm_sub_epi16(s5, s6);
          s5 = _mm_minpos_epu16(s5);
          temSum1  = _mm_extract_epi16(s5, 0);
          temSum1 += _mm_extract_epi16(s4, 0);
          temSum1 += _mm_extract_epi16(s6, 0);
          if (temSum1 < lowSum) {
            lowSum = temSum1;
            xBest = (EB_S16)(j + _mm_extract_epi16(s5, 1));
            yBest = i;
          }
        }
        ref += srcStrideRaw;
      }
    }
    else if (height <= 32) {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss6 = _mm256_adds_epu16(ss3, ss5);
          s3 = _mm256_extracti128_si256(ss6, 0);
          s4 = _mm256_extracti128_si256(ss6, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s3, 0, 4);
              UPDATE_BEST(s3, 1, 4);
              UPDATE_BEST(s3, 2, 4);
              UPDATE_BEST(s3, 3, 4);
            }
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss6 = _mm256_adds_epu16(ss3, ss5);
          s3 = _mm256_extracti128_si256(ss6, 0);
          s4 = _mm256_extracti128_si256(ss6, 1);
          s0 = _mm_adds_epu16(s3, s4);
          //s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  temSum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (temSum1 < lowSum) {
                    lowSum = temSum1;
                    xBest = (EB_S16)(j + leftover - k);
                    yBest = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += srcStrideRaw;
      }
    }
    else {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss7, 0);
          s4 = _mm256_extracti128_si256(ss7, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s3, 0, 4);
              UPDATE_BEST(s3, 1, 4);
              UPDATE_BEST(s3, 2, 4);
              UPDATE_BEST(s3, 3, 4);
            }
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss7, 0);
          s4 = _mm256_extracti128_si256(ss7, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  temSum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (temSum1 < lowSum) {
                    lowSum = temSum1;
                    xBest = (EB_S16)(j + leftover - k);
                    yBest = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += srcStrideRaw;
      }
    }
    break;

  case 48:
    if (height <= 32) {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          s3 = s4 = s5 = s6 = _mm_setzero_si128();
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            s0 = _mm_loadu_si128((__m128i*)(pRef + 32));
            s1 = _mm_loadu_si128((__m128i*)(pRef + 40));
            s2 = _mm_loadu_si128((__m128i*)(pSrc + 32));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
            pSrc += srcStride;
            pRef += refStride;
          }
          s3 = _mm_adds_epu16(s3, s4);
          s5 = _mm_adds_epu16(s5, s6);
          s0 = _mm_adds_epu16(s3, s5);
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss6 = _mm256_adds_epu16(ss3, ss5);
          s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm256_extracti128_si256(ss6, 0), _mm256_extracti128_si256(ss6, 1)));
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s1 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s3, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s5, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s3, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s5, _mm_setzero_si128()));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s1, 0, 4);
              UPDATE_BEST(s1, 1, 4);
              UPDATE_BEST(s1, 2, 4);
              UPDATE_BEST(s1, 3, 4);
            }
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          s3 = s4 = s5 = s6 = _mm_setzero_si128();
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            s0 = _mm_loadu_si128((__m128i*)(pRef + 32));
            s1 = _mm_loadu_si128((__m128i*)(pRef + 40));
            s2 = _mm_loadu_si128((__m128i*)(pSrc + 32));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
            pSrc += srcStride;
            pRef += refStride;
          }
          s3 = _mm_adds_epu16(s3, s4);
          s5 = _mm_adds_epu16(s5, s6);
          s0 = _mm_adds_epu16(s3, s5);
          ss3 = _mm256_adds_epu16(ss3, ss4);
          ss5 = _mm256_adds_epu16(ss5, ss6);
          ss6 = _mm256_adds_epu16(ss3, ss5);
          s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm256_extracti128_si256(ss6, 0), _mm256_extracti128_si256(ss6, 1)));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss4 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss6 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss4 = _mm256_add_epi32(ss4, ss6);
              ss3 = _mm256_add_epi32(ss3, ss5);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));
              s1 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s3, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s5, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s3, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s5, _mm_setzero_si128()));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  temSum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (temSum1 < lowSum) {
                    lowSum = temSum1;
                    xBest = (EB_S16)(j + leftover - k);
                    yBest = i;
                  }
                }
                s0 = s1;
              }
            }
          }
        }
        ref += srcStrideRaw;
      }
    }
    else {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          s3 = s4 = s5 = s6 = _mm_setzero_si128();
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            s0 = _mm_loadu_si128((__m128i*)(pRef + 32));
            s1 = _mm_loadu_si128((__m128i*)(pRef + 40));
            s2 = _mm_loadu_si128((__m128i*)(pSrc + 32));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
            pSrc += srcStride;
            pRef += refStride;
          }
          s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
          ss7 = _mm256_adds_epu16(ss3, ss4);
          ss8 = _mm256_adds_epu16(ss5, ss6);
          ss7 = _mm256_adds_epu16(ss7, ss8);
          s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm256_extracti128_si256(ss7, 0), _mm256_extracti128_si256(ss7, 1)));
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s1 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s3, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s4, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s5, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s6, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s3, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s4, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s5, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s6, _mm_setzero_si128()));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s1, 0, 4);
              UPDATE_BEST(s1, 1, 4);
              UPDATE_BEST(s1, 2, 4);
              UPDATE_BEST(s1, 3, 4);
            }
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          s3 = s4 = s5 = s6 = _mm_setzero_si128();
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            s0 = _mm_loadu_si128((__m128i*)(pRef + 32));
            s1 = _mm_loadu_si128((__m128i*)(pRef + 40));
            s2 = _mm_loadu_si128((__m128i*)(pSrc + 32));
            s3 = _mm_adds_epu16(s3, _mm_mpsadbw_epu8(s0, s2, 0));
            s4 = _mm_adds_epu16(s4, _mm_mpsadbw_epu8(s0, s2, 5));
            s5 = _mm_adds_epu16(s5, _mm_mpsadbw_epu8(s1, s2, 2));
            s6 = _mm_adds_epu16(s6, _mm_mpsadbw_epu8(s1, s2, 7));
            pSrc += srcStride;
            pRef += refStride;
          }
          s0 = _mm_adds_epu16(_mm_adds_epu16(s3, s4), _mm_adds_epu16(s5, s6));
          ss7 = _mm256_adds_epu16(ss3, ss4);
          ss8 = _mm256_adds_epu16(ss5, ss6);
          ss7 = _mm256_adds_epu16(ss7, ss8);
          s0 = _mm_adds_epu16(s0, _mm_adds_epu16(_mm256_extracti128_si256(ss7, 0), _mm256_extracti128_si256(ss7, 1)));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s1 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s3, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s4, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s5, _mm_setzero_si128()));
              s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(s6, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s3, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s4, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s5, _mm_setzero_si128()));
              s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(s6, _mm_setzero_si128()));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  temSum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (temSum1 < lowSum) {
                    lowSum = temSum1;
                    xBest = (EB_S16)(j + leftover - k);
                    yBest = i;
                  }
                }
                s0 = s1;
              }
            }
          }
        }
        ref += srcStrideRaw;
      }
    }
    break;

  case 64:
    if (height <= 32) {
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            ss0 = _mm256_loadu_si256((__m256i*)(pRef + 32));
            ss1 = _mm256_loadu_si256((__m256i*)(pRef + 40));
            ss2 = _mm256_loadu_si256((__m256i *)(pSrc + 32));
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss7, 0);
          s4 = _mm256_extracti128_si256(ss7, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s3, 0, 4);
              UPDATE_BEST(s3, 1, 4);
              UPDATE_BEST(s3, 2, 4);
              UPDATE_BEST(s3, 3, 4);
            }
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            ss0 = _mm256_loadu_si256((__m256i*)(pRef + 32));
            ss1 = _mm256_loadu_si256((__m256i*)(pRef + 40));
            ss2 = _mm256_loadu_si256((__m256i *)(pSrc + 32));
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss7 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          s3 = _mm256_extracti128_si256(ss7, 0);
          s4 = _mm256_extracti128_si256(ss7, 1);
          s0 = _mm_adds_epu16(s3, s4);
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss0 = _mm256_unpacklo_epi16(ss3, _mm256_setzero_si256());
              ss3 = _mm256_unpackhi_epi16(ss3, _mm256_setzero_si256());
              ss1 = _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256());
              ss4 = _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256());
              ss2 = _mm256_unpacklo_epi16(ss5, _mm256_setzero_si256());
              ss5 = _mm256_unpackhi_epi16(ss5, _mm256_setzero_si256());
              ss7 = _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256());
              ss6 = _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256());
              ss0 = _mm256_add_epi32(_mm256_add_epi32(ss0, ss1), _mm256_add_epi32(ss2, ss7));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(ss3, ss4), _mm256_add_epi32(ss5, ss6));
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  temSum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (temSum1 < lowSum) {
                    lowSum = temSum1;
                    xBest = (EB_S16)(j + leftover - k);
                    yBest = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += srcStrideRaw;
      }
    }
    else {
      __m256i ss9, ss10;
      for (i=0; i<searchAreaHeight; i++) {
        for (j=0; j<=searchAreaWidth-8; j+=8) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = ss7 = ss8 = ss9 = ss10 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            ss0 = _mm256_loadu_si256((__m256i*)(pRef + 32));
            ss1 = _mm256_loadu_si256((__m256i*)(pRef + 40));
            ss2 = _mm256_loadu_si256((__m256i *)(pSrc + 32));
            ss7 = _mm256_adds_epu16(ss7, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss8 = _mm256_adds_epu16(ss8, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss9 = _mm256_adds_epu16(ss9, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss10 = _mm256_adds_epu16(ss10, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss0 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          ss0 = _mm256_adds_epu16(ss0, _mm256_adds_epu16(_mm256_adds_epu16(ss7, ss8), _mm256_adds_epu16(ss9, ss10)));
          s0 = _mm_adds_epu16(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss0 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpacklo_epi16(ss3, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpacklo_epi16(ss5, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256())));
              ss1 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpackhi_epi16(ss3, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpackhi_epi16(ss5, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256())));
              ss2 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpacklo_epi16(ss7, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss8, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpacklo_epi16(ss9, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss10, _mm256_setzero_si256())));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpackhi_epi16(ss7, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss8, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpackhi_epi16(ss9, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss10, _mm256_setzero_si256())));
              ss0 = _mm256_add_epi32(ss0, ss2);
              ss1 = _mm256_add_epi32(ss1, ss3);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss1, 0), _mm256_extracti128_si256(ss1, 1));
              UPDATE_BEST(s0, 0, 0);
              UPDATE_BEST(s0, 1, 0);
              UPDATE_BEST(s0, 2, 0);
              UPDATE_BEST(s0, 3, 0);
              UPDATE_BEST(s3, 0, 4);
              UPDATE_BEST(s3, 1, 4);
              UPDATE_BEST(s3, 2, 4);
              UPDATE_BEST(s3, 3, 4);
            }
          }
        }

        if (leftover) {
          pSrc = src;
          pRef = ref + j;
          ss3 = ss4 = ss5 = ss6 = ss7 = ss8 = ss9 = ss10 = _mm256_setzero_si256();
          for (k=0; k<height; k++) {
            ss0 = _mm256_loadu_si256((__m256i*)pRef);
            ss1 = _mm256_loadu_si256((__m256i*)(pRef+8));
            ss2 = _mm256_loadu_si256((__m256i *)pSrc);
            ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss5 = _mm256_adds_epu16(ss5, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss6 = _mm256_adds_epu16(ss6, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            ss0 = _mm256_loadu_si256((__m256i*)(pRef + 32));
            ss1 = _mm256_loadu_si256((__m256i*)(pRef + 40));
            ss2 = _mm256_loadu_si256((__m256i *)(pSrc + 32));
            ss7 = _mm256_adds_epu16(ss7, _mm256_mpsadbw_epu8(ss0, ss2, 0));
            ss8 = _mm256_adds_epu16(ss8, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
            ss9 = _mm256_adds_epu16(ss9, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
            ss10 = _mm256_adds_epu16(ss10, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
            pSrc += srcStride;
            pRef += refStride;
          }
          ss0 = _mm256_adds_epu16(_mm256_adds_epu16(ss3, ss4), _mm256_adds_epu16(ss5, ss6));
          ss0 = _mm256_adds_epu16(ss0, _mm256_adds_epu16(_mm256_adds_epu16(ss7, ss8), _mm256_adds_epu16(ss9, ss10)));
          s0 = _mm_adds_epu16(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
          s0 = _mm_or_si128(s0, s8);
          s0 = _mm_minpos_epu16(s0);
          temSum1 = _mm_extract_epi16(s0, 0);
          if (temSum1 < lowSum) {
            if (temSum1 != 0xFFFF) { // no overflow
              lowSum = temSum1;
              xBest = (EB_S16)(j + _mm_extract_epi16(s0, 1));
              yBest = i;
            }
            else {
              ss0 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpacklo_epi16(ss3, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss4, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpacklo_epi16(ss5, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss6, _mm256_setzero_si256())));
              ss1 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpackhi_epi16(ss3, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss4, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpackhi_epi16(ss5, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss6, _mm256_setzero_si256())));
              ss2 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpacklo_epi16(ss7, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss8, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpacklo_epi16(ss9, _mm256_setzero_si256()), _mm256_unpacklo_epi16(ss10, _mm256_setzero_si256())));
              ss3 = _mm256_add_epi32(_mm256_add_epi32(_mm256_unpackhi_epi16(ss7, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss8, _mm256_setzero_si256())), _mm256_add_epi32(_mm256_unpackhi_epi16(ss9, _mm256_setzero_si256()), _mm256_unpackhi_epi16(ss10, _mm256_setzero_si256())));
              ss0 = _mm256_add_epi32(ss0, ss2);
              ss1 = _mm256_add_epi32(ss1, ss3);
              s0 = _mm_add_epi32(_mm256_extracti128_si256(ss0, 0), _mm256_extracti128_si256(ss0, 1));
              s3 = _mm_add_epi32(_mm256_extracti128_si256(ss1, 0), _mm256_extracti128_si256(ss1, 1));
              k = leftover;
              while (k > 0) {
                for (l=0; l < 4 && k; l++, k--) {
                  temSum1 = _mm_extract_epi32(s0, 0);
                  s0 = _mm_srli_si128(s0, 4);
                  if (temSum1 < lowSum) {
                    lowSum = temSum1;
                    xBest = (EB_S16)(j + leftover - k);
                    yBest = i;
                  }
                }
                s0 = s3;
              }
            }
          }
        }
        ref += srcStrideRaw;
      }
    }
    break;

  default:
    break;
  }


  *bestSad = lowSum;
  *xSearchCenter = xBest;
  *ySearchCenter = yBest;
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
EB_U32 Compute24xMSad_AVX2_INTRIN(
	EB_U8  *src,        // input parameter, source samples Ptr
	EB_U32  srcStride,  // input parameter, source stride
	EB_U8  *ref,        // input parameter, reference samples Ptr
	EB_U32  refStride,  // input parameter, reference stride
	EB_U32  height,     // input parameter, block height (M)
	EB_U32  width)     // input parameter, block width (N)  
{
	__m128i xmm0, xmm1;
	__m256i ymm0, ymm1;
	EB_U32 y;
	(void)width;

	ymm0 = ymm1 = _mm256_setzero_si256();
	for (y = 0; y < height; y += 2) {
		ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i *)ref)));
		ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + srcStride)), _mm256_loadu_si256((__m256i *)(ref + refStride))));
		src += srcStride << 1;
		ref += refStride << 1;
	}
	xmm0 = _mm_add_epi32(_mm256_extracti128_si256(ymm0, 0), _mm256_extracti128_si256(ymm1, 0));
	xmm1 = _mm_add_epi32(_mm256_extracti128_si256(ymm0, 1), _mm256_extracti128_si256(ymm1, 1));
	xmm0 = _mm_add_epi32(_mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8)), xmm1);
	return (EB_U32)_mm_cvtsi128_si32(xmm0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
EB_U32 Compute32xMSad_AVX2_INTRIN(
	EB_U8  *src,        // input parameter, source samples Ptr
	EB_U32  srcStride,  // input parameter, source stride
	EB_U8  *ref,        // input parameter, reference samples Ptr
	EB_U32  refStride,  // input parameter, reference stride
	EB_U32  height,     // input parameter, block height (M)
	EB_U32  width)     // input parameter, block width (N)  
{
	__m128i xmm0;
	__m256i ymm0, ymm1;
	EB_U32 y;
	(void)width;

	ymm0 = ymm1 = _mm256_setzero_si256();
	for (y = 0; y < height; y += 2) {
		ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i *)ref)));
		ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + srcStride)), _mm256_loadu_si256((__m256i *)(ref + refStride))));
		src += srcStride << 1;
		ref += refStride << 1;
	}
	ymm0 = _mm256_add_epi32(ymm0, ymm1);
	xmm0 = _mm_add_epi32(_mm256_castsi256_si128(ymm0), _mm256_extracti128_si256(ymm0, 1));
	xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));
	return (EB_U32) /*xmm0.m128i_i64[0];*/ _mm_cvtsi128_si32(xmm0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
EB_U32 Compute40xMSad_AVX2_INTRIN(
        EB_U8  *src,        // input parameter, source samples Ptr
        EB_U32  srcStride,  // input parameter, source stride
        EB_U8  *ref,        // input parameter, reference samples Ptr
        EB_U32  refStride,  // input parameter, reference stride
        EB_U32  height,     // input parameter, block height (M)
        EB_U32  width)     // input parameter, block width (N)
{
        __m128i xmm0, xmm1;
        __m256i ymm0, ymm1;
        EB_U32 y;
        (void)width;

        ymm0 = ymm1 = _mm256_setzero_si256();
        xmm0 = xmm1 = _mm_setzero_si128();
        for (y = 0; y < height; y += 2) {
                ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i*)ref)));
                xmm0 = _mm_add_epi16(xmm0, _mm_sad_epu8(_mm_loadl_epi64((__m128i*)(src + 32)), _mm_loadl_epi64((__m128i*)(ref + 32))));
                ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + srcStride)), _mm256_loadu_si256((__m256i*)(ref + refStride))));
                xmm1 = _mm_add_epi16(xmm1, _mm_sad_epu8(_mm_loadl_epi64((__m128i*)(src + srcStride + 32)), _mm_loadl_epi64((__m128i*)(ref + refStride + 32))));

                src += srcStride << 1;
                ref += refStride << 1;
        }
        ymm0 = _mm256_add_epi32(ymm0, ymm1);
        xmm0 = _mm_add_epi32(xmm0, xmm1);
        xmm0 = _mm_add_epi32(xmm0, _mm_add_epi32(_mm256_extracti128_si256(ymm0, 0), _mm256_extracti128_si256(ymm0, 1)));
        xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));
        return _mm_extract_epi32(xmm0, 0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
EB_U32 Compute48xMSad_AVX2_INTRIN(
	EB_U8  *src,        // input parameter, source samples Ptr
	EB_U32  srcStride,  // input parameter, source stride
	EB_U8  *ref,        // input parameter, reference samples Ptr
	EB_U32  refStride,  // input parameter, reference stride
	EB_U32  height,     // input parameter, block height (M)
	EB_U32  width)     // input parameter, block width (N)  
{
	__m128i xmm0, xmm1;
	__m256i ymm0, ymm1;
	EB_U32 y;
	(void)width;

	ymm0 = ymm1 = _mm256_setzero_si256();
	xmm0 = xmm1 = _mm_setzero_si128();
	for (y = 0; y < height; y += 2) {
		ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i*)ref)));
		xmm0 = _mm_add_epi32(xmm0, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + 32)), _mm_loadu_si128((__m128i*)(ref + 32))));
		ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + srcStride)), _mm256_loadu_si256((__m256i*)(ref + refStride))));
		xmm1 = _mm_add_epi32(xmm1, _mm_sad_epu8(_mm_loadu_si128((__m128i*)(src + srcStride + 32)), _mm_loadu_si128((__m128i*)(ref + refStride + 32))));
		src += srcStride << 1;
		ref += refStride << 1;
	}
	ymm0 = _mm256_add_epi32(ymm0, ymm1);
	xmm0 = _mm_add_epi32(xmm0, xmm1);
	xmm0 = _mm_add_epi32(xmm0, _mm_add_epi32(_mm256_extracti128_si256(ymm0, 0), _mm256_extracti128_si256(ymm0, 1)));
	xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));
	return _mm_extract_epi32(xmm0, 0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
EB_U32 Compute56xMSad_AVX2_INTRIN(
        EB_U8  *src,        // input parameter, source samples Ptr
        EB_U32  srcStride,  // input parameter, source stride
        EB_U8  *ref,        // input parameter, reference samples Ptr
        EB_U32  refStride,  // input parameter, reference stride
        EB_U32  height,     // input parameter, block height (M)
        EB_U32  width)     // input parameter, block width (N)
{
        __m128i xmm0, xmm1;
        __m256i ymm0, ymm1, ymm2, ymm3;
        EB_U32 y;
        (void)width;

        ymm0 = ymm1 = ymm2 = ymm3 = _mm256_setzero_si256();
        for (y = 0; y < height; y += 2) {
                ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i*)ref)));
                ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + 32)), _mm256_loadu_si256((__m256i*)(ref + 32))));
                ymm2 = _mm256_add_epi32(ymm2, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + srcStride)), _mm256_loadu_si256((__m256i*)(ref + refStride))));
                ymm3 = _mm256_add_epi32(ymm3, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + srcStride + 32)), _mm256_loadu_si256((__m256i*)(ref + refStride + 32))));

                src += srcStride << 1;
                ref += refStride << 1;
        }
        ymm0 = _mm256_add_epi32(ymm0, ymm2);
        xmm0 = _mm_add_epi32(_mm256_extracti128_si256(ymm0, 0), _mm256_extracti128_si256(ymm0, 1));

        xmm0 = _mm_add_epi32(xmm0, _mm_add_epi32(_mm256_extracti128_si256(ymm1, 0), _mm256_extracti128_si256(ymm3, 0)));
        xmm1 = _mm_add_epi32(_mm256_extracti128_si256(ymm1, 1), _mm256_extracti128_si256(ymm3, 1)); 

        xmm0 = _mm_add_epi32(_mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8)), xmm1);
       return _mm_extract_epi32(xmm0, 0);
}

/*******************************************************************************
* Requirement: height % 2 = 0
*******************************************************************************/
EB_U32 Compute64xMSad_AVX2_INTRIN(
	EB_U8  *src,        // input parameter, source samples Ptr
	EB_U32  srcStride,  // input parameter, source stride
	EB_U8  *ref,        // input parameter, reference samples Ptr
	EB_U32  refStride,  // input parameter, reference stride
	EB_U32  height,     // input parameter, block height (M)
	EB_U32  width)     // input parameter, block width (N) 
{
	__m128i xmm0;
	__m256i ymm0, ymm1, ymm2, ymm3;
	EB_U32 y;
	(void)width;

	ymm0 = ymm1 = ymm2 = ymm3 = _mm256_setzero_si256();
	for (y = 0; y < height; y += 2) {
		ymm0 = _mm256_add_epi32(ymm0, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)src), _mm256_loadu_si256((__m256i*)ref)));
		ymm1 = _mm256_add_epi32(ymm1, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + 32)), _mm256_loadu_si256((__m256i*)(ref + 32))));
		ymm2 = _mm256_add_epi32(ymm2, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + srcStride)), _mm256_loadu_si256((__m256i*)(ref + refStride))));
		ymm3 = _mm256_add_epi32(ymm3, _mm256_sad_epu8(_mm256_loadu_si256((__m256i*)(src + srcStride + 32)), _mm256_loadu_si256((__m256i*)(ref + refStride + 32))));
		src += srcStride << 1;
		ref += refStride << 1;
	}
	ymm0 = _mm256_add_epi32(_mm256_add_epi32(ymm0, ymm1), _mm256_add_epi32(ymm2, ymm3));
	xmm0 = _mm_add_epi32(_mm256_castsi256_si128(ymm0), _mm256_extracti128_si256(ymm0, 1));
	xmm0 = _mm_add_epi32(xmm0, _mm_srli_si128(xmm0, 8));
	return _mm_extract_epi32(xmm0, 0);
}

#ifdef NON_AVX512_SUPPORT

void GetEightHorizontalSearchPointResults_8x8_16x16_PU_AVX2_INTRIN(
    EB_U8   *src,
    EB_U32   srcStride,
    EB_U8   *ref,
    EB_U32   refStride,
    EB_U32  *pBestSad8x8,
    EB_U32  *pBestMV8x8,
    EB_U32  *pBestSad16x16,
    EB_U32  *pBestMV16x16,
    EB_U32   mv,
    EB_U16  *pSad16x16)
{

    EB_S16 xMv, yMv;
    __m128i s3;
    __m128i sad_0, sad_1, sad_2, sad_3;
    __m256i ss0, ss1, ss2, ss3, ss4;
    EB_U32 temSum;


    /*
    -------------------------------------   -----------------------------------
    | 8x8_00 | 8x8_01 | 8x8_04 | 8x8_05 |   8x8_16 | 8x8_17 | 8x8_20 | 8x8_21 |
    -------------------------------------   -----------------------------------
    | 8x8_02 | 8x8_03 | 8x8_06 | 8x8_07 |   8x8_18 | 8x8_19 | 8x8_22 | 8x8_23 |
    -----------------------   -----------   ----------------------   ----------
    | 8x8_08 | 8x8_09 | 8x8_12 | 8x8_13 |   8x8_24 | 8x8_25 | 8x8_29 | 8x8_29 |
    ----------------------    -----------   ---------------------    ----------
    | 8x8_10 | 8x8_11 | 8x8_14 | 8x8_15 |   8x8_26 | 8x8_27 | 8x8_30 | 8x8_31 |
    -------------------------------------   -----------------------------------

    -------------------------------------   -----------------------------------
    | 8x8_32 | 8x8_33 | 8x8_36 | 8x8_37 |   8x8_48 | 8x8_49 | 8x8_52 | 8x8_53 |
    -------------------------------------   -----------------------------------
    | 8x8_34 | 8x8_35 | 8x8_38 | 8x8_39 |   8x8_50 | 8x8_51 | 8x8_54 | 8x8_55 |
    -----------------------   -----------   ----------------------   ----------
    | 8x8_40 | 8x8_41 | 8x8_44 | 8x8_45 |   8x8_56 | 8x8_57 | 8x8_60 | 8x8_61 |
    ----------------------    -----------   ---------------------    ----------
    | 8x8_42 | 8x8_43 | 8x8_46 | 8x8_48 |   8x8_58 | 8x8_59 | 8x8_62 | 8x8_63 |
    -------------------------------------   -----------------------------------
    */

    /*
    ----------------------    ----------------------
    |  16x16_0  |  16x16_1  |  16x16_4  |  16x16_5  |
    ----------------------    ----------------------
    |  16x16_2  |  16x16_3  |  16x16_6  |  16x16_7  |
    -----------------------   -----------------------
    |  16x16_8  |  16x16_9  |  16x16_12 |  16x16_13 |
    ----------------------    ----------------------
    |  16x16_10 |  16x16_11 |  16x16_14 |  16x16_15 |
    -----------------------   -----------------------
    */

    //8x8_0 & 8x8_1
    ss0 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)ref), _mm_loadu_si128((__m128i*)(ref + 2 * refStride)));
    ss1 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)(ref + 8)), _mm_loadu_si128((__m128i*)(ref + 2 * refStride + 8)));
    ss2 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)(src + 2 * srcStride)));
    ss3 = _mm256_mpsadbw_epu8(ss0, ss2, 0);
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
    ss4 = _mm256_mpsadbw_epu8(ss1, ss2, 18);                         // 010 010
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
    src += srcStride * 4;
    ref += refStride * 4;
    ss0 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)ref), _mm_loadu_si128((__m128i*)(ref + 2 * refStride)));
    ss1 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)(ref + 8)), _mm_loadu_si128((__m128i*)(ref + 2 * refStride + 8)));
    ss2 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)(src + 2 * srcStride)));
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
    sad_0 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
    sad_1 = _mm_adds_epu16(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));

    //8x8_2 & 8x8_3
    src += srcStride * 4;
    ref += refStride * 4;
    ss0 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)ref), _mm_loadu_si128((__m128i*)(ref + 2 * refStride)));
    ss1 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)(ref + 8)), _mm_loadu_si128((__m128i*)(ref + 2 * refStride + 8)));
    ss2 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)(src + 2 * srcStride)));
    ss3 = _mm256_mpsadbw_epu8(ss0, ss2, 0);
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
    ss4 = _mm256_mpsadbw_epu8(ss1, ss2, 18);                         // 010 010
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111

    src += srcStride * 4;
    ref += refStride * 4;
    ss0 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)ref), _mm_loadu_si128((__m128i*)(ref + 2 * refStride)));
    ss1 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)(ref + 8)), _mm_loadu_si128((__m128i*)(ref + 2 * refStride + 8)));
    ss2 = _mm256_setr_m128i(_mm_loadu_si128((__m128i*)src), _mm_loadu_si128((__m128i*)(src + 2 * srcStride)));
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 0));
    ss3 = _mm256_adds_epu16(ss3, _mm256_mpsadbw_epu8(ss0, ss2, 45)); // 101 101
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 18)); // 010 010
    ss4 = _mm256_adds_epu16(ss4, _mm256_mpsadbw_epu8(ss1, ss2, 63)); // 111 111
    sad_2 = _mm_adds_epu16(_mm256_extracti128_si256(ss3, 0), _mm256_extracti128_si256(ss3, 1));
    sad_3 = _mm_adds_epu16(_mm256_extracti128_si256(ss4, 0), _mm256_extracti128_si256(ss4, 1));

    //16x16
    s3 = _mm_adds_epu16(_mm_adds_epu16(sad_0, sad_1), _mm_adds_epu16(sad_2, sad_3));
    //sotore the 8 SADs(16x8 SADs)
    _mm_store_si128((__m128i*)pSad16x16, s3);
    //find the best for 16x16
    s3 = _mm_minpos_epu16(s3);
    temSum = _mm_extract_epi16(s3, 0) << 1;
    if (temSum <  pBestSad16x16[0]) {
        pBestSad16x16[0] = temSum;
        xMv = _MVXT(mv) + (EB_S16)(_mm_extract_epi16(s3, 1) * 4);
        yMv = _MVYT(mv);
        pBestMV16x16[0] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
    }

    //find the best for 8x8_0, 8x8_1, 8x8_2 & 8x8_3
    sad_0 = _mm_minpos_epu16(sad_0);
    sad_1 = _mm_minpos_epu16(sad_1);
    sad_2 = _mm_minpos_epu16(sad_2);
    sad_3 = _mm_minpos_epu16(sad_3);
    sad_0 = _mm_unpacklo_epi16(sad_0, sad_1);
    sad_2 = _mm_unpacklo_epi16(sad_2, sad_3);
    sad_0 = _mm_unpacklo_epi32(sad_0, sad_2);
    sad_1 = _mm_unpackhi_epi16(sad_0, _mm_setzero_si128());
    sad_0 = _mm_unpacklo_epi16(sad_0, _mm_setzero_si128());
    sad_0 = _mm_slli_epi32(sad_0, 1);
    sad_1 = _mm_slli_epi16(sad_1, 2);
    sad_2 = _mm_loadu_si128((__m128i*)pBestSad8x8);
    s3 = _mm_cmpgt_epi32(sad_2, sad_0);
    sad_0 = _mm_min_epu32(sad_0, sad_2);
    _mm_storeu_si128((__m128i*)pBestSad8x8, sad_0);
    sad_3 = _mm_loadu_si128((__m128i*)pBestMV8x8);
    sad_3 = _mm_andnot_si128(s3, sad_3);
    sad_2 = _mm_set1_epi32(mv);
    sad_2 = _mm_add_epi16(sad_2, sad_1);
    sad_2 = _mm_and_si128(sad_2, s3);
    sad_2 = _mm_or_si128(sad_2, sad_3);
    _mm_storeu_si128((__m128i*)pBestMV8x8, sad_2);

}
void GetEightHorizontalSearchPointResults_32x32_64x64_PU_AVX2_INTRIN(
    EB_U16  *pSad16x16,
    EB_U32  *pBestSad32x32,
    EB_U32  *pBestSad64x64,
    EB_U32  *pBestMV32x32,
    EB_U32  *pBestMV64x64,
    EB_U32   mv)
{
    EB_S16 xMv, yMv;
    EB_U32 temSum, bestSad64x64, bestMV64x64;
    __m128i s0, s1, s2, s3, s4, s5, s6, s7, sad_0, sad_1;
    __m128i sad_00, sad_01, sad_10, sad_11, sad_20, sad_21, sad_30, sad_31;
    __m256i ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7;

    s0 = _mm_setzero_si128();
    s1 = _mm_setzero_si128();
    s2 = _mm_setzero_si128();
    s3 = _mm_setzero_si128();
    s4 = _mm_setzero_si128();
    s5 = _mm_setzero_si128();
    s6 = _mm_setzero_si128();
    s7 = _mm_setzero_si128();
    sad_0 = _mm_setzero_si128();
    sad_1 = _mm_setzero_si128();

    sad_00 = _mm_setzero_si128();
    sad_01 = _mm_setzero_si128();
    sad_10 = _mm_setzero_si128();
    sad_11 = _mm_setzero_si128();
    sad_20 = _mm_setzero_si128();
    sad_21 = _mm_setzero_si128();
    sad_30 = _mm_setzero_si128();
    sad_31 = _mm_setzero_si128();

    ss0 = _mm256_setzero_si256();
    ss1 = _mm256_setzero_si256();
    ss2 = _mm256_setzero_si256();
    ss3 = _mm256_setzero_si256();
    ss4 = _mm256_setzero_si256();
    ss5 = _mm256_setzero_si256();
    ss6 = _mm256_setzero_si256();
    ss7 = _mm256_setzero_si256();


    /*--------------------
    |  32x32_0  |  32x32_1
    ----------------------
    |  32x32_2  |  32x32_3
    ----------------------*/


    /*  data ordering in pSad16x16 buffer

    Search    Search            Search
    Point 0   Point 1           Point 7
    ---------------------------------------
    16x16_0    |    x    |    x    | ...... |    x    |
    ---------------------------------------
    16x16_1    |    x    |    x    | ...... |    x    |

    16x16_n    |    x    |    x    | ...... |    x    |

    ---------------------------------------
    16x16_15   |    x    |    x    | ...... |    x    |
    ---------------------------------------
    */

    //    __m128i Zero = _mm_setzero_si128();

    //32x32_0
    s0 = _mm_loadu_si128((__m128i*)(pSad16x16 + 0 * 8));
    s1 = _mm_loadu_si128((__m128i*)(pSad16x16 + 1 * 8));
    s2 = _mm_loadu_si128((__m128i*)(pSad16x16 + 2 * 8));
    s3 = _mm_loadu_si128((__m128i*)(pSad16x16 + 3 * 8));

    s4 = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0 = _mm_add_epi32(s4, s6);
    s1 = _mm_add_epi32(s5, s7);

    s4 = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2 = _mm_add_epi32(s4, s6);
    s3 = _mm_add_epi32(s5, s7);

    sad_01 = _mm_add_epi32(s0, s2);
    sad_00 = _mm_add_epi32(s1, s3);

    //32x32_1
    s0 = _mm_loadu_si128((__m128i*)(pSad16x16 + 4 * 8));
    s1 = _mm_loadu_si128((__m128i*)(pSad16x16 + 5 * 8));
    s2 = _mm_loadu_si128((__m128i*)(pSad16x16 + 6 * 8));
    s3 = _mm_loadu_si128((__m128i*)(pSad16x16 + 7 * 8));

    s4 = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0 = _mm_add_epi32(s4, s6);
    s1 = _mm_add_epi32(s5, s7);

    s4 = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2 = _mm_add_epi32(s4, s6);
    s3 = _mm_add_epi32(s5, s7);

    sad_11 = _mm_add_epi32(s0, s2);
    sad_10 = _mm_add_epi32(s1, s3);

    //32x32_2
    s0 = _mm_loadu_si128((__m128i*)(pSad16x16 + 8 * 8));
    s1 = _mm_loadu_si128((__m128i*)(pSad16x16 + 9 * 8));
    s2 = _mm_loadu_si128((__m128i*)(pSad16x16 + 10 * 8));
    s3 = _mm_loadu_si128((__m128i*)(pSad16x16 + 11 * 8));

    s4 = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0 = _mm_add_epi32(s4, s6);
    s1 = _mm_add_epi32(s5, s7);

    s4 = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2 = _mm_add_epi32(s4, s6);
    s3 = _mm_add_epi32(s5, s7);

    sad_21 = _mm_add_epi32(s0, s2);
    sad_20 = _mm_add_epi32(s1, s3);

    //32x32_3
    s0 = _mm_loadu_si128((__m128i*)(pSad16x16 + 12 * 8));
    s1 = _mm_loadu_si128((__m128i*)(pSad16x16 + 13 * 8));
    s2 = _mm_loadu_si128((__m128i*)(pSad16x16 + 14 * 8));
    s3 = _mm_loadu_si128((__m128i*)(pSad16x16 + 15 * 8));

    s4 = _mm_unpackhi_epi16(s0, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s0, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s1, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s1, _mm_setzero_si128());
    s0 = _mm_add_epi32(s4, s6);
    s1 = _mm_add_epi32(s5, s7);

    s4 = _mm_unpackhi_epi16(s2, _mm_setzero_si128());
    s5 = _mm_unpacklo_epi16(s2, _mm_setzero_si128());
    s6 = _mm_unpackhi_epi16(s3, _mm_setzero_si128());
    s7 = _mm_unpacklo_epi16(s3, _mm_setzero_si128());
    s2 = _mm_add_epi32(s4, s6);
    s3 = _mm_add_epi32(s5, s7);

    sad_31 = _mm_add_epi32(s0, s2);
    sad_30 = _mm_add_epi32(s1, s3);

    sad_0 = _mm_add_epi32(_mm_add_epi32(sad_00, sad_10), _mm_add_epi32(sad_20, sad_30));
    sad_1 = _mm_add_epi32(_mm_add_epi32(sad_01, sad_11), _mm_add_epi32(sad_21, sad_31));
    sad_0 = _mm_slli_epi32(sad_0, 1);
    sad_1 = _mm_slli_epi32(sad_1, 1);

    bestSad64x64 = pBestSad64x64[0];
    bestMV64x64 = 0;
    //sad_0
    temSum = _mm_extract_epi32(sad_0, 0);
    if (temSum < bestSad64x64) {
        bestSad64x64 = temSum;
    }
    temSum = _mm_extract_epi32(sad_0, 1);
    if (temSum < bestSad64x64) {
        bestSad64x64 = temSum;
        bestMV64x64 = 1 * 4;
    }
    temSum = _mm_extract_epi32(sad_0, 2);
    if (temSum < bestSad64x64) {
        bestSad64x64 = temSum;
        bestMV64x64 = 2 * 4;
    }
    temSum = _mm_extract_epi32(sad_0, 3);
    if (temSum < bestSad64x64) {
        bestSad64x64 = temSum;
        bestMV64x64 = 3 * 4;
    }

    //sad_1
    temSum = _mm_extract_epi32(sad_1, 0);
    if (temSum < bestSad64x64) {
        bestSad64x64 = temSum;
        bestMV64x64 = 4 * 4;
    }
    temSum = _mm_extract_epi32(sad_1, 1);
    if (temSum < bestSad64x64) {
        bestSad64x64 = temSum;
        bestMV64x64 = 5 * 4;
    }
    temSum = _mm_extract_epi32(sad_1, 2);
    if (temSum < bestSad64x64) {
        bestSad64x64 = temSum;
        bestMV64x64 = 6 * 4;
    }
    temSum = _mm_extract_epi32(sad_1, 3);
    if (temSum < bestSad64x64) {
        bestSad64x64 = temSum;
        bestMV64x64 = 7 * 4;
    }
    if (pBestSad64x64[0] != bestSad64x64) {
        pBestSad64x64[0] = bestSad64x64;
        xMv = _MVXT(mv) + (EB_S16)bestMV64x64;  yMv = _MVYT(mv);
        pBestMV64x64[0] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
    }

    // ****CODE PAST HERE IS BUGGY FOR GCC****

    // XY
    // X: 32x32 block [0..3]
    // Y: Search position [0..7]
    ss0 = _mm256_setr_m128i(sad_00, sad_01); // 07 06 05 04  03 02 01 00
    ss1 = _mm256_setr_m128i(sad_10, sad_11); // 17 16 15 14  13 12 11 10
    ss2 = _mm256_setr_m128i(sad_20, sad_21); // 27 26 25 24  23 22 21 20
    ss3 = _mm256_setr_m128i(sad_30, sad_31); // 37 36 35 34  33 32 31 30
    ss4 = _mm256_unpacklo_epi32(ss0, ss1);   // 15 05 14 04  11 01 10 00
    ss5 = _mm256_unpacklo_epi32(ss2, ss3);   // 35 25 34 24  31 21 30 20
    ss6 = _mm256_unpackhi_epi32(ss0, ss1);   // 17 07 16 06  13 03 12 02
    ss7 = _mm256_unpackhi_epi32(ss2, ss3);   // 37 27 36 26  33 23 32 22
    ss0 = _mm256_unpacklo_epi64(ss4, ss5);   // 34 24 14 04  30 20 10 00
    ss1 = _mm256_unpackhi_epi64(ss4, ss5);   // 35 25 15 05  31 21 11 01
    ss2 = _mm256_unpacklo_epi64(ss6, ss7);   // 36 26 16 06  32 22 12 02
    ss3 = _mm256_unpackhi_epi64(ss6, ss7);   // 37 27 17 07  33 23 13 03

                                             //   ss4   |  ss5-2  |                ss6        |
                                             // a0 : a1 | a2 : a3 | min(a0, a1) : min(a2, a3) |    | (ss4 & !ss6) | ((ss5-2) & ss6) | ((ss4 & !ss6) | ((ss5-2) & ss6)) |
                                             // > (-1)  | >  (-3) |         >     (-1)        | a3 |       0      |       -3        |              -3                  |
                                             // > (-1)  | >  (-3) |         <=     (0)        | a1 |      -1      |        0        |              -1                  |
                                             // > (-1)  | <= (-2) |         >     (-1)        | a2 |       0      |       -2        |              -2                  |
                                             // > (-1)  | <= (-2) |         <=     (0)        | a1 |      -1      |        0        |              -1                  |
                                             // <= (0)  | >  (-3) |         >     (-1)        | a3 |       0      |       -3        |              -3                  |
                                             // <= (0)  | >  (-3) |         <=     (0)        | a0 |       0      |        0        |               0                  |
                                             // <= (0)  | <= (-2) |         >     (-1)        | a2 |       0      |       -2        |              -2                  |
                                             // <= (0)  | <= (-2) |         <=     (0)        | a0 |       0      |        0        |               0                  |

                                             // *** 8 search points per position ***

                                             // ss0: Search Pos 0,4 for blocks 0,1,2,3
                                             // ss1: Search Pos 1,5 for blocks 0,1,2,3
                                             // ss2: Search Pos 2,6 for blocks 0,1,2,3
                                             // ss3: Search Pos 3,7 for blocks 0,1,2,3

    ss4 = _mm256_cmpgt_epi32(ss0, ss1);
    // not different printf("%d\n", _mm_extract_epi32(_mm256_extracti128_si256(ss4, 0), 0)); // DEBUG
    //ss4 = _mm256_or_si256(_mm256_cmpgt_epi32(ss0, ss1), _mm256_cmpeq_epi32(ss0, ss1));
    ss0 = _mm256_min_epi32(ss0, ss1);
    ss5 = _mm256_cmpgt_epi32(ss2, ss3);
    //ss5 = _mm256_or_si256(_mm256_cmpgt_epi32(ss2, ss3), _mm256_cmpeq_epi32(ss2, ss3));
    ss2 = _mm256_min_epi32(ss2, ss3);
    ss5 = _mm256_sub_epi32(ss5, _mm256_set1_epi32(2)); // ss5-2


                                                       // *** 4 search points per position ***
    ss6 = _mm256_cmpgt_epi32(ss0, ss2);
    //ss6 = _mm256_or_si256(_mm256_cmpgt_epi32(ss0, ss2), _mm256_cmpeq_epi32(ss0, ss2));
    ss0 = _mm256_min_epi32(ss0, ss2);
    ss5 = _mm256_and_si256(ss5, ss6); // (ss5-2) & ss6
    ss4 = _mm256_andnot_si256(ss6, ss4); // ss4 & !ss6
    ss4 = _mm256_or_si256(ss4, ss5); // (ss4 & !ss6) | ((ss5-2) & ss6)

                                     // *** 2 search points per position ***
    s0 = _mm_setzero_si128();
    s1 = _mm_setzero_si128();
    s2 = _mm_setzero_si128();
    s3 = _mm_setzero_si128();
    s4 = _mm_setzero_si128();
    s5 = _mm_setzero_si128();
    s6 = _mm_setzero_si128();
    s7 = _mm_setzero_si128();

    // ss0 = 8 SADs, two search points for each 32x32
    // ss4 = 8 MVs, two search points for each 32x32
    //
    // XY
    // X: 32x32 block [0..3]
    // Y: search position [0..1]
    // Format: 00 10 20 30  01 11 21 31

    // Each 128 bits contains 4 32x32 32bit block results
#ifdef __GNUC__
    // SAD
    s0 = _mm256_extracti128_si256(ss0, 1);
    s1 = _mm256_extracti128_si256(ss0, 0);
    // MV
    s2 = _mm256_extracti128_si256(ss4, 1);
    s3 = _mm256_extracti128_si256(ss4, 0);
#else
    // SAD
    s0 = _mm256_extracti128_si256(ss0, 0);
    s1 = _mm256_extracti128_si256(ss0, 1);
    // MV
    s2 = _mm256_extracti128_si256(ss4, 0);
    s3 = _mm256_extracti128_si256(ss4, 1);
#endif

    //// Should be fine
    //printf("sad0 %d, %d, %d, %d\n", _mm_extract_epi32(s0, 0), _mm_extract_epi32(s0, 1), _mm_extract_epi32(s0, 2), _mm_extract_epi32(s0, 3)); // DEBUG
    //printf("sad1 %d, %d, %d, %d\n", _mm_extract_epi32(s1, 0), _mm_extract_epi32(s1, 1), _mm_extract_epi32(s1, 2), _mm_extract_epi32(s1, 3)); // DEBUG
    //printf("mv0 %d, %d, %d, %d\n", _mm_extract_epi32(s2, 0), _mm_extract_epi32(s2, 1), _mm_extract_epi32(s2, 2), _mm_extract_epi32(s2, 3)); // DEBUG
    //printf("mv1 %d, %d, %d, %d\n", _mm_extract_epi32(s3, 0), _mm_extract_epi32(s3, 1), _mm_extract_epi32(s3, 2), _mm_extract_epi32(s3, 3)); // DEBUG


    // Choose the best MV out of the two, use s4 to hold results of min
    s4 = _mm_cmpgt_epi32(s0, s1);

    // DIFFERENT BETWEEN VS AND GCC
    // printf("%d, %d, %d, %d\n", _mm_extract_epi32(s4, 0), _mm_extract_epi32(s4, 1), _mm_extract_epi32(s4, 2), _mm_extract_epi32(s4, 3)); // DEBUG

    //s4 = _mm_or_si128(_mm_cmpgt_epi32(s0, s1), _mm_cmpeq_epi32(s0, s1));
    s0 = _mm_min_epi32(s0, s1);



    // Extract MV's based on the blocks to s2
    s3 = _mm_sub_epi32(s3, _mm_set1_epi32(4)); // s3-4
                                               // Remove the MV's are not used from s2
    s2 = _mm_andnot_si128(s4, s2);
    // Remove the MV's that are not used from s3 (inverse from s2 above operation)
    s3 = _mm_and_si128(s4, s3);
    // Combine the remaining candidates into s2
    s2 = _mm_or_si128(s2, s3);
    // Convert MV's into encoders format
    s2 = _mm_sub_epi32(_mm_setzero_si128(), s2);
    s2 = _mm_slli_epi32(s2, 2); // mv info


                                // ***SAD***
                                // s0: current SAD candidates for each 32x32
                                // s1: best SAD's for 32x32

                                // << 1 to compensate for every other line
    s0 = _mm_slli_epi32(s0, 1); // best sad info
                                // Load best SAD's
    s1 = _mm_loadu_si128((__m128i*)pBestSad32x32);

    // Determine which candidates are better than the current best SAD's.
    // s4 is used to determine the MV's of the new best SAD's
    s4 = _mm_cmpgt_epi32(s1, s0);
    // not different printf("%d, %d, %d, %d\n", _mm_extract_epi32(s4, 0), _mm_extract_epi32(s4, 1), _mm_extract_epi32(s4, 2), _mm_extract_epi32(s4, 3)); // DEBUG
    //s4 = _mm_or_si128(_mm_cmpgt_epi32(s1, s0), _mm_cmpeq_epi32(s1, s0));
    // Combine old and new min SAD's
    s0 = _mm_min_epu32(s0, s1);
    // Store new best SAD's back to memory
    _mm_storeu_si128((__m128i*)pBestSad32x32, s0);


    // ***Motion Vectors***
    // Load best MV's
    // s3: candidate MV's
    // s4: results of comparing SAD's
    // s5: previous best MV's

    // Load previous best MV's
    s5 = _mm_loadu_si128((__m128i*)pBestMV32x32);
    // Remove the MV's that are being replaced
    s5 = _mm_andnot_si128(s4, s5);
    // Set s3 to the base MV
    s3 = _mm_set1_epi32(mv);
    // Add candidate MV's to base MV
    s3 = _mm_add_epi16(s3, s2);
    // Remove non-candidate's
    s3 = _mm_and_si128(s3, s4);
    // Combine remaining candidates with remaining best MVs
    s3 = _mm_or_si128(s3, s5);
    // Store back to memory
    _mm_storeu_si128((__m128i*)pBestMV32x32, s3);
}

#endif
