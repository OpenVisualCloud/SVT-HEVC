/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include "tmmintrin.h"

// Putting an #include here for EbIntraPrediction_SSSE3.h causes overloading errors

EB_EXTERN void IntraModeDCChroma16bit_SSSE3_INTRIN(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    const EB_U16   *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 pStride = skip ? (predictionBufferStride << 1) : predictionBufferStride;

    if (size == 4) {
        __m128i sum = _mm_setr_epi16(4, 0, 0, 0, 0, 0, 0, 0);
        sum = _mm_add_epi16(sum, _mm_loadl_epi64((__m128i *)refSamples));
        sum = _mm_add_epi16(sum, _mm_loadl_epi64((__m128i *)(refSamples+topOffset)));
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_srli_epi16(sum, 3);
        sum = _mm_unpacklo_epi16(sum, sum);
        sum = _mm_unpacklo_epi32(sum, sum);
        _mm_storel_epi64((__m128i *)predictionPtr, sum);
        predictionPtr += pStride;
        _mm_storel_epi64((__m128i *)predictionPtr, sum);
        if (!skip) {
            predictionPtr += pStride;
            _mm_storel_epi64((__m128i *)predictionPtr, sum);
            predictionPtr += pStride;
            _mm_storel_epi64((__m128i *)predictionPtr, sum);
        }
    }
    else if (size == 8) {
        __m128i sum = _mm_setr_epi16(8, 0, 0, 0, 0, 0, 0, 0);
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)refSamples));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+topOffset)));
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_srli_epi16(sum, 4);
        sum = _mm_unpacklo_epi16(sum, sum);
        sum = _mm_unpacklo_epi32(sum, sum);
        sum = _mm_unpacklo_epi64(sum, sum);
        _mm_storeu_si128((__m128i *)predictionPtr, sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
        predictionPtr += 2*pStride;
        _mm_storeu_si128((__m128i *)predictionPtr, sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
        if (!skip) {
            predictionPtr += 2*pStride;
            _mm_storeu_si128((__m128i *)predictionPtr, sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
            predictionPtr += 2*pStride;
            _mm_storeu_si128((__m128i *)predictionPtr, sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
        }
    }
    else if (size == 16 ) {
        __m128i sum = _mm_setr_epi16(16, 0, 0, 0, 0, 0, 0, 0);
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)refSamples));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+8)));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+topOffset)));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+topOffset+8)));
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_srli_epi16(sum, 5);
        sum = _mm_unpacklo_epi16(sum, sum);
        sum = _mm_unpacklo_epi32(sum, sum);
        sum = _mm_unpacklo_epi64(sum, sum);
        _mm_storeu_si128((__m128i *)predictionPtr, sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
        predictionPtr += 2*pStride;
        _mm_storeu_si128((__m128i *)predictionPtr, sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
        predictionPtr += 2*pStride;
        _mm_storeu_si128((__m128i *)predictionPtr, sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
        predictionPtr += 2*pStride;
        _mm_storeu_si128((__m128i *)predictionPtr, sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
        _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
        if (!skip) {
            predictionPtr += 2*pStride;
            _mm_storeu_si128((__m128i *)predictionPtr, sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
            predictionPtr += 2*pStride;
            _mm_storeu_si128((__m128i *)predictionPtr, sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
            predictionPtr += 2*pStride;
            _mm_storeu_si128((__m128i *)predictionPtr, sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
            predictionPtr += 2*pStride;
            _mm_storeu_si128((__m128i *)predictionPtr, sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
        }
    } else { ///* if (size == 32) {*/
        __m128i sum = _mm_setr_epi16(32, 0, 0, 0, 0, 0, 0, 0);
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)refSamples));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+8)));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+16)));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+24)));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+topOffset)));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+topOffset+8)));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+topOffset+16)));
        sum = _mm_add_epi16(sum, _mm_loadu_si128((__m128i *)(refSamples+topOffset+24)));
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_hadd_epi16(sum, sum);
        sum = _mm_srli_epi16(sum, 6);
        sum = _mm_unpacklo_epi16(sum, sum);
        sum = _mm_unpacklo_epi32(sum, sum);
        sum = _mm_unpacklo_epi64(sum, sum);
        for (unsigned int i=0; i<(EB_U32)(skip ? 8: 16); i++) {
            _mm_storeu_si128((__m128i *)predictionPtr, sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+8), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+16), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+24), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride+8), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride+16), sum);
            _mm_storeu_si128((__m128i *)(predictionPtr+pStride+24), sum);
            predictionPtr += 2 * pStride;
        }
    }
}
