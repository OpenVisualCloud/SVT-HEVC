/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbNoiseExtractAVX2.h"
#include "EbDefinitions.h"
#include "immintrin.h"
#include "EbUtility.h"

EB_EXTERN EB_ALIGN(16) const EB_U8 filterType[] = {
	1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4
};

EB_EXTERN EB_ALIGN(16) const EB_U8 WeakChromafilter[2][32] = {
		{ 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4 },
		{ 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2 },
};

inline void lumaWeakFilter_AVX2_INTRIN(

	__m256i						top,
	__m256i						curr,
	__m256i						bottom,
	__m256i						currPrev,
	__m256i						currNext,
	EB_U8					   *ptrDenoised,
	EB_U8					    *ptrNoise
	)
{
	__m256i  topFirstHalf, bottomFirstHalf,
		filterFirstHalf, filterSecondHalf,
		currNextFirstHalf, currNextSecondHalf,
		weights, currLeftMidFirstHalfWeight,
		currLeftMidFirstHalflo, currLeftMidFirstHalfhi, currPrevPermutation, currPermutation, currNextPermutation,
		topPermutation, bottomPermutation;

	currPrevPermutation = _mm256_permute4x64_epi64(currPrev, 216);
	currPermutation = _mm256_permute4x64_epi64(curr, 216);
	currLeftMidFirstHalflo = _mm256_unpacklo_epi8(currPrevPermutation, currPermutation);
	weights = _mm256_loadu_si256((__m256i*)filterType);
	currLeftMidFirstHalfWeight = _mm256_maddubs_epi16(currLeftMidFirstHalflo, weights);
	currNextPermutation = _mm256_permute4x64_epi64(currNext, 88);
	currNextFirstHalf = _mm256_unpacklo_epi8(currNextPermutation, _mm256_setzero_si256());
	currLeftMidFirstHalflo = _mm256_add_epi16(currNextFirstHalf, currLeftMidFirstHalfWeight);

	currLeftMidFirstHalfhi = _mm256_unpackhi_epi8(currPrevPermutation, currPermutation);
	currLeftMidFirstHalfWeight = _mm256_maddubs_epi16(currLeftMidFirstHalfhi, weights);
	currNextPermutation = _mm256_permute4x64_epi64(currNext, 216);
	currNextSecondHalf = _mm256_unpackhi_epi8(currNextPermutation, _mm256_setzero_si256());
	currLeftMidFirstHalfhi = _mm256_add_epi16(currNextSecondHalf, currLeftMidFirstHalfWeight);


	topPermutation = _mm256_permute4x64_epi64(top, 216);
	topFirstHalf = _mm256_unpacklo_epi8(topPermutation, _mm256_setzero_si256());
	bottomPermutation = _mm256_permute4x64_epi64(bottom, 216);
	bottomFirstHalf = _mm256_unpacklo_epi8(bottomPermutation, _mm256_setzero_si256());
	filterFirstHalf = _mm256_adds_epi16(_mm256_adds_epi16(bottomFirstHalf, topFirstHalf), currLeftMidFirstHalflo);
	filterFirstHalf = _mm256_srli_epi16(filterFirstHalf, 3);


	topFirstHalf = _mm256_unpackhi_epi8(topPermutation, _mm256_setzero_si256());
	bottomFirstHalf = _mm256_unpackhi_epi8(bottomPermutation, _mm256_setzero_si256());
	filterSecondHalf = _mm256_adds_epi16(_mm256_adds_epi16(bottomFirstHalf, topFirstHalf), currLeftMidFirstHalfhi);
	filterSecondHalf = _mm256_srli_epi16(filterSecondHalf, 3);

	filterFirstHalf = _mm256_permute4x64_epi64(_mm256_packus_epi16(filterFirstHalf, filterSecondHalf), 216);
	_mm256_storeu_si256((__m256i *)(ptrDenoised ), filterFirstHalf);

	_mm256_storeu_si256((__m256i *)(ptrNoise), _mm256_subs_epu8(curr, filterFirstHalf));

}
inline void chromaWeakLumaStrongFilter_AVX2_INTRIN(

	__m256i						top,
	__m256i						curr,
	__m256i						bottom,
	__m256i						currPrev,
	__m256i						currNext,
	__m256i						topPrev,
	__m256i						topNext,
	__m256i						bottomPrev,
	__m256i						bottomNext,
	EB_U8					   *ptrDenoised
	)
{
	__m256i filterFirstHalf, filterSecondHalf,
		currNextFirstHalf, currNextSecondHalf,
		weights, currLeftMidFirstHalfWeight,
		currLeftMidFirstHalflo, currLeftMidFirstHalfhi, currPrevPermutation, currPermutation, currNextPermutation,
		topPermutation, bottomPermutation,
		topPrevPermutation, topLeftMidFirstHalflo, topLeftMidFirstHalfWeight, topNextFirstHalf,
		topNextPermutation, topLeftMidFirstHalfhi, topNextSecondHalf,
		bottomPrevPermutation, bottomLeftMidFirstHalflo, bottomLeftMidFirstHalfWeight, bottomNextPermutation,
		bottomNextFirstHalf, bottomLeftMidFirstHalfhi, bottomNextSecondHalf;


	//  Curr
	currPrevPermutation = _mm256_permute4x64_epi64(currPrev, 216);
	currPermutation = _mm256_permute4x64_epi64(curr, 216);
	currLeftMidFirstHalflo = _mm256_unpacklo_epi8(currPrevPermutation, currPermutation);
	weights = _mm256_loadu_si256((__m256i*)WeakChromafilter[0]);
	currLeftMidFirstHalfWeight = _mm256_maddubs_epi16(currLeftMidFirstHalflo, weights);
	currNextPermutation = _mm256_permute4x64_epi64(currNext, 88);
	currNextFirstHalf = _mm256_unpacklo_epi8(currNextPermutation, _mm256_setzero_si256());
	currNextFirstHalf = _mm256_slli_epi16(currNextFirstHalf, 1);
	currLeftMidFirstHalflo = _mm256_add_epi16(currNextFirstHalf, currLeftMidFirstHalfWeight);

	currLeftMidFirstHalfhi = _mm256_unpackhi_epi8(currPrevPermutation, currPermutation);
	currLeftMidFirstHalfWeight = _mm256_maddubs_epi16(currLeftMidFirstHalfhi, weights);
	currNextPermutation = _mm256_permute4x64_epi64(currNext, 216);
	currNextSecondHalf = _mm256_unpackhi_epi8(currNextPermutation, _mm256_setzero_si256());
	currNextSecondHalf = _mm256_slli_epi16(currNextSecondHalf, 1);
	currLeftMidFirstHalfhi = _mm256_add_epi16(currNextSecondHalf, currLeftMidFirstHalfWeight);

	// Top
	topPrevPermutation = _mm256_permute4x64_epi64(topPrev, 216);
	topPermutation = _mm256_permute4x64_epi64(top, 216);
	topLeftMidFirstHalflo = _mm256_unpacklo_epi8(topPrevPermutation, topPermutation);
	weights = _mm256_loadu_si256((__m256i*)WeakChromafilter[1]);
	topLeftMidFirstHalfWeight = _mm256_maddubs_epi16(topLeftMidFirstHalflo, weights);
	topNextPermutation = _mm256_permute4x64_epi64(topNext, 88);
	topNextFirstHalf = _mm256_unpacklo_epi8(topNextPermutation, _mm256_setzero_si256());
	topLeftMidFirstHalflo = _mm256_add_epi16(topNextFirstHalf, topLeftMidFirstHalfWeight);

	topLeftMidFirstHalfhi = _mm256_unpackhi_epi8(topPrevPermutation, topPermutation);
	topLeftMidFirstHalfWeight = _mm256_maddubs_epi16(topLeftMidFirstHalfhi, weights);
	topNextPermutation = _mm256_permute4x64_epi64(topNext, 216);
	topNextSecondHalf = _mm256_unpackhi_epi8(topNextPermutation, _mm256_setzero_si256());
	topLeftMidFirstHalfhi = _mm256_add_epi16(topNextSecondHalf, topLeftMidFirstHalfWeight);


	// Bottom
	bottomPrevPermutation = _mm256_permute4x64_epi64(bottomPrev, 216);
	bottomPermutation = _mm256_permute4x64_epi64(bottom, 216);
	bottomLeftMidFirstHalflo = _mm256_unpacklo_epi8(bottomPrevPermutation, bottomPermutation);
	weights = _mm256_loadu_si256((__m256i*)WeakChromafilter[1]);
	bottomLeftMidFirstHalfWeight = _mm256_maddubs_epi16(bottomLeftMidFirstHalflo, weights);
	bottomNextPermutation = _mm256_permute4x64_epi64(bottomNext, 88);
	bottomNextFirstHalf = _mm256_unpacklo_epi8(bottomNextPermutation, _mm256_setzero_si256());
	bottomLeftMidFirstHalflo = _mm256_add_epi16(bottomNextFirstHalf, bottomLeftMidFirstHalfWeight);

	bottomLeftMidFirstHalfhi = _mm256_unpackhi_epi8(bottomPrevPermutation, bottomPermutation);
	bottomLeftMidFirstHalfWeight = _mm256_maddubs_epi16(bottomLeftMidFirstHalfhi, weights);
	bottomNextPermutation = _mm256_permute4x64_epi64(bottomNext, 216);
	bottomNextSecondHalf = _mm256_unpackhi_epi8(bottomNextPermutation, _mm256_setzero_si256());
	bottomLeftMidFirstHalfhi = _mm256_add_epi16(bottomNextSecondHalf, bottomLeftMidFirstHalfWeight);


	filterFirstHalf = _mm256_adds_epi16(_mm256_adds_epi16(bottomLeftMidFirstHalflo, topLeftMidFirstHalflo), currLeftMidFirstHalflo);
	filterFirstHalf = _mm256_srli_epi16(filterFirstHalf, 4);
	filterSecondHalf = _mm256_adds_epi16(_mm256_adds_epi16(bottomLeftMidFirstHalfhi, topLeftMidFirstHalfhi), currLeftMidFirstHalfhi);
	filterSecondHalf = _mm256_srli_epi16(filterSecondHalf, 4);


	filterFirstHalf = _mm256_permute4x64_epi64(_mm256_packus_epi16(filterFirstHalf, filterSecondHalf), 216);
	_mm256_storeu_si256((__m256i *)(ptrDenoised), filterFirstHalf);


}

