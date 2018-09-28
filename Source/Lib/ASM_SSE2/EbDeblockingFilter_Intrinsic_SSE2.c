/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "emmintrin.h"
#include "EbTypes.h"

#include "EbDeblockingFilter_SSE2.h"

void Chroma2SampleEdgeDLFCore16bit_SSE2_INTRIN(
    EB_U16				  *edgeStartSampleCb,
    EB_U16				  *edgeStartSampleCr,
    EB_U32                 reconChromaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_U8                  cbTc,
    EB_U8                  crTc)
{    
    __m128i xmm_tcN = _mm_setzero_si128();
    __m128i xmm0 = _mm_setzero_si128();
    __m128i xmm_4 = _mm_set1_epi16(4);
    __m128i xmm_Max10bit = _mm_set1_epi16(0x03FF);
    __m128i xmm_p0, xmm_p1, xmm_q0, xmm_q1, xmm_p0Cr, xmm_temp1, xmm_delta;
    __m128i xmm_Tc = _mm_unpacklo_epi8(_mm_cvtsi32_si128(cbTc), _mm_cvtsi32_si128(crTc));
    xmm_Tc = _mm_unpacklo_epi8(xmm_Tc, xmm_Tc);
    xmm_Tc = _mm_unpacklo_epi8(xmm_Tc, xmm0);
    xmm_Tc = _mm_unpacklo_epi64(xmm_Tc, xmm_Tc);
    xmm_tcN = _mm_sub_epi16(xmm_tcN, xmm_Tc);

    if (0 == isVerticalEdge) {

        xmm_p0 = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i *)(edgeStartSampleCb -  reconChromaPicStride)), _mm_loadl_epi64((__m128i *)(edgeStartSampleCr - reconChromaPicStride)));
        xmm_p1 = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i *)(edgeStartSampleCb -  (reconChromaPicStride << 1))), _mm_loadl_epi64((__m128i *)(edgeStartSampleCr - (reconChromaPicStride << 1))));
        xmm_q0 = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i *)(edgeStartSampleCb)), _mm_loadl_epi64((__m128i *)(edgeStartSampleCr)));
        xmm_q1 = _mm_unpacklo_epi32(_mm_loadl_epi64((__m128i *)(edgeStartSampleCb +  reconChromaPicStride)), _mm_loadl_epi64((__m128i *)(edgeStartSampleCr + reconChromaPicStride)));
                
        xmm_temp1 = _mm_srai_epi16(_mm_add_epi16(_mm_slli_epi16(_mm_sub_epi16(xmm_q0, xmm_p0), 2),  _mm_add_epi16(_mm_sub_epi16(xmm_p1, xmm_q1), xmm_4)), 3);
        xmm_delta = _mm_max_epi16(xmm_temp1, xmm_tcN);
        xmm_delta = _mm_min_epi16(xmm_delta, xmm_Tc);

        xmm_p0 = _mm_unpacklo_epi64(_mm_add_epi16(xmm_p0, xmm_delta), _mm_sub_epi16(xmm_q0, xmm_delta));
        xmm_p0 = _mm_max_epi16(_mm_min_epi16(xmm_p0, xmm_Max10bit), xmm0);

        *(EB_U32 *)(edgeStartSampleCb - reconChromaPicStride) = _mm_cvtsi128_si32(xmm_p0);
        *(EB_U32 *)(edgeStartSampleCr - reconChromaPicStride) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_p0, 4));
        *(EB_U32 *)(edgeStartSampleCb) = _mm_cvtsi128_si32( _mm_srli_si128(xmm_p0, 8));
        *(EB_U32 *)(edgeStartSampleCr) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_p0, 12));
    }
    else {
        xmm_p0 = _mm_unpacklo_epi16(_mm_loadl_epi64((__m128i *)(edgeStartSampleCb - 2)), _mm_loadl_epi64((__m128i *)(edgeStartSampleCb + reconChromaPicStride - 2)));       
        xmm_p0Cr = _mm_unpacklo_epi16(_mm_loadl_epi64((__m128i *)(edgeStartSampleCr - 2)), _mm_loadl_epi64((__m128i *)(edgeStartSampleCr + reconChromaPicStride - 2))); 
  
        xmm_q0 = _mm_unpackhi_epi32(xmm_p0, xmm_p0Cr); 
        xmm_p0 = _mm_unpacklo_epi32(xmm_p0, xmm_p0Cr);                       
        xmm_p0 = _mm_unpacklo_epi64(_mm_srli_si128(xmm_p0, 8), xmm_p0);    
                                   
        xmm_temp1 = _mm_sub_epi16(xmm_q0, xmm_p0);     
        xmm_temp1 = _mm_srai_epi16(_mm_add_epi16(_mm_sub_epi16(_mm_slli_epi16(xmm_temp1, 2), _mm_srli_si128(xmm_temp1, 8)), xmm_4), 3);           
        xmm_delta = _mm_min_epi16(_mm_max_epi16(xmm_temp1, xmm_tcN), xmm_Tc);

        xmm_p0 = _mm_unpacklo_epi16(_mm_add_epi16(xmm_p0, xmm_delta), _mm_sub_epi16(xmm_q0, xmm_delta)); 
        xmm_p0 = _mm_max_epi16(_mm_min_epi16(xmm_p0, xmm_Max10bit), xmm0); 

        *(EB_U32*)(edgeStartSampleCb-1) = _mm_cvtsi128_si32(xmm_p0);
        *(EB_U32*)(edgeStartSampleCb+reconChromaPicStride-1) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_p0, 4));
        *(EB_U32*)(edgeStartSampleCr-1) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_p0, 8));
        *(EB_U32*)(edgeStartSampleCr+reconChromaPicStride-1) = _mm_cvtsi128_si32(_mm_srli_si128(xmm_p0, 12));
    }

    return;
}
