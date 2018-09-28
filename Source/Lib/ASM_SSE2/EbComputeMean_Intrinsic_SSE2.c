/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbTypes.h"
#include "emmintrin.h"
#include "EbComputeMean_SSE2.h"

EB_U64 ComputeSubdMeanOfSquaredValues8x8_SSE2_INTRIN(
	EB_U8 *  inputSamples,      // input parameter, input samples Ptr
	EB_U16   inputStride)       // input parameter, input stride

{
	__m128i xmm0, xmm_blockMean, xmm_input;

	xmm0 = _mm_setzero_si128();
	xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)inputSamples), xmm0);
	xmm_blockMean = _mm_madd_epi16(xmm_input, xmm_input);

	/*xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples + inputStride)), xmm0);
	xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));*/

	xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+2*inputStride)), xmm0);
	xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

	/*xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+3*inputStride)), xmm0);
	xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));*/

	xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+4*inputStride)), xmm0);
	xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

	//xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+5*inputStride)), xmm0);
	//xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

	xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+6*inputStride)), xmm0);
	xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

	/*xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+7*inputStride)), xmm0);
	xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));*/

	xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_srli_si128(xmm_blockMean, 8));
	xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_srli_si128(xmm_blockMean, 4));


	return (EB_U64)_mm_cvtsi128_si32(xmm_blockMean) << 11;


		

}

EB_U64 ComputeSubMean8x8_SSE2_INTRIN(
	EB_U8 *  inputSamples,      // input parameter, input samples Ptr
	EB_U16   inputStride)       // input parameter, input stride

{

	__m128i xmm0 = _mm_setzero_si128(), xmm1, xmm3, xmm_sum1, xmm_sum2;

	xmm1 = _mm_sad_epu8(_mm_loadl_epi64((__m128i *)(inputSamples)), xmm0);
	//xmm2 = _mm_sad_epu8(_mm_loadl_epi64((__m128i *)(inputSamples + inputStride)), xmm0);
	xmm3 = _mm_sad_epu8(_mm_loadl_epi64((__m128i *)(inputSamples + 2 * inputStride)), xmm0);
	//xmm4 = _mm_sad_epu8(_mm_loadl_epi64((__m128i *)(inputSamples + 3 * inputStride)), xmm0);
	xmm_sum1 = _mm_add_epi16(xmm1,xmm3);

	inputSamples += 4 * inputStride;
	xmm1 = _mm_sad_epu8(_mm_loadl_epi64((__m128i *)(inputSamples)), xmm0);
	//xmm2 = _mm_sad_epu8(_mm_loadl_epi64((__m128i *)(inputSamples + inputStride)), xmm0);
	xmm3 = _mm_sad_epu8(_mm_loadl_epi64((__m128i *)(inputSamples + 2 * inputStride)), xmm0);
	//xmm4 = _mm_sad_epu8(_mm_loadl_epi64((__m128i *)(inputSamples + 3 * inputStride)), xmm0);
	xmm_sum2 = _mm_add_epi16(xmm1, xmm3);
	xmm_sum2 = _mm_add_epi16(xmm_sum1, xmm_sum2);

	return (EB_U64)_mm_cvtsi128_si32(xmm_sum2) << 3;

}



EB_U64 ComputeMeanOfSquaredValues8x8_SSE2_INTRIN(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight)   // input parameter, input area height
{
    __m128i xmm0, xmm_blockMean, xmm_input;
    (void)inputAreaWidth;
    (void)inputAreaHeight;
    xmm0 = _mm_setzero_si128();
    xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)inputSamples), xmm0);
    xmm_blockMean = _mm_madd_epi16(xmm_input, xmm_input);
    
    xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples + inputStride)), xmm0);
    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));
    
    xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+2*inputStride)), xmm0);
    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

    xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+3*inputStride)), xmm0);
    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

    xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+4*inputStride)), xmm0);
    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

    xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+5*inputStride)), xmm0);
    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

    xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+6*inputStride)), xmm0);
    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));
       
    xmm_input = _mm_unpacklo_epi8(_mm_loadl_epi64((__m128i *)(inputSamples+7*inputStride)), xmm0);
    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_madd_epi16(xmm_input, xmm_input));

    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_srli_si128(xmm_blockMean, 8));
    xmm_blockMean = _mm_add_epi32(xmm_blockMean, _mm_srli_si128(xmm_blockMean, 4));
 
    return (EB_U64)_mm_cvtsi128_si32(xmm_blockMean) << 10;
}