inline void ChromaStrong_AVX2_INTRIN(

	__m256i						top,
	__m256i						curr,
	__m256i						bottom,
	__m256i						currPrev,
	__m256i						currNext,
	__m256i						topPrev,
	__m256i						topNext,
	__m256i						bottomPrev,
	__m256i						bottomNext,
	EB_U8					   *ptrDenoised
	)
{
	__m256i   currLeftMidFirstHalflo, currLeftMidFirstHalfhi, currPrevPermutation, currPermutation, currNextPermutation,
		topPermutation, topPrevPermutation, topLeftMidFirstHalflo, topNextPermutation, topLeftMidFirstHalfhi,
		bottomPermutation, bottomPrevPermutation, bottomLeftMidFirstHalflo, bottomNextPermutation, bottomLeftMidFirstHalfhi;


	currPrevPermutation = _mm256_permute4x64_epi64(currPrev, 216);
	currPermutation = _mm256_permute4x64_epi64(curr, 216);
	currNextPermutation = _mm256_permute4x64_epi64(currNext, 216);

	currLeftMidFirstHalflo = _mm256_add_epi16(_mm256_unpacklo_epi8(currPermutation, _mm256_setzero_si256()),
		_mm256_unpacklo_epi8(currPrevPermutation, _mm256_setzero_si256()));
	currLeftMidFirstHalflo = _mm256_add_epi16(_mm256_unpacklo_epi8(currNextPermutation, _mm256_setzero_si256()), currLeftMidFirstHalflo);

	currLeftMidFirstHalfhi = _mm256_add_epi16(_mm256_unpackhi_epi8(currPermutation, _mm256_setzero_si256()),
		_mm256_unpackhi_epi8(currPrevPermutation, _mm256_setzero_si256()));
	currLeftMidFirstHalfhi = _mm256_add_epi16(_mm256_unpackhi_epi8(currNextPermutation, _mm256_setzero_si256()), currLeftMidFirstHalfhi);


	topPrevPermutation = _mm256_permute4x64_epi64(topPrev, 216);
	topPermutation = _mm256_permute4x64_epi64(top, 216);
	topNextPermutation = _mm256_permute4x64_epi64(topNext, 216);


	topLeftMidFirstHalflo = _mm256_add_epi16(_mm256_unpacklo_epi8(topPermutation, _mm256_setzero_si256()),
		_mm256_unpacklo_epi8(topPrevPermutation, _mm256_setzero_si256()));
	topLeftMidFirstHalflo = _mm256_add_epi16(_mm256_unpacklo_epi8(topNextPermutation, _mm256_setzero_si256()), topLeftMidFirstHalflo);


	topLeftMidFirstHalfhi = _mm256_add_epi16(_mm256_unpackhi_epi8(topPermutation, _mm256_setzero_si256()),
		_mm256_unpackhi_epi8(topPrevPermutation, _mm256_setzero_si256()));
	topLeftMidFirstHalfhi = _mm256_add_epi16(_mm256_unpackhi_epi8(topNextPermutation, _mm256_setzero_si256()), topLeftMidFirstHalfhi);



	bottomPrevPermutation = _mm256_permute4x64_epi64(bottomPrev, 216);
	bottomPermutation = _mm256_permute4x64_epi64(bottom, 216);
	bottomNextPermutation = _mm256_permute4x64_epi64(bottomNext, 216);

	bottomLeftMidFirstHalflo = _mm256_add_epi16(_mm256_unpacklo_epi8(bottomPermutation, _mm256_setzero_si256()),
		_mm256_unpacklo_epi8(bottomPrevPermutation, _mm256_setzero_si256()));
	bottomLeftMidFirstHalflo = _mm256_add_epi16(_mm256_unpacklo_epi8(bottomNextPermutation, _mm256_setzero_si256()), bottomLeftMidFirstHalflo);


	bottomLeftMidFirstHalfhi = _mm256_add_epi16(_mm256_unpackhi_epi8(bottomPermutation, _mm256_setzero_si256()),
		_mm256_unpackhi_epi8(bottomPrevPermutation, _mm256_setzero_si256()));
	bottomLeftMidFirstHalfhi = _mm256_add_epi16(_mm256_unpackhi_epi8(bottomNextPermutation, _mm256_setzero_si256()), bottomLeftMidFirstHalfhi);


	currLeftMidFirstHalflo = _mm256_add_epi16(_mm256_add_epi16(currLeftMidFirstHalflo, topLeftMidFirstHalflo), bottomLeftMidFirstHalflo);
	currLeftMidFirstHalfhi = _mm256_add_epi16(_mm256_add_epi16(currLeftMidFirstHalfhi, topLeftMidFirstHalfhi), bottomLeftMidFirstHalfhi);

	topLeftMidFirstHalflo = _mm256_unpacklo_epi16(currLeftMidFirstHalflo, _mm256_setzero_si256());
	topLeftMidFirstHalflo = _mm256_mullo_epi32(topLeftMidFirstHalflo, _mm256_set1_epi32(7282));
	topLeftMidFirstHalflo = _mm256_srli_epi32(topLeftMidFirstHalflo, 16);
	bottomLeftMidFirstHalflo = _mm256_unpackhi_epi16(currLeftMidFirstHalflo, _mm256_setzero_si256());
	bottomLeftMidFirstHalflo = _mm256_mullo_epi32(bottomLeftMidFirstHalflo, _mm256_set1_epi32(7282));
	bottomLeftMidFirstHalflo = _mm256_srli_epi32(bottomLeftMidFirstHalflo, 16);
	currLeftMidFirstHalflo = _mm256_packus_epi32(topLeftMidFirstHalflo, bottomLeftMidFirstHalflo);

	currLeftMidFirstHalflo = _mm256_insertf128_si256(_mm256_setzero_si256(), _mm_packus_epi16(_mm256_extracti128_si256(currLeftMidFirstHalflo, 0), _mm256_extracti128_si256(currLeftMidFirstHalflo, 1)), 0);


	topLeftMidFirstHalfhi = _mm256_unpacklo_epi16(currLeftMidFirstHalfhi, _mm256_setzero_si256());
	topLeftMidFirstHalfhi = _mm256_mullo_epi32(topLeftMidFirstHalfhi, _mm256_set1_epi32(7282));
	topLeftMidFirstHalfhi = _mm256_srli_epi32(topLeftMidFirstHalfhi, 16);

	bottomLeftMidFirstHalfhi = _mm256_unpackhi_epi16(currLeftMidFirstHalfhi, _mm256_setzero_si256());
	bottomLeftMidFirstHalfhi = _mm256_mullo_epi32(bottomLeftMidFirstHalfhi, _mm256_set1_epi32(7282));
	bottomLeftMidFirstHalfhi = _mm256_srli_epi32(bottomLeftMidFirstHalfhi, 16);
	currLeftMidFirstHalfhi = _mm256_packus_epi32(topLeftMidFirstHalfhi, bottomLeftMidFirstHalfhi);

	currLeftMidFirstHalflo = _mm256_insertf128_si256(currLeftMidFirstHalflo, _mm_packus_epi16(_mm256_extracti128_si256(currLeftMidFirstHalfhi, 0), _mm256_extracti128_si256(currLeftMidFirstHalfhi, 1)), 1);
	_mm256_storeu_si256((__m256i *)(ptrDenoised), currLeftMidFirstHalflo);


}
/*******************************************
* noiseExtractLumaWeak
*  weak filter Luma and store noise.
*******************************************/
void noiseExtractLumaWeak_AVX2_INTRIN(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EbPictureBufferDesc_t       *noisePicturePtr,
	EB_U32                       lcuOriginY,
	EB_U32						 lcuOriginX
	)
{
	EB_U32  ii, jj, kk;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;
	EB_U32  noiseOriginIndex;

	EB_U8 *ptrIn;
	EB_U32 strideIn;
	EB_U8 *ptrDenoised, *ptrDenoisedInterm;

	EB_U8 *ptrNoise, *ptrNoiseInterm;
	EB_U32 strideOut;

	__m256i top, curr, bottom, currPrev, currNext,
		secondtop, secondcurr, secondbottom, secondcurrPrev, secondcurrNext;
	(void)lcuOriginX;

	//Luma
	{
		picHeight = inputPicturePtr->height;
		picWidth = inputPicturePtr->width;
		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);
		lcuHeight = ((lcuOriginY + MAX_LCU_SIZE >= picHeight) || (lcuOriginY == 0)) ? lcuHeight - 1 : lcuHeight;
		strideIn = inputPicturePtr->strideY;
		inputOriginIndex = inputPicturePtr->originX + (inputPicturePtr->originY + lcuOriginY) * inputPicturePtr->strideY;
		ptrIn = &(inputPicturePtr->bufferY[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX + (denoisedPicturePtr->originY + lcuOriginY) * denoisedPicturePtr->strideY;
		strideOut = denoisedPicturePtr->strideY;
		ptrDenoised = &(denoisedPicturePtr->bufferY[inputOriginIndexPad]);
		ptrDenoisedInterm = ptrDenoised;

		noiseOriginIndex = noisePicturePtr->originX + noisePicturePtr->originY * noisePicturePtr->strideY;
		ptrNoise = &(noisePicturePtr->bufferY[noiseOriginIndex]);
		ptrNoiseInterm = ptrNoise;

		////Luma
		//a = (p[1] +
		//	p[0 + stride] + 4 * p[1 + stride] + p[2 + stride] +
		//	p[1 + 2 * stride]) / 8;

		top = curr =  secondtop = secondcurr = _mm256_setzero_si256();
	
		for (kk = 0; kk + MAX_LCU_SIZE <= picWidth; kk += MAX_LCU_SIZE)
		{
			for (jj = 0; jj < lcuHeight; jj++)
			{
				if (lcuOriginY == 0)
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + kk + jj*strideIn));
						secondtop = _mm256_loadu_si256((__m256i*)(ptrIn + kk + 32 + jj*strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + kk + (1 + jj)*strideIn));
						secondcurr = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + (1 + jj)*strideIn));
						_mm256_storeu_si256((__m256i *)(ptrDenoised + kk), top);
						_mm256_storeu_si256((__m256i *)(ptrDenoised + kk + 32), secondtop);
						_mm256_storeu_si256((__m256i *)(ptrNoise + kk), _mm256_setzero_si256());
						_mm256_storeu_si256((__m256i *)(ptrNoise + kk + 32), _mm256_setzero_si256());
					}
					currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((1 + jj)*strideIn)));
					currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((1 + jj)*strideIn)));
					secondcurrPrev = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) - 1 + ((1 + jj)*strideIn)));
					secondcurrNext = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + 1 + ((1 + jj)*strideIn)));
					bottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk) + (2 + jj)* strideIn));
					secondbottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + (2 + jj)* strideIn));
					ptrDenoisedInterm = ptrDenoised + kk + ((1 + jj)*strideOut);
					ptrNoiseInterm = ptrNoise + kk + ((1 + jj)*strideOut);

				}
				else
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + kk + jj*strideIn - strideIn));
						secondtop = _mm256_loadu_si256((__m256i*)(ptrIn + kk + 32 + jj *strideIn - strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + kk + (1 + jj)*strideIn - strideIn));
						secondcurr = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + (1 + jj)*strideIn - strideIn));
					}
					currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((1 + jj)*strideIn - strideIn)));
					currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((1 + jj)*strideIn - strideIn)));
					secondcurrPrev = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) - 1 + ((1 + jj)*strideIn - strideIn)));
					secondcurrNext = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + 1 + ((1 + jj)*strideIn - strideIn)));
					bottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk) + (2 + jj)* strideIn - strideIn));
					secondbottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + (2 + jj)* strideIn - strideIn));
					ptrDenoisedInterm = ptrDenoised + kk + ((1 + jj)*strideOut - strideOut);
					ptrNoiseInterm = ptrNoise + kk + jj*strideOut;

				}

				lumaWeakFilter_AVX2_INTRIN(
					top,
					curr,
					bottom,
					currPrev,
					currNext,
					ptrDenoisedInterm,
					ptrNoiseInterm);

				lumaWeakFilter_AVX2_INTRIN(
					secondtop,
					secondcurr,
					secondbottom,
					secondcurrPrev,
					secondcurrNext,
					ptrDenoisedInterm + 32,
					ptrNoiseInterm + 32);

				top = curr;
				curr = bottom;
				secondtop = secondcurr;
				secondcurr = secondbottom;

			}


		}

		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);

		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = 0; ii < picWidth; ii++){

				if (!((jj<lcuHeight - 1 || lcuOriginY + lcuHeight<picHeight) && ii>0 && ii < picWidth - 1)){

					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
					ptrNoise[ii + jj*strideOut] = 0;
				}

			}
		}

	}

}

