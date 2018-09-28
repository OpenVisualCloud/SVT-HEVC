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
			ss0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)(pRef + 2 * refStride))), _mm_loadu_si128((__m128i*)pRef), 0x1);
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