void noiseExtractLumaWeakLcu_AVX2_INTRIN(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EbPictureBufferDesc_t       *noisePicturePtr,
	EB_U32                       lcuOriginY,
	EB_U32						 lcuOriginX
	)
{
	EB_U32  ii, jj;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth, lcuWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;
	EB_U32  noiseOriginIndex;

	EB_U8 *ptrIn;
	EB_U32 strideIn;
	EB_U8 *ptrDenoised, *ptrDenoisedInterm;

	EB_U8 *ptrNoise, *ptrNoiseInterm;
	EB_U32 strideOut;

	__m256i top, curr, bottom, currPrev, currNext,
		secondtop, secondcurr, secondbottom, secondcurrPrev, secondcurrNext;
	(void)lcuOriginX;

	//Luma
	{
		picHeight = inputPicturePtr->height;
		picWidth = inputPicturePtr->width;
		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);
		lcuWidth = MIN(MAX_LCU_SIZE, picWidth - lcuOriginX);
		lcuHeight = ((lcuOriginY + MAX_LCU_SIZE >= picHeight) || (lcuOriginY == 0)) ? lcuHeight - 1 : lcuHeight;
		strideIn = inputPicturePtr->strideY;
		inputOriginIndex = inputPicturePtr->originX + lcuOriginX + (inputPicturePtr->originY + lcuOriginY) * inputPicturePtr->strideY;
		ptrIn = &(inputPicturePtr->bufferY[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX + lcuOriginX + (denoisedPicturePtr->originY + lcuOriginY) * denoisedPicturePtr->strideY;
		strideOut = denoisedPicturePtr->strideY;
		ptrDenoised = &(denoisedPicturePtr->bufferY[inputOriginIndexPad]);
		ptrDenoisedInterm = ptrDenoised;

		noiseOriginIndex = noisePicturePtr->originX + lcuOriginX + noisePicturePtr->originY * noisePicturePtr->strideY;
		ptrNoise = &(noisePicturePtr->bufferY[noiseOriginIndex]);
		ptrNoiseInterm = ptrNoise;

		////Luma
		//a = (p[1] +
		//	p[0 + stride] + 4 * p[1 + stride] + p[2 + stride] +
		//	p[1 + 2 * stride]) / 8;

		top = curr = secondtop = secondcurr = _mm256_setzero_si256();

		//for (kk = 0; kk + MAX_LCU_SIZE <= picWidth; kk += MAX_LCU_SIZE)
		{
			for (jj = 0; jj < lcuHeight; jj++)
			{
				if (lcuOriginY == 0)
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + jj*strideIn));
						secondtop = _mm256_loadu_si256((__m256i*)(ptrIn + 32 + jj*strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + (1 + jj)*strideIn));
						secondcurr = _mm256_loadu_si256((__m256i*)((ptrIn + 32) + (1 + jj)*strideIn));
						_mm256_storeu_si256((__m256i *)(ptrDenoised ), top);
						_mm256_storeu_si256((__m256i *)(ptrDenoised + 32), secondtop);
						_mm256_storeu_si256((__m256i *)(ptrNoise ), _mm256_setzero_si256());
						_mm256_storeu_si256((__m256i *)(ptrNoise + 32), _mm256_setzero_si256());
					}
					currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + ((1 + jj)*strideIn)));
					currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + ((1 + jj)*strideIn)));
					secondcurrPrev = _mm256_loadu_si256((__m256i*)((ptrIn + 32) - 1 + ((1 + jj)*strideIn)));
					secondcurrNext = _mm256_loadu_si256((__m256i*)((ptrIn + 32) + 1 + ((1 + jj)*strideIn)));
					bottom = _mm256_loadu_si256((__m256i*)((ptrIn ) + (2 + jj)* strideIn));
					secondbottom = _mm256_loadu_si256((__m256i*)((ptrIn + 32) + (2 + jj)* strideIn));
					ptrDenoisedInterm = ptrDenoised + ((1 + jj)*strideOut);
					ptrNoiseInterm = ptrNoise + ((1 + jj)*strideOut);

				}
				else
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + jj*strideIn - strideIn));
						secondtop = _mm256_loadu_si256((__m256i*)(ptrIn + 32 + jj *strideIn - strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + (1 + jj)*strideIn - strideIn));
						secondcurr = _mm256_loadu_si256((__m256i*)((ptrIn + 32) + (1 + jj)*strideIn - strideIn));
					}
					currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + ((1 + jj)*strideIn - strideIn)));
					currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + ((1 + jj)*strideIn - strideIn)));
					secondcurrPrev = _mm256_loadu_si256((__m256i*)((ptrIn  + 32) - 1 + ((1 + jj)*strideIn - strideIn)));
					secondcurrNext = _mm256_loadu_si256((__m256i*)((ptrIn  + 32) + 1 + ((1 + jj)*strideIn - strideIn)));
					bottom = _mm256_loadu_si256((__m256i*)((ptrIn ) + (2 + jj)* strideIn - strideIn));
					secondbottom = _mm256_loadu_si256((__m256i*)((ptrIn + 32) + (2 + jj)* strideIn - strideIn));
					ptrDenoisedInterm = ptrDenoised  + ((1 + jj)*strideOut - strideOut);
					ptrNoiseInterm = ptrNoise + jj*strideOut;

				}

				lumaWeakFilter_AVX2_INTRIN(
					top,
					curr,
					bottom,
					currPrev,
					currNext,
					ptrDenoisedInterm,
					ptrNoiseInterm);

				lumaWeakFilter_AVX2_INTRIN(
					secondtop,
					secondcurr,
					secondbottom,
					secondcurrPrev,
					secondcurrNext,
					ptrDenoisedInterm + 32,
					ptrNoiseInterm + 32);

				top = curr;
				curr = bottom;
				secondtop = secondcurr;
				secondcurr = secondbottom;

			}


		}

		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);

		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = 0; ii < lcuWidth; ii++){

				if (!((jj>0 || lcuOriginY>0) && (jj<lcuHeight - 1 || lcuOriginY + lcuHeight<picHeight) && (ii>0 || lcuOriginX>0) && (ii + lcuOriginX) < picWidth - 1)){

					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
					ptrNoise[ii + jj*strideOut] = 0;
				}

			}
		}

	}

}
/*******************************************
* noiseExtractLumaStrong
*  strong filter Luma.
*******************************************/
void noiseExtractLumaStrong_AVX2_INTRIN(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EB_U32                       lcuOriginY,
	EB_U32                       lcuOriginX
	)
{
	EB_U32  ii, jj, kk;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;

	EB_U8 *ptrIn;
	EB_U32 strideIn;
	EB_U8 *ptrDenoised, *ptrDenoisedInterm;

	EB_U32 strideOut;
	__m256i top, curr, bottom, currPrev, currNext, topPrev, topNext, bottomPrev, bottomNext,
		secondtop, secondcurr, secondcurrPrev, secondcurrNext, secondbottom, secondtopPrev, secondtopNext, secondbottomPrev, secondbottomNext;
	(void)lcuOriginX;
	//Luma
	{
		picHeight = inputPicturePtr->height;
		picWidth = inputPicturePtr->width;
		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);

		lcuHeight = ((lcuOriginY + MAX_LCU_SIZE >= picHeight) || (lcuOriginY == 0)) ? lcuHeight - 1 : lcuHeight;
		strideIn = inputPicturePtr->strideY;
		inputOriginIndex = inputPicturePtr->originX + (inputPicturePtr->originY + lcuOriginY)* inputPicturePtr->strideY;
		ptrIn = &(inputPicturePtr->bufferY[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX + (denoisedPicturePtr->originY + lcuOriginY) * denoisedPicturePtr->strideY;
		strideOut = denoisedPicturePtr->strideY;
		ptrDenoised = &(denoisedPicturePtr->bufferY[inputOriginIndexPad]);
		ptrDenoisedInterm = ptrDenoised;


		top = curr = secondtop = secondcurr = topNext = topPrev = currNext = currPrev = secondcurrPrev = secondcurrNext = secondtopPrev = secondtopNext = _mm256_setzero_si256();
		for (kk = 0; kk + MAX_LCU_SIZE <= picWidth; kk += MAX_LCU_SIZE)
		{
			for (jj = 0; jj < lcuHeight; jj++)
			{
				if (lcuOriginY == 0)
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + kk + jj*strideIn));
						secondtop = _mm256_loadu_si256((__m256i*)(ptrIn + kk + 32 + jj*strideIn));

						curr = _mm256_loadu_si256((__m256i*)(ptrIn + kk + (1 + jj)*strideIn));
						secondcurr = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + (1 + jj)*strideIn));

						topPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((jj)*strideIn)));
						secondtopPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + 32 + ((jj)*strideIn)));

						topNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((jj)*strideIn)));
						secondtopNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + 32 + ((jj)*strideIn)));

						currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((1 + jj)*strideIn)));
						secondcurrPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + 32 + ((1 + jj)*strideIn)));

						currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((1 + jj)*strideIn)));
						secondcurrNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + 32 + ((1 + jj)*strideIn)));

						_mm256_storeu_si256((__m256i *)(ptrDenoised + kk), top);
						_mm256_storeu_si256((__m256i *)(ptrDenoised + kk + 32), secondtop);
					}
					bottomPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((2 + jj)*strideIn)));
					secondbottomPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + 32 + ((2 + jj)*strideIn)));

					bottomNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((2 + jj)*strideIn)));
					secondbottomNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + 32 + ((2 + jj)*strideIn)));

					bottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk) + (2 + jj)* strideIn));
					secondbottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + (2 + jj)* strideIn));
					ptrDenoisedInterm = ptrDenoised + kk + ((1 + jj)*strideOut);
				}
				else
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + kk + jj*strideIn - strideIn));
						secondtop = _mm256_loadu_si256((__m256i*)(ptrIn + kk + 32 + jj*strideIn - strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + kk + (1 + jj)*strideIn - strideIn));
						secondcurr = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + (1 + jj)*strideIn - strideIn));
						topPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((jj)*strideIn) - strideIn));
						secondtopPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + 32 + ((jj)*strideIn) - strideIn));

						topNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((jj)*strideIn) - strideIn));
						secondtopNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + 32 + ((jj)*strideIn) - strideIn));

						currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((1 + jj)*strideIn - strideIn)));
						secondcurrPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + 32 + ((1 + jj)*strideIn - strideIn)));

						currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((1 + jj)*strideIn - strideIn)));
						secondcurrNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + 32 + ((1 + jj)*strideIn - strideIn)));
					}
					bottomPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((2 + jj)*strideIn) - strideIn));
					secondbottomPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + 32 + ((2 + jj)*strideIn - strideIn)));

					bottomNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((2 + jj)*strideIn) - strideIn));
					secondbottomNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + 32 + ((2 + jj)*strideIn - strideIn)));

					bottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk) + (2 + jj)* strideIn - strideIn));
					secondbottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk + 32) + (2 + jj)* strideIn - strideIn));

					ptrDenoisedInterm = ptrDenoised + kk + ((1 + jj)*strideOut - strideOut);

				}

				chromaWeakLumaStrongFilter_AVX2_INTRIN(
					top,
					curr,
					bottom,
					currPrev,
					currNext,
					topPrev,
					topNext,
					bottomPrev,
					bottomNext,
					ptrDenoisedInterm);


				chromaWeakLumaStrongFilter_AVX2_INTRIN(
					secondtop,
					secondcurr,
					secondbottom,
					secondcurrPrev,
					secondcurrNext,
					secondtopPrev,
					secondtopNext,
					secondbottomPrev,
					secondbottomNext,
					ptrDenoisedInterm + 32);


				top = curr;
				curr = bottom;
				topPrev = currPrev;
				topNext = currNext;
				currPrev = bottomPrev;
				currNext = bottomNext;
				secondtop = secondcurr;
				secondcurr = secondbottom;
				secondtopPrev = secondcurrPrev;
				secondtopNext = secondcurrNext;
				secondcurrPrev = secondbottomPrev;
				secondcurrNext = secondbottomNext;

			}


		}

		lcuHeight = MIN(MAX_LCU_SIZE, picHeight - lcuOriginY);

		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = 0; ii < picWidth; ii++){

				if (!((jj<lcuHeight - 1 || lcuOriginY + lcuHeight<picHeight) && ii>0 && ii < picWidth - 1)){
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
				}

			}
		}

	}

}

/*******************************************
* noiseExtractChromaStrong
*  strong filter chroma.
*******************************************/
void noiseExtractChromaStrong_AVX2_INTRIN(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EB_U32                       lcuOriginY,
	EB_U32                       lcuOriginX
	)
{
	EB_U32  ii, jj, kk;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;

	EB_U8 *ptrIn, *ptrInCr;
	EB_U32 strideIn, strideInCr;
	EB_U8 *ptrDenoised, *ptrDenoisedInterm, *ptrDenoisedCr, *ptrDenoisedIntermCr;

	EB_U32 strideOut, strideOutCr;
	__m256i top, curr, bottom, currPrev, currNext, topPrev, topNext, bottomPrev, bottomNext,
		topCr, currCr, bottomCr, currPrevCr, currNextCr, topPrevCr, topNextCr, bottomPrevCr, bottomNextCr;
	(void)lcuOriginX;
	{
		picHeight = inputPicturePtr->height / 2;
		picWidth = inputPicturePtr->width / 2;
		lcuHeight = MIN(MAX_LCU_SIZE / 2, picHeight - lcuOriginY);

		lcuHeight = ((lcuOriginY + MAX_LCU_SIZE / 2 >= picHeight) || (lcuOriginY == 0)) ? lcuHeight - 1 : lcuHeight;

		strideIn = inputPicturePtr->strideCb;
		inputOriginIndex = inputPicturePtr->originX / 2 + (inputPicturePtr->originY / 2 + lcuOriginY)  * inputPicturePtr->strideCb;
		ptrIn = &(inputPicturePtr->bufferCb[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX / 2 + (denoisedPicturePtr->originY / 2 + lcuOriginY)  * denoisedPicturePtr->strideCb;
		strideOut = denoisedPicturePtr->strideCb;
		ptrDenoised = &(denoisedPicturePtr->bufferCb[inputOriginIndexPad]);
		ptrDenoisedInterm = ptrDenoised;

		strideInCr = inputPicturePtr->strideCr;
		inputOriginIndex = inputPicturePtr->originX / 2 + (inputPicturePtr->originY / 2 + lcuOriginY)  * inputPicturePtr->strideCr;
		ptrInCr = &(inputPicturePtr->bufferCr[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX / 2 + (denoisedPicturePtr->originY / 2 + lcuOriginY)  * denoisedPicturePtr->strideCr;
		strideOutCr = denoisedPicturePtr->strideCr;
		ptrDenoisedCr = &(denoisedPicturePtr->bufferCr[inputOriginIndexPad]);
		ptrDenoisedIntermCr = ptrDenoisedCr;
		////Chroma
		//a = (4 * p[0] + 4 * p[1] + 4 * p[2] +
		//	4 * p[0 + stride] + 4 * p[1 + stride] + 4 * p[2 + stride] +
		//	4 * p[0 + 2 * stride] + 4 * p[1 + 2 * stride] + 4 * p[2 + 2 * stride]) / 36;

		top = curr = topNext = topPrev = currNext = currPrev = topCr = currCr = topNextCr = topPrevCr = currNextCr = currPrevCr =  _mm256_setzero_si256();

		for (kk = 0; kk + MAX_LCU_SIZE / 2 <= picWidth; kk += MAX_LCU_SIZE / 2)
		{

			for (jj = 0; jj < lcuHeight; jj++)
			{
				if (lcuOriginY == 0)
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + kk + jj*strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + kk + (1 + jj)*strideIn));
						topPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((jj)*strideIn)));
						topNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((jj)*strideIn)));
						currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((1 + jj)*strideIn)));
						currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((1 + jj)*strideIn)));
						topCr = _mm256_loadu_si256((__m256i*)(ptrInCr + kk + jj*strideInCr));
						currCr = _mm256_loadu_si256((__m256i*)(ptrInCr + kk + (1 + jj)*strideInCr));
						topPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((jj)*strideInCr)));
						topNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((jj)*strideInCr)));
						currPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((1 + jj)*strideInCr)));
						currNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((1 + jj)*strideInCr)));
						_mm256_storeu_si256((__m256i *)(ptrDenoised + kk), top);
						_mm256_storeu_si256((__m256i *)(ptrDenoisedCr + kk), topCr);
					}
					bottomPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((2 + jj)*strideIn)));
					bottomNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((2 + jj)*strideIn)));
					bottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk) + (2 + jj)* strideIn));
					bottomPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((2 + jj)*strideInCr)));
					bottomNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((2 + jj)*strideInCr)));
					bottomCr = _mm256_loadu_si256((__m256i*)((ptrInCr + kk) + (2 + jj)* strideInCr));
					ptrDenoisedInterm = ptrDenoised + kk + ((1 + jj)*strideOut);
					ptrDenoisedIntermCr = ptrDenoisedCr + kk + ((1 + jj)*strideOutCr);
				}
				else
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + kk + jj*strideIn - strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + kk + (1 + jj)*strideIn - strideIn));
						topPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((jj)*strideIn) - strideIn));
						topNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((jj)*strideIn) - strideIn));
						currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((1 + jj)*strideIn - strideIn)));
						currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((1 + jj)*strideIn - strideIn)));
						topCr = _mm256_loadu_si256((__m256i*)(ptrInCr + kk + jj*strideInCr - strideInCr));
						currCr = _mm256_loadu_si256((__m256i*)(ptrInCr + kk + (1 + jj)*strideInCr - strideInCr));
						topPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((jj)*strideInCr) - strideInCr));
						topNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((jj)*strideInCr) - strideInCr));
						currPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((1 + jj)*strideInCr - strideInCr)));
						currNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((1 + jj)*strideInCr - strideInCr)));
					}
					bottomPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((2 + jj)*strideIn) - strideIn));
					bottomNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((2 + jj)*strideIn) - strideIn));
					bottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk) + (2 + jj)* strideIn - strideIn));
					ptrDenoisedInterm = ptrDenoised + kk + ((1 + jj)*strideOut - strideOut);
					bottomPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((2 + jj)*strideInCr) - strideInCr));
					bottomNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((2 + jj)*strideInCr) - strideInCr));
					bottomCr = _mm256_loadu_si256((__m256i*)((ptrInCr + kk) + (2 + jj)* strideInCr - strideInCr));
					ptrDenoisedIntermCr = ptrDenoisedCr + kk + ((1 + jj)*strideOutCr - strideOutCr);


				}

				ChromaStrong_AVX2_INTRIN(
					top,
					curr,
					bottom,
					currPrev,
					currNext,
					topPrev,
					topNext,
					bottomPrev,
					bottomNext,
					ptrDenoisedInterm);

				ChromaStrong_AVX2_INTRIN(
					topCr,
					currCr,
					bottomCr,
					currPrevCr,
					currNextCr,
					topPrevCr,
					topNextCr,
					bottomPrevCr,
					bottomNextCr,
					ptrDenoisedIntermCr);

				top = curr;
				curr = bottom;
				topPrev = currPrev;
				topNext = currNext;
				currPrev = bottomPrev;
				currNext = bottomNext;
				topCr = currCr;
				currCr = bottomCr;
				topPrevCr = currPrevCr;
				topNextCr = currNextCr;
				currPrevCr = bottomPrevCr;
				currNextCr = bottomNextCr;

			}


		}

		lcuHeight = MIN(MAX_LCU_SIZE / 2, picHeight - lcuOriginY);

		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = 0; ii < picWidth; ii++){

				if (!((jj<lcuHeight - 1 || (lcuOriginY + lcuHeight)<picHeight) && ii>0 && ii < picWidth - 1)){
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
					ptrDenoisedCr[ii + jj*strideOut] = ptrInCr[ii + jj*strideIn];
				}

			}
		}
	}


}

/*******************************************
* noiseExtractChromaWeak
*  weak filter chroma.
*******************************************/
void noiseExtractChromaWeak_AVX2_INTRIN(
	EbPictureBufferDesc_t       *inputPicturePtr,
	EbPictureBufferDesc_t       *denoisedPicturePtr,
	EB_U32                       lcuOriginY,
	EB_U32                       lcuOriginX
	)
{
	EB_U32  ii, jj, kk;
	EB_U32  picHeight, lcuHeight;
	EB_U32  picWidth;
	EB_U32  inputOriginIndex;
	EB_U32  inputOriginIndexPad;

	EB_U8 *ptrIn, *ptrInCr;
	EB_U32 strideIn, strideInCr;
	EB_U8 *ptrDenoised, *ptrDenoisedInterm, *ptrDenoisedCr, *ptrDenoisedIntermCr;

	EB_U32 strideOut, strideOutCr;

	__m256i top, curr, bottom, currPrev, currNext, topPrev, topNext, bottomPrev, bottomNext,
		topCr, currCr, bottomCr, currPrevCr, currNextCr, topPrevCr, topNextCr, bottomPrevCr, bottomNextCr;
	(void)lcuOriginX;
	////gaussian matrix(Chroma)
	//a = (1 * p[0] + 2 * p[1] + 1 * p[2] +
	//	2 * p[0 + stride] + 4 * p[1 + stride] + 2 * p[2 + stride] +
	//	1 * p[0 + 2 * stride] + 2 * p[1 + 2 * stride] + 1 * p[2 + 2 * stride]) / 16;

	{
		picHeight = inputPicturePtr->height / 2;
		picWidth = inputPicturePtr->width / 2;

		lcuHeight = MIN(MAX_LCU_SIZE / 2, picHeight - lcuOriginY);

		lcuHeight = ((lcuOriginY + MAX_LCU_SIZE / 2 >= picHeight) || (lcuOriginY == 0)) ? lcuHeight - 1 : lcuHeight;
		strideIn = inputPicturePtr->strideCb;
		inputOriginIndex = inputPicturePtr->originX / 2 + (inputPicturePtr->originY / 2 + lcuOriginY)* inputPicturePtr->strideCb;
		ptrIn = &(inputPicturePtr->bufferCb[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX / 2 + (denoisedPicturePtr->originY / 2 + lcuOriginY)* denoisedPicturePtr->strideCb;
		strideOut = denoisedPicturePtr->strideCb;
		ptrDenoised = &(denoisedPicturePtr->bufferCb[inputOriginIndexPad]);
		ptrDenoisedInterm = ptrDenoised;


		strideInCr = inputPicturePtr->strideCr;
		inputOriginIndex = inputPicturePtr->originX / 2 + (inputPicturePtr->originY / 2 + lcuOriginY)  * inputPicturePtr->strideCr;
		ptrInCr = &(inputPicturePtr->bufferCr[inputOriginIndex]);

		inputOriginIndexPad = denoisedPicturePtr->originX / 2 + (denoisedPicturePtr->originY / 2 + lcuOriginY)  * denoisedPicturePtr->strideCr;
		strideOutCr = denoisedPicturePtr->strideCr;
		ptrDenoisedCr = &(denoisedPicturePtr->bufferCr[inputOriginIndexPad]);
		ptrDenoisedIntermCr = ptrDenoisedCr;

		top = curr = topNext = topPrev = currNext = currPrev = topCr = currCr = topNextCr = topPrevCr = currNextCr = currPrevCr = _mm256_setzero_si256();
		for (kk = 0; kk + MAX_LCU_SIZE / 2 <= picWidth; kk += MAX_LCU_SIZE / 2)
		{

			for (jj = 0; jj < lcuHeight; jj++)
			{
				if (lcuOriginY == 0)
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + kk + jj*strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + kk + (1 + jj)*strideIn));
						topPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((jj)*strideIn)));
						topNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((jj)*strideIn)));
						currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((1 + jj)*strideIn)));
						currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((1 + jj)*strideIn)));
						_mm256_storeu_si256((__m256i *)(ptrDenoised + kk), top);
						topCr = _mm256_loadu_si256((__m256i*)(ptrInCr + kk + jj*strideInCr));
						currCr = _mm256_loadu_si256((__m256i*)(ptrInCr + kk + (1 + jj)*strideInCr));
						topPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((jj)*strideInCr)));
						topNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((jj)*strideInCr)));
						currPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((1 + jj)*strideInCr)));
						currNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((1 + jj)*strideInCr)));
						_mm256_storeu_si256((__m256i *)(ptrDenoisedCr + kk), topCr);
					}
					bottomPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((2 + jj)*strideIn)));
					bottomNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((2 + jj)*strideIn)));
					bottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk) + (2 + jj)* strideIn));
					ptrDenoisedInterm = ptrDenoised + kk + ((1 + jj)*strideOut);
					bottomPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((2 + jj)*strideInCr)));
					bottomNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((2 + jj)*strideInCr)));
					bottomCr = _mm256_loadu_si256((__m256i*)((ptrInCr + kk) + (2 + jj)* strideInCr));
					ptrDenoisedIntermCr = ptrDenoisedCr + kk + ((1 + jj)*strideOutCr);
				}
				else
				{
					if (jj == 0)
					{
						top = _mm256_loadu_si256((__m256i*)(ptrIn + kk + jj*strideIn - strideIn));
						curr = _mm256_loadu_si256((__m256i*)(ptrIn + kk + (1 + jj)*strideIn - strideIn));
						topPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((jj)*strideIn) - strideIn));
						topNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((jj)*strideIn) - strideIn));
						currPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((1 + jj)*strideIn - strideIn)));
						currNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((1 + jj)*strideIn - strideIn)));
						topCr = _mm256_loadu_si256((__m256i*)(ptrInCr + kk + jj*strideInCr - strideInCr));
						currCr = _mm256_loadu_si256((__m256i*)(ptrInCr + kk + (1 + jj)*strideInCr - strideInCr));
						topPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((jj)*strideInCr) - strideInCr));
						topNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((jj)*strideInCr) - strideInCr));
						currPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((1 + jj)*strideInCr - strideInCr)));
						currNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((1 + jj)*strideInCr - strideInCr)));
					}
					bottomPrev = _mm256_loadu_si256((__m256i*)(ptrIn - 1 + kk + ((2 + jj)*strideIn) - strideIn));
					bottomNext = _mm256_loadu_si256((__m256i*)(ptrIn + 1 + kk + ((2 + jj)*strideIn) - strideIn));
					bottom = _mm256_loadu_si256((__m256i*)((ptrIn + kk) + (2 + jj)* strideIn - strideIn));
					ptrDenoisedInterm = ptrDenoised + kk + ((1 + jj)*strideOut - strideOut);
					bottomPrevCr = _mm256_loadu_si256((__m256i*)(ptrInCr - 1 + kk + ((2 + jj)*strideInCr) - strideInCr));
					bottomNextCr = _mm256_loadu_si256((__m256i*)(ptrInCr + 1 + kk + ((2 + jj)*strideInCr) - strideInCr));
					bottomCr = _mm256_loadu_si256((__m256i*)((ptrInCr + kk) + (2 + jj)* strideInCr - strideInCr));
					ptrDenoisedIntermCr = ptrDenoisedCr + kk + ((1 + jj)*strideOutCr - strideOutCr);

				}

				chromaWeakLumaStrongFilter_AVX2_INTRIN(
					top,
					curr,
					bottom,
					currPrev,
					currNext,
					topPrev,
					topNext,
					bottomPrev,
					bottomNext,
					ptrDenoisedInterm);

				chromaWeakLumaStrongFilter_AVX2_INTRIN(
					topCr,
					currCr,
					bottomCr,
					currPrevCr,
					currNextCr,
					topPrevCr,
					topNextCr,
					bottomPrevCr,
					bottomNextCr,
					ptrDenoisedIntermCr);


				top = curr;
				curr = bottom;
				topPrev = currPrev;
				topNext = currNext;
				currPrev = bottomPrev;
				currNext = bottomNext;
				topCr = currCr;
				currCr = bottomCr;
				topPrevCr = currPrevCr;
				topNextCr = currNextCr;
				currPrevCr = bottomPrevCr;
				currNextCr = bottomNextCr;

			}


		}


		lcuHeight = MIN(MAX_LCU_SIZE / 2, picHeight - lcuOriginY);
		for (jj = 0; jj < lcuHeight; jj++){
			for (ii = 0; ii < picWidth; ii++){

				if (!((jj<lcuHeight - 1 || (lcuOriginY + lcuHeight)<picHeight) && ii>0 && ii < picWidth - 1)){
					ptrDenoised[ii + jj*strideOut] = ptrIn[ii + jj*strideIn];
					ptrDenoisedCr[ii + jj*strideOut] = ptrInCr[ii + jj*strideInCr];
				}

			}
		}
	}


}
