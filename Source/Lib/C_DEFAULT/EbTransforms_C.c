/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbTransforms_C.h"
#include "EbUtility.h"

#define MAX_POS_16BIT_NUM      32767
#define MIN_NEG_16BIT_NUM      -32768

static const EB_S16 DctCoef4x4[] = {
    64, 64, 64, 64,
    83, 36, -36, -83,
    64, -64, -64, 64,
    36, -83, 83, -36
};

static const EB_S16 DctCoef8x8[] = {
    64, 64, 64, 64, 64, 64, 64, 64,
    89, 75, 50, 18, -18, -50, -75, -89,
    83, 36, -36, -83, -83, -36, 36, 83,
    75, -18, -89, -50, 50, 89, 18, -75,
    64, -64, -64, 64, 64, -64, -64, 64,
    50, -89, 18, 75, -75, -18, 89, -50,
    36, -83, 83, -36, -36, 83, -83, 36,
    18, -50, 75, -89, 89, -75, 50, -18
};

static const EB_S16 DctCoef16x16[] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    90, 87, 80, 70, 57, 43, 25, 9, -9, -25, -43, -57, -70, -80, -87, -90,
    89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89,
    87, 57, 9, -43, -80, -90, -70, -25, 25, 70, 90, 80, 43, -9, -57, -87,
    83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83,
    80, 9, -70, -87, -25, 57, 90, 43, -43, -90, -57, 25, 87, 70, -9, -80,
    75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75,
    70, -43, -87, 9, 90, 25, -80, -57, 57, 80, -25, -90, -9, 87, 43, -70,
    64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64,
    57, -80, -25, 90, -9, -87, 43, 70, -70, -43, 87, 9, -90, 25, 80, -57,
    50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50,
    43, -90, 57, 25, -87, 70, 9, -80, 80, -9, -70, 87, -25, -57, 90, -43,
    36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36,
    25, -70, 90, -80, 43, 9, -57, 87, -87, 57, -9, -43, 80, -90, 70, -25,
    18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18,
    9, -25, 43, -57, 70, -80, 87, -90, 90, -87, 80, -70, 57, -43, 25, -9
};

static const EB_S16 DctCoef32x32[] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13, 4, -4, -13, -22, -31, -38, -46, -54, -61, -67, -73, -78, -82, -85, -88, -90, -90,
    90, 87, 80, 70, 57, 43, 25, 9, -9, -25, -43, -57, -70, -80, -87, -90, -90, -87, -80, -70, -57, -43, -25, -9, 9, 25, 43, 57, 70, 80, 87, 90,
    90, 82, 67, 46, 22, -4, -31, -54, -73, -85, -90, -88, -78, -61, -38, -13, 13, 38, 61, 78, 88, 90, 85, 73, 54, 31, 4, -22, -46, -67, -82, -90,
    89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89, 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89,
    88, 67, 31, -13, -54, -82, -90, -78, -46, -4, 38, 73, 90, 85, 61, 22, -22, -61, -85, -90, -73, -38, 4, 46, 78, 90, 82, 54, 13, -31, -67, -88,
    87, 57, 9, -43, -80, -90, -70, -25, 25, 70, 90, 80, 43, -9, -57, -87, -87, -57, -9, 43, 80, 90, 70, 25, -25, -70, -90, -80, -43, 9, 57, 87,
    85, 46, -13, -67, -90, -73, -22, 38, 82, 88, 54, -4, -61, -90, -78, -31, 31, 78, 90, 61, 4, -54, -88, -82, -38, 22, 73, 90, 67, 13, -46, -85,
    83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83,
    82, 22, -54, -90, -61, 13, 78, 85, 31, -46, -90, -67, 4, 73, 88, 38, -38, -88, -73, -4, 67, 90, 46, -31, -85, -78, -13, 61, 90, 54, -22, -82,
    80, 9, -70, -87, -25, 57, 90, 43, -43, -90, -57, 25, 87, 70, -9, -80, -80, -9, 70, 87, 25, -57, -90, -43, 43, 90, 57, -25, -87, -70, 9, 80,
    78, -4, -82, -73, 13, 85, 67, -22, -88, -61, 31, 90, 54, -38, -90, -46, 46, 90, 38, -54, -90, -31, 61, 88, 22, -67, -85, -13, 73, 82, 4, -78,
    75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75, 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75,
    73, -31, -90, -22, 78, 67, -38, -90, -13, 82, 61, -46, -88, -4, 85, 54, -54, -85, 4, 88, 46, -61, -82, 13, 90, 38, -67, -78, 22, 90, 31, -73,
    70, -43, -87, 9, 90, 25, -80, -57, 57, 80, -25, -90, -9, 87, 43, -70, -70, 43, 87, -9, -90, -25, 80, 57, -57, -80, 25, 90, 9, -87, -43, 70,
    67, -54, -78, 38, 85, -22, -90, 4, 90, 13, -88, -31, 82, 46, -73, -61, 61, 73, -46, -82, 31, 88, -13, -90, -4, 90, 22, -85, -38, 78, 54, -67,
    64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64,
    61, -73, -46, 82, 31, -88, -13, 90, -4, -90, 22, 85, -38, -78, 54, 67, -67, -54, 78, 38, -85, -22, 90, 4, -90, 13, 88, -31, -82, 46, 73, -61,
    57, -80, -25, 90, -9, -87, 43, 70, -70, -43, 87, 9, -90, 25, 80, -57, -57, 80, 25, -90, 9, 87, -43, -70, 70, 43, -87, -9, 90, -25, -80, 57,
    54, -85, -4, 88, -46, -61, 82, 13, -90, 38, 67, -78, -22, 90, -31, -73, 73, 31, -90, 22, 78, -67, -38, 90, -13, -82, 61, 46, -88, 4, 85, -54,
    50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50, 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50,
    46, -90, 38, 54, -90, 31, 61, -88, 22, 67, -85, 13, 73, -82, 4, 78, -78, -4, 82, -73, -13, 85, -67, -22, 88, -61, -31, 90, -54, -38, 90, -46,
    43, -90, 57, 25, -87, 70, 9, -80, 80, -9, -70, 87, -25, -57, 90, -43, -43, 90, -57, -25, 87, -70, -9, 80, -80, 9, 70, -87, 25, 57, -90, 43,
    38, -88, 73, -4, -67, 90, -46, -31, 85, -78, 13, 61, -90, 54, 22, -82, 82, -22, -54, 90, -61, -13, 78, -85, 31, 46, -90, 67, 4, -73, 88, -38,
    36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36,
    31, -78, 90, -61, 4, 54, -88, 82, -38, -22, 73, -90, 67, -13, -46, 85, -85, 46, 13, -67, 90, -73, 22, 38, -82, 88, -54, -4, 61, -90, 78, -31,
    25, -70, 90, -80, 43, 9, -57, 87, -87, 57, -9, -43, 80, -90, 70, -25, -25, 70, -90, 80, -43, -9, 57, -87, 87, -57, 9, 43, -80, 90, -70, 25,
    22, -61, 85, -90, 73, -38, -4, 46, -78, 90, -82, 54, -13, -31, 67, -88, 88, -67, 31, 13, -54, 82, -90, 78, -46, 4, 38, -73, 90, -85, 61, -22,
    18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18, 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18,
    13, -38, 61, -78, 88, -90, 85, -73, 54, -31, 4, 22, -46, 67, -82, 90, -90, 82, -67, 46, -22, -4, 31, -54, 73, -85, 90, -88, 78, -61, 38, -13,
    9, -25, 43, -57, 70, -80, 87, -90, 90, -87, 80, -70, 57, -43, 25, -9, -9, 25, -43, 57, -70, 80, -87, 90, -90, 87, -80, 70, -57, 43, -25, 9,
    4, -13, 22, -31, 38, -46, 54, -61, 67, -73, 78, -82, 85, -88, 90, -90, 90, -90, 88, -85, 82, -78, 73, -67, 61, -54, 46, -38, 31, -22, 13, -4
};

/*********************************************************************
* QuantizeInvQuantize
*
*  Quant +iQuant kernel
*********************************************************************/
void QuantizeInvQuantize(
    EB_S16           *coeff,
    const EB_U32     coeffStride,
    EB_S16           *quantCoeff,
    EB_S16           *reconCoeff,
    const EB_U32     qFunc,
    const EB_U32     q_offset,
    const EB_S32     shiftedQBits,
    const EB_S32     shiftedFFunc,
    const EB_S32     iq_offset,
    const EB_S32     shiftNum,
    const EB_U32     areaSize,
    EB_U32           *nonzerocoeff)
{
    EB_U32 coeffLocation = 0;
    EB_U32 rowIndex, colIndex;
    EB_S32 coeffTemp; // based on HM4.0, this variable which is used in quantized coef calculations can be up to 15+16 -transformSizeLog2 bits
    EB_S32 sign;

    EB_S32 coeffTemp1, coeffTemp2;
    EB_S16 qCoeffTmp, rCoeffTmp;
    *nonzerocoeff = 0;
    for (rowIndex = 0; rowIndex < areaSize; ++rowIndex) {
        for (colIndex = 0; colIndex < areaSize; ++colIndex) {
            coeffTemp = (EB_S32)coeff[coeffLocation];
            sign = (coeffTemp < 0 ? -1 : 1);
            //coeffTemp1 = (ABS(coeffTemp) * qFunc + q_offset ) >> shiftedQBits;
            coeffTemp1 = ABS(coeffTemp);
            coeffTemp1 *= qFunc;
            coeffTemp1 += q_offset;
            coeffTemp1 >>= shiftedQBits;
            qCoeffTmp = (EB_S16)CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, (sign*coeffTemp1));
            quantCoeff[coeffLocation] = qCoeffTmp;
            (*nonzerocoeff) += (qCoeffTmp != 0);
            //iQ
            //coeffTemp = ( (CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, quantCoeff[quantLocation]) * shiftedFFunc) + iq_offset) >> shiftNum;
            coeffTemp2 = ((qCoeffTmp * shiftedFFunc) + iq_offset) >> shiftNum;
            rCoeffTmp = (EB_S16)CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, coeffTemp2);
            reconCoeff[coeffLocation] = rCoeffTmp;

            ++coeffLocation;
        }
        coeffLocation += coeffStride - areaSize;
    }
}

#define EB_INTRA_CHROMA_DM          4
#define MAX_TU_SIZE					32

enum COMPONENT_TYPE
{
	COMPONENT_LUMA = 0,			// luma
	COMPONENT_CHROMA = 1,		// chroma (Cb+Cr)
	COMPONENT_CHROMA_CB = 2,	// chroma Cb
	COMPONENT_CHROMA_CR = 3,	// chroma Cr
	COMPONENT_ALL = 4,			// Y+Cb+Cr
	COMPONENT_NONE = 15
};


enum COEFF_SCAN_TYPE2
{
	SCAN_DIAG2 = 0,				// diagonal scan
	SCAN_HOR2,					// first scan is horizontal
	SCAN_VER2					// first scan is vertical
};

/**************************************
* Static Arrays
**************************************/

void UpdateQiQCoef_R(
	EB_S16           *quantCoeff,
	EB_S16           *reconCoeff,
	const EB_U32      coeffStride,
	const EB_S32      shiftedFFunc,
	const EB_S32      iq_offset,
	const EB_S32      shiftNum,
	const EB_U32      areaSize,
	EB_U32           *nonzerocoeff,
	EB_U32            componentType,
	EB_SLICE          sliceType,
	EB_U32            temporalLayer,
	EB_U32            enableCbflag,
	EB_U8             enableContouringQCUpdateFlag)

{

	EB_U32 coeffLocation = 0;

	//EB_U32 rowIndex = 0;
	//EB_U32 colIndex = 0;


    if ((*nonzerocoeff < 10) && enableContouringQCUpdateFlag && sliceType == EB_I_SLICE && temporalLayer == 0 && componentType == 0){

		coeffLocation = (areaSize - 1) + (areaSize - 1) * coeffStride;

		if (quantCoeff[coeffLocation] == 0){
			(*nonzerocoeff)++;
			quantCoeff[coeffLocation] = 1;
			reconCoeff[coeffLocation] = (EB_S16)((quantCoeff[coeffLocation] * shiftedFFunc) + iq_offset) >> shiftNum;
		}
	}

	if ((*nonzerocoeff == 0) && (enableCbflag == 1))
	{
		EB_U32 coeffLocation = ((areaSize - 2)*coeffStride) + (areaSize - 1);
		*nonzerocoeff = 1;
		quantCoeff[coeffLocation] = 1;
		reconCoeff[coeffLocation] = (EB_S16)((quantCoeff[coeffLocation] * shiftedFFunc) + iq_offset) >> shiftNum;
	}
}

/*********************************************************************
* UpdateQiQCoef
*
*  Update the quantized coef and do iQ for the updated coef
*********************************************************************/

void UpdateQiQCoef(
	EB_S16           *quantCoeff,
	EB_S16           *reconCoeff,
	const EB_U32      coeffStride,
	const EB_S32      shiftedFFunc,
	const EB_S32      iq_offset,
	const EB_S32      shiftNum,
	const EB_U32      areaSize,
	EB_U32           *nonzerocoeff,
	EB_U32            componentType,
	EB_SLICE          sliceType,
	EB_U32            temporalLayer,
	EB_U32            enableCbflag,
	EB_U8             enableContouringQCUpdateFlag)

{

	EB_U32 coeffLocation = 0;
	//EB_U32 rowIndex = 0;
	//EB_U32 colIndex = 0;



    if ((*nonzerocoeff < 10) && enableContouringQCUpdateFlag && sliceType == EB_I_SLICE && temporalLayer == 0 && componentType == 0){

        coeffLocation = (areaSize - 1) + (areaSize - 1) * coeffStride;

        if (quantCoeff[coeffLocation] == 0){
            (*nonzerocoeff)++;
            quantCoeff[coeffLocation] = 1;
            reconCoeff[coeffLocation] = (EB_S16)((quantCoeff[coeffLocation] * shiftedFFunc) + iq_offset) >> shiftNum;
        }
    }

  
	if ((*nonzerocoeff == 0) && (enableCbflag == 1))
	{
		EB_U32 coeffLocation = ((areaSize - 2)*coeffStride) + (areaSize - 1);
		*nonzerocoeff = 1;
		quantCoeff[coeffLocation] = 1;
		reconCoeff[coeffLocation] = (EB_S16)((quantCoeff[coeffLocation] * shiftedFFunc) + iq_offset) >> shiftNum;
	}
}

/*********************************************************************
* PartialButterfly32
*   1D 32x32 forward transform using partial butterfly
*
*  residual
*   is the input pointer for pixels
*
*  transformCoefficients
*   is the output after applying the butterfly transform (the output is transposed)
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transform pointers
*
*  shift
*   is the input and shows the number of shifts to right
*********************************************************************/
inline static void PartialButterfly32(
    EB_S16       *residual,
    EB_S16       *transformCoefficients,
    const EB_U32  srcStride,
    const EB_U32  dstStride,
    const EB_U32  shift)
{
    EB_S32 even0, even1, even2, even3, even4, even5, even6, even7, even8, even9, even10, even11, even12, even13, even14, even15;
    EB_S32 odd0, odd1, odd2, odd3, odd4, odd5, odd6, odd7, odd8, odd9, odd10, odd11, odd12, odd13, odd14, odd15;
    EB_S32 evenEven0, evenEven1, evenEven2, evenEven3, evenEven4, evenEven5, evenEven6, evenEven7;
    EB_S32 evenOdd0, evenOdd1, evenOdd2, evenOdd3, evenOdd4, evenOdd5, evenOdd6, evenOdd7;
    EB_S32 evenEvenEven0, evenEvenEven1, evenEvenEven2, evenEvenEven3;
    EB_S32 evenEvenOdd0, evenEvenOdd1, evenEvenOdd2, evenEvenOdd3;
    EB_S32 evenEvenEvenEven0, evenEvenEvenEven1;
    EB_S32 evenEvenEvenOdd0, evenEvenEvenOdd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 32; rowIndex++) {
        rowStrideIndex = rowIndex*srcStride;

        //Calculating even and odd variables
        even0 = residual[rowStrideIndex + 0] + residual[rowStrideIndex + 31];
        even1 = residual[rowStrideIndex + 1] + residual[rowStrideIndex + 30];
        even2 = residual[rowStrideIndex + 2] + residual[rowStrideIndex + 29];
        even3 = residual[rowStrideIndex + 3] + residual[rowStrideIndex + 28];
        even4 = residual[rowStrideIndex + 4] + residual[rowStrideIndex + 27];
        even5 = residual[rowStrideIndex + 5] + residual[rowStrideIndex + 26];
        even6 = residual[rowStrideIndex + 6] + residual[rowStrideIndex + 25];
        even7 = residual[rowStrideIndex + 7] + residual[rowStrideIndex + 24];
        even8 = residual[rowStrideIndex + 8] + residual[rowStrideIndex + 23];
        even9 = residual[rowStrideIndex + 9] + residual[rowStrideIndex + 22];
        even10 = residual[rowStrideIndex + 10] + residual[rowStrideIndex + 21];
        even11 = residual[rowStrideIndex + 11] + residual[rowStrideIndex + 20];
        even12 = residual[rowStrideIndex + 12] + residual[rowStrideIndex + 19];
        even13 = residual[rowStrideIndex + 13] + residual[rowStrideIndex + 18];
        even14 = residual[rowStrideIndex + 14] + residual[rowStrideIndex + 17];
        even15 = residual[rowStrideIndex + 15] + residual[rowStrideIndex + 16];

        odd0 = residual[rowStrideIndex + 0] - residual[rowStrideIndex + 31];
        odd1 = residual[rowStrideIndex + 1] - residual[rowStrideIndex + 30];
        odd2 = residual[rowStrideIndex + 2] - residual[rowStrideIndex + 29];
        odd3 = residual[rowStrideIndex + 3] - residual[rowStrideIndex + 28];
        odd4 = residual[rowStrideIndex + 4] - residual[rowStrideIndex + 27];
        odd5 = residual[rowStrideIndex + 5] - residual[rowStrideIndex + 26];
        odd6 = residual[rowStrideIndex + 6] - residual[rowStrideIndex + 25];
        odd7 = residual[rowStrideIndex + 7] - residual[rowStrideIndex + 24];
        odd8 = residual[rowStrideIndex + 8] - residual[rowStrideIndex + 23];
        odd9 = residual[rowStrideIndex + 9] - residual[rowStrideIndex + 22];
        odd10 = residual[rowStrideIndex + 10] - residual[rowStrideIndex + 21];
        odd11 = residual[rowStrideIndex + 11] - residual[rowStrideIndex + 20];
        odd12 = residual[rowStrideIndex + 12] - residual[rowStrideIndex + 19];
        odd13 = residual[rowStrideIndex + 13] - residual[rowStrideIndex + 18];
        odd14 = residual[rowStrideIndex + 14] - residual[rowStrideIndex + 17];
        odd15 = residual[rowStrideIndex + 15] - residual[rowStrideIndex + 16];

        //Calculating evenEven and evenOdd variables
        evenEven0 = even0 + even15;
        evenEven1 = even1 + even14;
        evenEven2 = even2 + even13;
        evenEven3 = even3 + even12;
        evenEven4 = even4 + even11;
        evenEven5 = even5 + even10;
        evenEven6 = even6 + even9;
        evenEven7 = even7 + even8;

        evenOdd0 = even0 - even15;
        evenOdd1 = even1 - even14;
        evenOdd2 = even2 - even13;
        evenOdd3 = even3 - even12;
        evenOdd4 = even4 - even11;
        evenOdd5 = even5 - even10;
        evenOdd6 = even6 - even9;
        evenOdd7 = even7 - even8;

        //Calculating evenEvenEven and evenEvenOdd variables
        evenEvenEven0 = evenEven0 + evenEven7;
        evenEvenEven1 = evenEven1 + evenEven6;
        evenEvenEven2 = evenEven2 + evenEven5;
        evenEvenEven3 = evenEven3 + evenEven4;

        evenEvenOdd0 = evenEven0 - evenEven7;
        evenEvenOdd1 = evenEven1 - evenEven6;
        evenEvenOdd2 = evenEven2 - evenEven5;
        evenEvenOdd3 = evenEven3 - evenEven4;

        //Calculating evenEvenEvenEven and evenEvenEvenOdd variables
        evenEvenEvenEven0 = evenEvenEven0 + evenEvenEven3;
        evenEvenEvenEven1 = evenEvenEven1 + evenEvenEven2;

        evenEvenEvenOdd0 = evenEvenEven0 - evenEvenEven3;
        evenEvenEvenOdd1 = evenEvenEven1 - evenEvenEven2;

        transformCoefficients[0 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[0 * 32 + 0] * evenEvenEvenEven0 + DctCoef32x32[0 * 32 + 1] * evenEvenEvenEven1 + offset) >> shift);
        transformCoefficients[8 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[8 * 32 + 0] * evenEvenEvenOdd0 + DctCoef32x32[8 * 32 + 1] * evenEvenEvenOdd1 + offset) >> shift);
        transformCoefficients[16 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[16 * 32 + 0] * evenEvenEvenEven0 + DctCoef32x32[16 * 32 + 1] * evenEvenEvenEven1 + offset) >> shift);
        transformCoefficients[24 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[24 * 32 + 0] * evenEvenEvenOdd0 + DctCoef32x32[24 * 32 + 1] * evenEvenEvenOdd1 + offset) >> shift);

        transformCoefficients[4 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[4 * 32 + 0] * evenEvenOdd0 + DctCoef32x32[4 * 32 + 1] * evenEvenOdd1 +
            DctCoef32x32[4 * 32 + 2] * evenEvenOdd2 + DctCoef32x32[4 * 32 + 3] * evenEvenOdd3 + offset) >> shift);
        transformCoefficients[12 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[12 * 32 + 0] * evenEvenOdd0 + DctCoef32x32[12 * 32 + 1] * evenEvenOdd1 +
            DctCoef32x32[12 * 32 + 2] * evenEvenOdd2 + DctCoef32x32[12 * 32 + 3] * evenEvenOdd3 + offset) >> shift);
        transformCoefficients[20 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[20 * 32 + 0] * evenEvenOdd0 + DctCoef32x32[20 * 32 + 1] * evenEvenOdd1 +
            DctCoef32x32[20 * 32 + 2] * evenEvenOdd2 + DctCoef32x32[20 * 32 + 3] * evenEvenOdd3 + offset) >> shift);
        transformCoefficients[28 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[28 * 32 + 0] * evenEvenOdd0 + DctCoef32x32[28 * 32 + 1] * evenEvenOdd1 +
            DctCoef32x32[28 * 32 + 2] * evenEvenOdd2 + DctCoef32x32[28 * 32 + 3] * evenEvenOdd3 + offset) >> shift);

        transformCoefficients[2 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[2 * 32 + 0] * evenOdd0 + DctCoef32x32[2 * 32 + 1] * evenOdd1 +
            DctCoef32x32[2 * 32 + 2] * evenOdd2 + DctCoef32x32[2 * 32 + 3] * evenOdd3 +
            DctCoef32x32[2 * 32 + 4] * evenOdd4 + DctCoef32x32[2 * 32 + 5] * evenOdd5 +
            DctCoef32x32[2 * 32 + 6] * evenOdd6 + DctCoef32x32[2 * 32 + 7] * evenOdd7 + offset) >> shift);
        transformCoefficients[6 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[6 * 32 + 0] * evenOdd0 + DctCoef32x32[6 * 32 + 1] * evenOdd1 +
            DctCoef32x32[6 * 32 + 2] * evenOdd2 + DctCoef32x32[6 * 32 + 3] * evenOdd3 +
            DctCoef32x32[6 * 32 + 4] * evenOdd4 + DctCoef32x32[6 * 32 + 5] * evenOdd5 +
            DctCoef32x32[6 * 32 + 6] * evenOdd6 + DctCoef32x32[6 * 32 + 7] * evenOdd7 + offset) >> shift);
        transformCoefficients[10 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[10 * 32 + 0] * evenOdd0 + DctCoef32x32[10 * 32 + 1] * evenOdd1 +
            DctCoef32x32[10 * 32 + 2] * evenOdd2 + DctCoef32x32[10 * 32 + 3] * evenOdd3 +
            DctCoef32x32[10 * 32 + 4] * evenOdd4 + DctCoef32x32[10 * 32 + 5] * evenOdd5 +
            DctCoef32x32[10 * 32 + 6] * evenOdd6 + DctCoef32x32[10 * 32 + 7] * evenOdd7 + offset) >> shift);
        transformCoefficients[14 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[14 * 32 + 0] * evenOdd0 + DctCoef32x32[14 * 32 + 1] * evenOdd1 +
            DctCoef32x32[14 * 32 + 2] * evenOdd2 + DctCoef32x32[14 * 32 + 3] * evenOdd3 +
            DctCoef32x32[14 * 32 + 4] * evenOdd4 + DctCoef32x32[14 * 32 + 5] * evenOdd5 +
            DctCoef32x32[14 * 32 + 6] * evenOdd6 + DctCoef32x32[14 * 32 + 7] * evenOdd7 + offset) >> shift);
        transformCoefficients[18 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[18 * 32 + 0] * evenOdd0 + DctCoef32x32[18 * 32 + 1] * evenOdd1 +
            DctCoef32x32[18 * 32 + 2] * evenOdd2 + DctCoef32x32[18 * 32 + 3] * evenOdd3 +
            DctCoef32x32[18 * 32 + 4] * evenOdd4 + DctCoef32x32[18 * 32 + 5] * evenOdd5 +
            DctCoef32x32[18 * 32 + 6] * evenOdd6 + DctCoef32x32[18 * 32 + 7] * evenOdd7 + offset) >> shift);
        transformCoefficients[22 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[22 * 32 + 0] * evenOdd0 + DctCoef32x32[22 * 32 + 1] * evenOdd1 +
            DctCoef32x32[22 * 32 + 2] * evenOdd2 + DctCoef32x32[22 * 32 + 3] * evenOdd3 +
            DctCoef32x32[22 * 32 + 4] * evenOdd4 + DctCoef32x32[22 * 32 + 5] * evenOdd5 +
            DctCoef32x32[22 * 32 + 6] * evenOdd6 + DctCoef32x32[22 * 32 + 7] * evenOdd7 + offset) >> shift);
        transformCoefficients[26 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[26 * 32 + 0] * evenOdd0 + DctCoef32x32[26 * 32 + 1] * evenOdd1 +
            DctCoef32x32[26 * 32 + 2] * evenOdd2 + DctCoef32x32[26 * 32 + 3] * evenOdd3 +
            DctCoef32x32[26 * 32 + 4] * evenOdd4 + DctCoef32x32[26 * 32 + 5] * evenOdd5 +
            DctCoef32x32[26 * 32 + 6] * evenOdd6 + DctCoef32x32[26 * 32 + 7] * evenOdd7 + offset) >> shift);
        transformCoefficients[30 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[30 * 32 + 0] * evenOdd0 + DctCoef32x32[30 * 32 + 1] * evenOdd1 +
            DctCoef32x32[30 * 32 + 2] * evenOdd2 + DctCoef32x32[30 * 32 + 3] * evenOdd3 +
            DctCoef32x32[30 * 32 + 4] * evenOdd4 + DctCoef32x32[30 * 32 + 5] * evenOdd5 +
            DctCoef32x32[30 * 32 + 6] * evenOdd6 + DctCoef32x32[30 * 32 + 7] * evenOdd7 + offset) >> shift);

        transformCoefficients[1 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[1 * 32 + 0] * odd0 + DctCoef32x32[1 * 32 + 1] * odd1 + DctCoef32x32[1 * 32 + 2] * odd2 + DctCoef32x32[1 * 32 + 3] * odd3 +
            DctCoef32x32[1 * 32 + 4] * odd4 + DctCoef32x32[1 * 32 + 5] * odd5 + DctCoef32x32[1 * 32 + 6] * odd6 + DctCoef32x32[1 * 32 + 7] * odd7 +
            DctCoef32x32[1 * 32 + 8] * odd8 + DctCoef32x32[1 * 32 + 9] * odd9 + DctCoef32x32[1 * 32 + 10] * odd10 + DctCoef32x32[1 * 32 + 11] * odd11 +
            DctCoef32x32[1 * 32 + 12] * odd12 + DctCoef32x32[1 * 32 + 13] * odd13 + DctCoef32x32[1 * 32 + 14] * odd14 + DctCoef32x32[1 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[3 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[3 * 32 + 0] * odd0 + DctCoef32x32[3 * 32 + 1] * odd1 + DctCoef32x32[3 * 32 + 2] * odd2 + DctCoef32x32[3 * 32 + 3] * odd3 +
            DctCoef32x32[3 * 32 + 4] * odd4 + DctCoef32x32[3 * 32 + 5] * odd5 + DctCoef32x32[3 * 32 + 6] * odd6 + DctCoef32x32[3 * 32 + 7] * odd7 +
            DctCoef32x32[3 * 32 + 8] * odd8 + DctCoef32x32[3 * 32 + 9] * odd9 + DctCoef32x32[3 * 32 + 10] * odd10 + DctCoef32x32[3 * 32 + 11] * odd11 +
            DctCoef32x32[3 * 32 + 12] * odd12 + DctCoef32x32[3 * 32 + 13] * odd13 + DctCoef32x32[3 * 32 + 14] * odd14 + DctCoef32x32[3 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[5 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[5 * 32 + 0] * odd0 + DctCoef32x32[5 * 32 + 1] * odd1 + DctCoef32x32[5 * 32 + 2] * odd2 + DctCoef32x32[5 * 32 + 3] * odd3 +
            DctCoef32x32[5 * 32 + 4] * odd4 + DctCoef32x32[5 * 32 + 5] * odd5 + DctCoef32x32[5 * 32 + 6] * odd6 + DctCoef32x32[5 * 32 + 7] * odd7 +
            DctCoef32x32[5 * 32 + 8] * odd8 + DctCoef32x32[5 * 32 + 9] * odd9 + DctCoef32x32[5 * 32 + 10] * odd10 + DctCoef32x32[5 * 32 + 11] * odd11 +
            DctCoef32x32[5 * 32 + 12] * odd12 + DctCoef32x32[5 * 32 + 13] * odd13 + DctCoef32x32[5 * 32 + 14] * odd14 + DctCoef32x32[5 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[7 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[7 * 32 + 0] * odd0 + DctCoef32x32[7 * 32 + 1] * odd1 + DctCoef32x32[7 * 32 + 2] * odd2 + DctCoef32x32[7 * 32 + 3] * odd3 +
            DctCoef32x32[7 * 32 + 4] * odd4 + DctCoef32x32[7 * 32 + 5] * odd5 + DctCoef32x32[7 * 32 + 6] * odd6 + DctCoef32x32[7 * 32 + 7] * odd7 +
            DctCoef32x32[7 * 32 + 8] * odd8 + DctCoef32x32[7 * 32 + 9] * odd9 + DctCoef32x32[7 * 32 + 10] * odd10 + DctCoef32x32[7 * 32 + 11] * odd11 +
            DctCoef32x32[7 * 32 + 12] * odd12 + DctCoef32x32[7 * 32 + 13] * odd13 + DctCoef32x32[7 * 32 + 14] * odd14 + DctCoef32x32[7 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[9 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[9 * 32 + 0] * odd0 + DctCoef32x32[9 * 32 + 1] * odd1 + DctCoef32x32[9 * 32 + 2] * odd2 + DctCoef32x32[9 * 32 + 3] * odd3 +
            DctCoef32x32[9 * 32 + 4] * odd4 + DctCoef32x32[9 * 32 + 5] * odd5 + DctCoef32x32[9 * 32 + 6] * odd6 + DctCoef32x32[9 * 32 + 7] * odd7 +
            DctCoef32x32[9 * 32 + 8] * odd8 + DctCoef32x32[9 * 32 + 9] * odd9 + DctCoef32x32[9 * 32 + 10] * odd10 + DctCoef32x32[9 * 32 + 11] * odd11 +
            DctCoef32x32[9 * 32 + 12] * odd12 + DctCoef32x32[9 * 32 + 13] * odd13 + DctCoef32x32[9 * 32 + 14] * odd14 + DctCoef32x32[9 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[11 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[11 * 32 + 0] * odd0 + DctCoef32x32[11 * 32 + 1] * odd1 + DctCoef32x32[11 * 32 + 2] * odd2 + DctCoef32x32[11 * 32 + 3] * odd3 +
            DctCoef32x32[11 * 32 + 4] * odd4 + DctCoef32x32[11 * 32 + 5] * odd5 + DctCoef32x32[11 * 32 + 6] * odd6 + DctCoef32x32[11 * 32 + 7] * odd7 +
            DctCoef32x32[11 * 32 + 8] * odd8 + DctCoef32x32[11 * 32 + 9] * odd9 + DctCoef32x32[11 * 32 + 10] * odd10 + DctCoef32x32[11 * 32 + 11] * odd11 +
            DctCoef32x32[11 * 32 + 12] * odd12 + DctCoef32x32[11 * 32 + 13] * odd13 + DctCoef32x32[11 * 32 + 14] * odd14 + DctCoef32x32[11 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[13 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[13 * 32 + 0] * odd0 + DctCoef32x32[13 * 32 + 1] * odd1 + DctCoef32x32[13 * 32 + 2] * odd2 + DctCoef32x32[13 * 32 + 3] * odd3 +
            DctCoef32x32[13 * 32 + 4] * odd4 + DctCoef32x32[13 * 32 + 5] * odd5 + DctCoef32x32[13 * 32 + 6] * odd6 + DctCoef32x32[13 * 32 + 7] * odd7 +
            DctCoef32x32[13 * 32 + 8] * odd8 + DctCoef32x32[13 * 32 + 9] * odd9 + DctCoef32x32[13 * 32 + 10] * odd10 + DctCoef32x32[13 * 32 + 11] * odd11 +
            DctCoef32x32[13 * 32 + 12] * odd12 + DctCoef32x32[13 * 32 + 13] * odd13 + DctCoef32x32[13 * 32 + 14] * odd14 + DctCoef32x32[13 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[15 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[15 * 32 + 0] * odd0 + DctCoef32x32[15 * 32 + 1] * odd1 + DctCoef32x32[15 * 32 + 2] * odd2 + DctCoef32x32[15 * 32 + 3] * odd3 +
            DctCoef32x32[15 * 32 + 4] * odd4 + DctCoef32x32[15 * 32 + 5] * odd5 + DctCoef32x32[15 * 32 + 6] * odd6 + DctCoef32x32[15 * 32 + 7] * odd7 +
            DctCoef32x32[15 * 32 + 8] * odd8 + DctCoef32x32[15 * 32 + 9] * odd9 + DctCoef32x32[15 * 32 + 10] * odd10 + DctCoef32x32[15 * 32 + 11] * odd11 +
            DctCoef32x32[15 * 32 + 12] * odd12 + DctCoef32x32[15 * 32 + 13] * odd13 + DctCoef32x32[15 * 32 + 14] * odd14 + DctCoef32x32[15 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[17 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[17 * 32 + 0] * odd0 + DctCoef32x32[17 * 32 + 1] * odd1 + DctCoef32x32[17 * 32 + 2] * odd2 + DctCoef32x32[17 * 32 + 3] * odd3 +
            DctCoef32x32[17 * 32 + 4] * odd4 + DctCoef32x32[17 * 32 + 5] * odd5 + DctCoef32x32[17 * 32 + 6] * odd6 + DctCoef32x32[17 * 32 + 7] * odd7 +
            DctCoef32x32[17 * 32 + 8] * odd8 + DctCoef32x32[17 * 32 + 9] * odd9 + DctCoef32x32[17 * 32 + 10] * odd10 + DctCoef32x32[17 * 32 + 11] * odd11 +
            DctCoef32x32[17 * 32 + 12] * odd12 + DctCoef32x32[17 * 32 + 13] * odd13 + DctCoef32x32[17 * 32 + 14] * odd14 + DctCoef32x32[17 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[19 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[19 * 32 + 0] * odd0 + DctCoef32x32[19 * 32 + 1] * odd1 + DctCoef32x32[19 * 32 + 2] * odd2 + DctCoef32x32[19 * 32 + 3] * odd3 +
            DctCoef32x32[19 * 32 + 4] * odd4 + DctCoef32x32[19 * 32 + 5] * odd5 + DctCoef32x32[19 * 32 + 6] * odd6 + DctCoef32x32[19 * 32 + 7] * odd7 +
            DctCoef32x32[19 * 32 + 8] * odd8 + DctCoef32x32[19 * 32 + 9] * odd9 + DctCoef32x32[19 * 32 + 10] * odd10 + DctCoef32x32[19 * 32 + 11] * odd11 +
            DctCoef32x32[19 * 32 + 12] * odd12 + DctCoef32x32[19 * 32 + 13] * odd13 + DctCoef32x32[19 * 32 + 14] * odd14 + DctCoef32x32[19 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[21 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[21 * 32 + 0] * odd0 + DctCoef32x32[21 * 32 + 1] * odd1 + DctCoef32x32[21 * 32 + 2] * odd2 + DctCoef32x32[21 * 32 + 3] * odd3 +
            DctCoef32x32[21 * 32 + 4] * odd4 + DctCoef32x32[21 * 32 + 5] * odd5 + DctCoef32x32[21 * 32 + 6] * odd6 + DctCoef32x32[21 * 32 + 7] * odd7 +
            DctCoef32x32[21 * 32 + 8] * odd8 + DctCoef32x32[21 * 32 + 9] * odd9 + DctCoef32x32[21 * 32 + 10] * odd10 + DctCoef32x32[21 * 32 + 11] * odd11 +
            DctCoef32x32[21 * 32 + 12] * odd12 + DctCoef32x32[21 * 32 + 13] * odd13 + DctCoef32x32[21 * 32 + 14] * odd14 + DctCoef32x32[21 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[23 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[23 * 32 + 0] * odd0 + DctCoef32x32[23 * 32 + 1] * odd1 + DctCoef32x32[23 * 32 + 2] * odd2 + DctCoef32x32[23 * 32 + 3] * odd3 +
            DctCoef32x32[23 * 32 + 4] * odd4 + DctCoef32x32[23 * 32 + 5] * odd5 + DctCoef32x32[23 * 32 + 6] * odd6 + DctCoef32x32[23 * 32 + 7] * odd7 +
            DctCoef32x32[23 * 32 + 8] * odd8 + DctCoef32x32[23 * 32 + 9] * odd9 + DctCoef32x32[23 * 32 + 10] * odd10 + DctCoef32x32[23 * 32 + 11] * odd11 +
            DctCoef32x32[23 * 32 + 12] * odd12 + DctCoef32x32[23 * 32 + 13] * odd13 + DctCoef32x32[23 * 32 + 14] * odd14 + DctCoef32x32[23 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[25 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[25 * 32 + 0] * odd0 + DctCoef32x32[25 * 32 + 1] * odd1 + DctCoef32x32[25 * 32 + 2] * odd2 + DctCoef32x32[25 * 32 + 3] * odd3 +
            DctCoef32x32[25 * 32 + 4] * odd4 + DctCoef32x32[25 * 32 + 5] * odd5 + DctCoef32x32[25 * 32 + 6] * odd6 + DctCoef32x32[25 * 32 + 7] * odd7 +
            DctCoef32x32[25 * 32 + 8] * odd8 + DctCoef32x32[25 * 32 + 9] * odd9 + DctCoef32x32[25 * 32 + 10] * odd10 + DctCoef32x32[25 * 32 + 11] * odd11 +
            DctCoef32x32[25 * 32 + 12] * odd12 + DctCoef32x32[25 * 32 + 13] * odd13 + DctCoef32x32[25 * 32 + 14] * odd14 + DctCoef32x32[25 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[27 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[27 * 32 + 0] * odd0 + DctCoef32x32[27 * 32 + 1] * odd1 + DctCoef32x32[27 * 32 + 2] * odd2 + DctCoef32x32[27 * 32 + 3] * odd3 +
            DctCoef32x32[27 * 32 + 4] * odd4 + DctCoef32x32[27 * 32 + 5] * odd5 + DctCoef32x32[27 * 32 + 6] * odd6 + DctCoef32x32[27 * 32 + 7] * odd7 +
            DctCoef32x32[27 * 32 + 8] * odd8 + DctCoef32x32[27 * 32 + 9] * odd9 + DctCoef32x32[27 * 32 + 10] * odd10 + DctCoef32x32[27 * 32 + 11] * odd11 +
            DctCoef32x32[27 * 32 + 12] * odd12 + DctCoef32x32[27 * 32 + 13] * odd13 + DctCoef32x32[27 * 32 + 14] * odd14 + DctCoef32x32[27 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[29 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[29 * 32 + 0] * odd0 + DctCoef32x32[29 * 32 + 1] * odd1 + DctCoef32x32[29 * 32 + 2] * odd2 + DctCoef32x32[29 * 32 + 3] * odd3 +
            DctCoef32x32[29 * 32 + 4] * odd4 + DctCoef32x32[29 * 32 + 5] * odd5 + DctCoef32x32[29 * 32 + 6] * odd6 + DctCoef32x32[29 * 32 + 7] * odd7 +
            DctCoef32x32[29 * 32 + 8] * odd8 + DctCoef32x32[29 * 32 + 9] * odd9 + DctCoef32x32[29 * 32 + 10] * odd10 + DctCoef32x32[29 * 32 + 11] * odd11 +
            DctCoef32x32[29 * 32 + 12] * odd12 + DctCoef32x32[29 * 32 + 13] * odd13 + DctCoef32x32[29 * 32 + 14] * odd14 + DctCoef32x32[29 * 32 + 15] * odd15 +
            offset) >> shift);
        transformCoefficients[31 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[31 * 32 + 0] * odd0 + DctCoef32x32[31 * 32 + 1] * odd1 + DctCoef32x32[31 * 32 + 2] * odd2 + DctCoef32x32[31 * 32 + 3] * odd3 +
            DctCoef32x32[31 * 32 + 4] * odd4 + DctCoef32x32[31 * 32 + 5] * odd5 + DctCoef32x32[31 * 32 + 6] * odd6 + DctCoef32x32[31 * 32 + 7] * odd7 +
            DctCoef32x32[31 * 32 + 8] * odd8 + DctCoef32x32[31 * 32 + 9] * odd9 + DctCoef32x32[31 * 32 + 10] * odd10 + DctCoef32x32[31 * 32 + 11] * odd11 +
            DctCoef32x32[31 * 32 + 12] * odd12 + DctCoef32x32[31 * 32 + 13] * odd13 + DctCoef32x32[31 * 32 + 14] * odd14 + DctCoef32x32[31 * 32 + 15] * odd15 +
            offset) >> shift);
    }
}

inline static void PartialButterfly32Estimate(
	EB_S16       *residual,
	EB_S16       *transformCoefficients,
	const EB_U32  srcStride,
	const EB_U32  dstStride,
	const EB_U32  shift)
{
	EB_S16 even0, even1, even2, even3, even4, even5, even6, even7, even8, even9, even10, even11, even12, even13, even14, even15;
	EB_S16 odd0, odd1, odd2, odd3, odd4, odd5, odd6, odd7, odd8, odd9, odd10, odd11, odd12, odd13, odd14, odd15;
	EB_S16 evenEven0, evenEven1, evenEven2, evenEven3, evenEven4, evenEven5, evenEven6, evenEven7;
	EB_S16 evenOdd0, evenOdd1, evenOdd2, evenOdd3, evenOdd4, evenOdd5, evenOdd6, evenOdd7;
	EB_S32 evenEvenEven0, evenEvenEven1, evenEvenEven2, evenEvenEven3;
	EB_S32 evenEvenOdd0, evenEvenOdd1, evenEvenOdd2, evenEvenOdd3;
	EB_S32 evenEvenEvenEven0, evenEvenEvenEven1;
	EB_S32 evenEvenEvenOdd0, evenEvenEvenOdd1;
	EB_U32 rowIndex, rowStrideIndex;
	const EB_S16 offset = 1 << (shift - 1);

	for (rowIndex = 0; rowIndex < 32; rowIndex++) {
		rowStrideIndex = rowIndex*srcStride;

		//Calculating even and odd variables
		even0 = residual[rowStrideIndex + 0] + residual[rowStrideIndex + 31];
		even1 = residual[rowStrideIndex + 1] + residual[rowStrideIndex + 30];
		even2 = residual[rowStrideIndex + 2] + residual[rowStrideIndex + 29];
		even3 = residual[rowStrideIndex + 3] + residual[rowStrideIndex + 28];
		even4 = residual[rowStrideIndex + 4] + residual[rowStrideIndex + 27];
		even5 = residual[rowStrideIndex + 5] + residual[rowStrideIndex + 26];
		even6 = residual[rowStrideIndex + 6] + residual[rowStrideIndex + 25];
		even7 = residual[rowStrideIndex + 7] + residual[rowStrideIndex + 24];
		even8 = residual[rowStrideIndex + 8] + residual[rowStrideIndex + 23];
		even9 = residual[rowStrideIndex + 9] + residual[rowStrideIndex + 22];
		even10 = residual[rowStrideIndex + 10] + residual[rowStrideIndex + 21];
		even11 = residual[rowStrideIndex + 11] + residual[rowStrideIndex + 20];
		even12 = residual[rowStrideIndex + 12] + residual[rowStrideIndex + 19];
		even13 = residual[rowStrideIndex + 13] + residual[rowStrideIndex + 18];
		even14 = residual[rowStrideIndex + 14] + residual[rowStrideIndex + 17];
		even15 = residual[rowStrideIndex + 15] + residual[rowStrideIndex + 16];

		odd0 = residual[rowStrideIndex + 0] - residual[rowStrideIndex + 31];
		odd1 = residual[rowStrideIndex + 1] - residual[rowStrideIndex + 30];
		odd2 = residual[rowStrideIndex + 2] - residual[rowStrideIndex + 29];
		odd3 = residual[rowStrideIndex + 3] - residual[rowStrideIndex + 28];
		odd4 = residual[rowStrideIndex + 4] - residual[rowStrideIndex + 27];
		odd5 = residual[rowStrideIndex + 5] - residual[rowStrideIndex + 26];
		odd6 = residual[rowStrideIndex + 6] - residual[rowStrideIndex + 25];
		odd7 = residual[rowStrideIndex + 7] - residual[rowStrideIndex + 24];
		odd8 = residual[rowStrideIndex + 8] - residual[rowStrideIndex + 23];
		odd9 = residual[rowStrideIndex + 9] - residual[rowStrideIndex + 22];
		odd10 = residual[rowStrideIndex + 10] - residual[rowStrideIndex + 21];
		odd11 = residual[rowStrideIndex + 11] - residual[rowStrideIndex + 20];
		odd12 = residual[rowStrideIndex + 12] - residual[rowStrideIndex + 19];
		odd13 = residual[rowStrideIndex + 13] - residual[rowStrideIndex + 18];
		odd14 = residual[rowStrideIndex + 14] - residual[rowStrideIndex + 17];
		odd15 = residual[rowStrideIndex + 15] - residual[rowStrideIndex + 16];

		//Calculating evenEven and evenOdd variables
		evenEven0 = even0 + even15;
		evenEven1 = even1 + even14;
		evenEven2 = even2 + even13;
		evenEven3 = even3 + even12;
		evenEven4 = even4 + even11;
		evenEven5 = even5 + even10;
		evenEven6 = even6 + even9;
		evenEven7 = even7 + even8;

		evenOdd0 = even0 - even15;
		evenOdd1 = even1 - even14;
		evenOdd2 = even2 - even13;
		evenOdd3 = even3 - even12;
		evenOdd4 = even4 - even11;
		evenOdd5 = even5 - even10;
		evenOdd6 = even6 - even9;
		evenOdd7 = even7 - even8;

		//Calculating evenEvenEven and evenEvenOdd variables
		evenEvenEven0 = evenEven0 + evenEven7;
		evenEvenEven1 = evenEven1 + evenEven6;
		evenEvenEven2 = evenEven2 + evenEven5;
		evenEvenEven3 = evenEven3 + evenEven4;

		evenEvenOdd0 = evenEven0 - evenEven7;
		evenEvenOdd1 = evenEven1 - evenEven6;
		evenEvenOdd2 = evenEven2 - evenEven5;
		evenEvenOdd3 = evenEven3 - evenEven4;

		//Calculating evenEvenEvenEven and evenEvenEvenOdd variables
		evenEvenEvenEven0 = evenEvenEven0 + evenEvenEven3;
		evenEvenEvenEven1 = evenEvenEven1 + evenEvenEven2;

		evenEvenEvenOdd0 = evenEvenEven0 - evenEvenEven3;
		evenEvenEvenOdd1 = evenEvenEven1 - evenEvenEven2;

		transformCoefficients[0 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[0 * 32 + 0] * evenEvenEvenEven0 + DctCoef32x32[0 * 32 + 1] * evenEvenEvenEven1 + offset) >> shift);
		transformCoefficients[8 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[8 * 32 + 0] * evenEvenEvenOdd0 + DctCoef32x32[8 * 32 + 1] * evenEvenEvenOdd1 + offset) >> shift);
		transformCoefficients[16 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[16 * 32 + 0] * evenEvenEvenEven0 + DctCoef32x32[16 * 32 + 1] * evenEvenEvenEven1 + offset) >> shift);
		transformCoefficients[24 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[24 * 32 + 0] * evenEvenEvenOdd0 + DctCoef32x32[24 * 32 + 1] * evenEvenEvenOdd1 + offset) >> shift);

		transformCoefficients[4 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[4 * 32 + 0] * evenEvenOdd0 + DctCoef32x32[4 * 32 + 1] * evenEvenOdd1 +
			DctCoef32x32[4 * 32 + 2] * evenEvenOdd2 + DctCoef32x32[4 * 32 + 3] * evenEvenOdd3 + offset) >> shift);
		transformCoefficients[12 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[12 * 32 + 0] * evenEvenOdd0 + DctCoef32x32[12 * 32 + 1] * evenEvenOdd1 +
			DctCoef32x32[12 * 32 + 2] * evenEvenOdd2 + DctCoef32x32[12 * 32 + 3] * evenEvenOdd3 + offset) >> shift);
		transformCoefficients[20 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[20 * 32 + 0] * evenEvenOdd0 + DctCoef32x32[20 * 32 + 1] * evenEvenOdd1 +
			DctCoef32x32[20 * 32 + 2] * evenEvenOdd2 + DctCoef32x32[20 * 32 + 3] * evenEvenOdd3 + offset) >> shift);
		transformCoefficients[28 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[28 * 32 + 0] * evenEvenOdd0 + DctCoef32x32[28 * 32 + 1] * evenEvenOdd1 +
			DctCoef32x32[28 * 32 + 2] * evenEvenOdd2 + DctCoef32x32[28 * 32 + 3] * evenEvenOdd3 + offset) >> shift);

		transformCoefficients[2 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[2 * 32 + 0] * evenOdd0 + DctCoef32x32[2 * 32 + 1] * evenOdd1 +
			DctCoef32x32[2 * 32 + 2] * evenOdd2 + DctCoef32x32[2 * 32 + 3] * evenOdd3 +
			DctCoef32x32[2 * 32 + 4] * evenOdd4 + DctCoef32x32[2 * 32 + 5] * evenOdd5 +
			DctCoef32x32[2 * 32 + 6] * evenOdd6 + DctCoef32x32[2 * 32 + 7] * evenOdd7 + offset) >> shift);
		transformCoefficients[6 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[6 * 32 + 0] * evenOdd0 + DctCoef32x32[6 * 32 + 1] * evenOdd1 +
			DctCoef32x32[6 * 32 + 2] * evenOdd2 + DctCoef32x32[6 * 32 + 3] * evenOdd3 +
			DctCoef32x32[6 * 32 + 4] * evenOdd4 + DctCoef32x32[6 * 32 + 5] * evenOdd5 +
			DctCoef32x32[6 * 32 + 6] * evenOdd6 + DctCoef32x32[6 * 32 + 7] * evenOdd7 + offset) >> shift);
		transformCoefficients[10 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[10 * 32 + 0] * evenOdd0 + DctCoef32x32[10 * 32 + 1] * evenOdd1 +
			DctCoef32x32[10 * 32 + 2] * evenOdd2 + DctCoef32x32[10 * 32 + 3] * evenOdd3 +
			DctCoef32x32[10 * 32 + 4] * evenOdd4 + DctCoef32x32[10 * 32 + 5] * evenOdd5 +
			DctCoef32x32[10 * 32 + 6] * evenOdd6 + DctCoef32x32[10 * 32 + 7] * evenOdd7 + offset) >> shift);
		transformCoefficients[14 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[14 * 32 + 0] * evenOdd0 + DctCoef32x32[14 * 32 + 1] * evenOdd1 +
			DctCoef32x32[14 * 32 + 2] * evenOdd2 + DctCoef32x32[14 * 32 + 3] * evenOdd3 +
			DctCoef32x32[14 * 32 + 4] * evenOdd4 + DctCoef32x32[14 * 32 + 5] * evenOdd5 +
			DctCoef32x32[14 * 32 + 6] * evenOdd6 + DctCoef32x32[14 * 32 + 7] * evenOdd7 + offset) >> shift);
		transformCoefficients[18 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[18 * 32 + 0] * evenOdd0 + DctCoef32x32[18 * 32 + 1] * evenOdd1 +
			DctCoef32x32[18 * 32 + 2] * evenOdd2 + DctCoef32x32[18 * 32 + 3] * evenOdd3 +
			DctCoef32x32[18 * 32 + 4] * evenOdd4 + DctCoef32x32[18 * 32 + 5] * evenOdd5 +
			DctCoef32x32[18 * 32 + 6] * evenOdd6 + DctCoef32x32[18 * 32 + 7] * evenOdd7 + offset) >> shift);
		transformCoefficients[22 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[22 * 32 + 0] * evenOdd0 + DctCoef32x32[22 * 32 + 1] * evenOdd1 +
			DctCoef32x32[22 * 32 + 2] * evenOdd2 + DctCoef32x32[22 * 32 + 3] * evenOdd3 +
			DctCoef32x32[22 * 32 + 4] * evenOdd4 + DctCoef32x32[22 * 32 + 5] * evenOdd5 +
			DctCoef32x32[22 * 32 + 6] * evenOdd6 + DctCoef32x32[22 * 32 + 7] * evenOdd7 + offset) >> shift);
		transformCoefficients[26 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[26 * 32 + 0] * evenOdd0 + DctCoef32x32[26 * 32 + 1] * evenOdd1 +
			DctCoef32x32[26 * 32 + 2] * evenOdd2 + DctCoef32x32[26 * 32 + 3] * evenOdd3 +
			DctCoef32x32[26 * 32 + 4] * evenOdd4 + DctCoef32x32[26 * 32 + 5] * evenOdd5 +
			DctCoef32x32[26 * 32 + 6] * evenOdd6 + DctCoef32x32[26 * 32 + 7] * evenOdd7 + offset) >> shift);
		transformCoefficients[30 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[30 * 32 + 0] * evenOdd0 + DctCoef32x32[30 * 32 + 1] * evenOdd1 +
			DctCoef32x32[30 * 32 + 2] * evenOdd2 + DctCoef32x32[30 * 32 + 3] * evenOdd3 +
			DctCoef32x32[30 * 32 + 4] * evenOdd4 + DctCoef32x32[30 * 32 + 5] * evenOdd5 +
			DctCoef32x32[30 * 32 + 6] * evenOdd6 + DctCoef32x32[30 * 32 + 7] * evenOdd7 + offset) >> shift);

		transformCoefficients[1 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[1 * 32 + 0] * odd0 + DctCoef32x32[1 * 32 + 1] * odd1 + DctCoef32x32[1 * 32 + 2] * odd2 + DctCoef32x32[1 * 32 + 3] * odd3 +
			DctCoef32x32[1 * 32 + 4] * odd4 + DctCoef32x32[1 * 32 + 5] * odd5 + DctCoef32x32[1 * 32 + 6] * odd6 + DctCoef32x32[1 * 32 + 7] * odd7 +
			DctCoef32x32[1 * 32 + 8] * odd8 + DctCoef32x32[1 * 32 + 9] * odd9 + DctCoef32x32[1 * 32 + 10] * odd10 + DctCoef32x32[1 * 32 + 11] * odd11 +
			DctCoef32x32[1 * 32 + 12] * odd12 + DctCoef32x32[1 * 32 + 13] * odd13 + DctCoef32x32[1 * 32 + 14] * odd14 + DctCoef32x32[1 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[3 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[3 * 32 + 0] * odd0 + DctCoef32x32[3 * 32 + 1] * odd1 + DctCoef32x32[3 * 32 + 2] * odd2 + DctCoef32x32[3 * 32 + 3] * odd3 +
			DctCoef32x32[3 * 32 + 4] * odd4 + DctCoef32x32[3 * 32 + 5] * odd5 + DctCoef32x32[3 * 32 + 6] * odd6 + DctCoef32x32[3 * 32 + 7] * odd7 +
			DctCoef32x32[3 * 32 + 8] * odd8 + DctCoef32x32[3 * 32 + 9] * odd9 + DctCoef32x32[3 * 32 + 10] * odd10 + DctCoef32x32[3 * 32 + 11] * odd11 +
			DctCoef32x32[3 * 32 + 12] * odd12 + DctCoef32x32[3 * 32 + 13] * odd13 + DctCoef32x32[3 * 32 + 14] * odd14 + DctCoef32x32[3 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[5 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[5 * 32 + 0] * odd0 + DctCoef32x32[5 * 32 + 1] * odd1 + DctCoef32x32[5 * 32 + 2] * odd2 + DctCoef32x32[5 * 32 + 3] * odd3 +
			DctCoef32x32[5 * 32 + 4] * odd4 + DctCoef32x32[5 * 32 + 5] * odd5 + DctCoef32x32[5 * 32 + 6] * odd6 + DctCoef32x32[5 * 32 + 7] * odd7 +
			DctCoef32x32[5 * 32 + 8] * odd8 + DctCoef32x32[5 * 32 + 9] * odd9 + DctCoef32x32[5 * 32 + 10] * odd10 + DctCoef32x32[5 * 32 + 11] * odd11 +
			DctCoef32x32[5 * 32 + 12] * odd12 + DctCoef32x32[5 * 32 + 13] * odd13 + DctCoef32x32[5 * 32 + 14] * odd14 + DctCoef32x32[5 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[7 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[7 * 32 + 0] * odd0 + DctCoef32x32[7 * 32 + 1] * odd1 + DctCoef32x32[7 * 32 + 2] * odd2 + DctCoef32x32[7 * 32 + 3] * odd3 +
			DctCoef32x32[7 * 32 + 4] * odd4 + DctCoef32x32[7 * 32 + 5] * odd5 + DctCoef32x32[7 * 32 + 6] * odd6 + DctCoef32x32[7 * 32 + 7] * odd7 +
			DctCoef32x32[7 * 32 + 8] * odd8 + DctCoef32x32[7 * 32 + 9] * odd9 + DctCoef32x32[7 * 32 + 10] * odd10 + DctCoef32x32[7 * 32 + 11] * odd11 +
			DctCoef32x32[7 * 32 + 12] * odd12 + DctCoef32x32[7 * 32 + 13] * odd13 + DctCoef32x32[7 * 32 + 14] * odd14 + DctCoef32x32[7 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[9 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[9 * 32 + 0] * odd0 + DctCoef32x32[9 * 32 + 1] * odd1 + DctCoef32x32[9 * 32 + 2] * odd2 + DctCoef32x32[9 * 32 + 3] * odd3 +
			DctCoef32x32[9 * 32 + 4] * odd4 + DctCoef32x32[9 * 32 + 5] * odd5 + DctCoef32x32[9 * 32 + 6] * odd6 + DctCoef32x32[9 * 32 + 7] * odd7 +
			DctCoef32x32[9 * 32 + 8] * odd8 + DctCoef32x32[9 * 32 + 9] * odd9 + DctCoef32x32[9 * 32 + 10] * odd10 + DctCoef32x32[9 * 32 + 11] * odd11 +
			DctCoef32x32[9 * 32 + 12] * odd12 + DctCoef32x32[9 * 32 + 13] * odd13 + DctCoef32x32[9 * 32 + 14] * odd14 + DctCoef32x32[9 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[11 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[11 * 32 + 0] * odd0 + DctCoef32x32[11 * 32 + 1] * odd1 + DctCoef32x32[11 * 32 + 2] * odd2 + DctCoef32x32[11 * 32 + 3] * odd3 +
			DctCoef32x32[11 * 32 + 4] * odd4 + DctCoef32x32[11 * 32 + 5] * odd5 + DctCoef32x32[11 * 32 + 6] * odd6 + DctCoef32x32[11 * 32 + 7] * odd7 +
			DctCoef32x32[11 * 32 + 8] * odd8 + DctCoef32x32[11 * 32 + 9] * odd9 + DctCoef32x32[11 * 32 + 10] * odd10 + DctCoef32x32[11 * 32 + 11] * odd11 +
			DctCoef32x32[11 * 32 + 12] * odd12 + DctCoef32x32[11 * 32 + 13] * odd13 + DctCoef32x32[11 * 32 + 14] * odd14 + DctCoef32x32[11 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[13 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[13 * 32 + 0] * odd0 + DctCoef32x32[13 * 32 + 1] * odd1 + DctCoef32x32[13 * 32 + 2] * odd2 + DctCoef32x32[13 * 32 + 3] * odd3 +
			DctCoef32x32[13 * 32 + 4] * odd4 + DctCoef32x32[13 * 32 + 5] * odd5 + DctCoef32x32[13 * 32 + 6] * odd6 + DctCoef32x32[13 * 32 + 7] * odd7 +
			DctCoef32x32[13 * 32 + 8] * odd8 + DctCoef32x32[13 * 32 + 9] * odd9 + DctCoef32x32[13 * 32 + 10] * odd10 + DctCoef32x32[13 * 32 + 11] * odd11 +
			DctCoef32x32[13 * 32 + 12] * odd12 + DctCoef32x32[13 * 32 + 13] * odd13 + DctCoef32x32[13 * 32 + 14] * odd14 + DctCoef32x32[13 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[15 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[15 * 32 + 0] * odd0 + DctCoef32x32[15 * 32 + 1] * odd1 + DctCoef32x32[15 * 32 + 2] * odd2 + DctCoef32x32[15 * 32 + 3] * odd3 +
			DctCoef32x32[15 * 32 + 4] * odd4 + DctCoef32x32[15 * 32 + 5] * odd5 + DctCoef32x32[15 * 32 + 6] * odd6 + DctCoef32x32[15 * 32 + 7] * odd7 +
			DctCoef32x32[15 * 32 + 8] * odd8 + DctCoef32x32[15 * 32 + 9] * odd9 + DctCoef32x32[15 * 32 + 10] * odd10 + DctCoef32x32[15 * 32 + 11] * odd11 +
			DctCoef32x32[15 * 32 + 12] * odd12 + DctCoef32x32[15 * 32 + 13] * odd13 + DctCoef32x32[15 * 32 + 14] * odd14 + DctCoef32x32[15 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[17 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[17 * 32 + 0] * odd0 + DctCoef32x32[17 * 32 + 1] * odd1 + DctCoef32x32[17 * 32 + 2] * odd2 + DctCoef32x32[17 * 32 + 3] * odd3 +
			DctCoef32x32[17 * 32 + 4] * odd4 + DctCoef32x32[17 * 32 + 5] * odd5 + DctCoef32x32[17 * 32 + 6] * odd6 + DctCoef32x32[17 * 32 + 7] * odd7 +
			DctCoef32x32[17 * 32 + 8] * odd8 + DctCoef32x32[17 * 32 + 9] * odd9 + DctCoef32x32[17 * 32 + 10] * odd10 + DctCoef32x32[17 * 32 + 11] * odd11 +
			DctCoef32x32[17 * 32 + 12] * odd12 + DctCoef32x32[17 * 32 + 13] * odd13 + DctCoef32x32[17 * 32 + 14] * odd14 + DctCoef32x32[17 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[19 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[19 * 32 + 0] * odd0 + DctCoef32x32[19 * 32 + 1] * odd1 + DctCoef32x32[19 * 32 + 2] * odd2 + DctCoef32x32[19 * 32 + 3] * odd3 +
			DctCoef32x32[19 * 32 + 4] * odd4 + DctCoef32x32[19 * 32 + 5] * odd5 + DctCoef32x32[19 * 32 + 6] * odd6 + DctCoef32x32[19 * 32 + 7] * odd7 +
			DctCoef32x32[19 * 32 + 8] * odd8 + DctCoef32x32[19 * 32 + 9] * odd9 + DctCoef32x32[19 * 32 + 10] * odd10 + DctCoef32x32[19 * 32 + 11] * odd11 +
			DctCoef32x32[19 * 32 + 12] * odd12 + DctCoef32x32[19 * 32 + 13] * odd13 + DctCoef32x32[19 * 32 + 14] * odd14 + DctCoef32x32[19 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[21 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[21 * 32 + 0] * odd0 + DctCoef32x32[21 * 32 + 1] * odd1 + DctCoef32x32[21 * 32 + 2] * odd2 + DctCoef32x32[21 * 32 + 3] * odd3 +
			DctCoef32x32[21 * 32 + 4] * odd4 + DctCoef32x32[21 * 32 + 5] * odd5 + DctCoef32x32[21 * 32 + 6] * odd6 + DctCoef32x32[21 * 32 + 7] * odd7 +
			DctCoef32x32[21 * 32 + 8] * odd8 + DctCoef32x32[21 * 32 + 9] * odd9 + DctCoef32x32[21 * 32 + 10] * odd10 + DctCoef32x32[21 * 32 + 11] * odd11 +
			DctCoef32x32[21 * 32 + 12] * odd12 + DctCoef32x32[21 * 32 + 13] * odd13 + DctCoef32x32[21 * 32 + 14] * odd14 + DctCoef32x32[21 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[23 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[23 * 32 + 0] * odd0 + DctCoef32x32[23 * 32 + 1] * odd1 + DctCoef32x32[23 * 32 + 2] * odd2 + DctCoef32x32[23 * 32 + 3] * odd3 +
			DctCoef32x32[23 * 32 + 4] * odd4 + DctCoef32x32[23 * 32 + 5] * odd5 + DctCoef32x32[23 * 32 + 6] * odd6 + DctCoef32x32[23 * 32 + 7] * odd7 +
			DctCoef32x32[23 * 32 + 8] * odd8 + DctCoef32x32[23 * 32 + 9] * odd9 + DctCoef32x32[23 * 32 + 10] * odd10 + DctCoef32x32[23 * 32 + 11] * odd11 +
			DctCoef32x32[23 * 32 + 12] * odd12 + DctCoef32x32[23 * 32 + 13] * odd13 + DctCoef32x32[23 * 32 + 14] * odd14 + DctCoef32x32[23 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[25 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[25 * 32 + 0] * odd0 + DctCoef32x32[25 * 32 + 1] * odd1 + DctCoef32x32[25 * 32 + 2] * odd2 + DctCoef32x32[25 * 32 + 3] * odd3 +
			DctCoef32x32[25 * 32 + 4] * odd4 + DctCoef32x32[25 * 32 + 5] * odd5 + DctCoef32x32[25 * 32 + 6] * odd6 + DctCoef32x32[25 * 32 + 7] * odd7 +
			DctCoef32x32[25 * 32 + 8] * odd8 + DctCoef32x32[25 * 32 + 9] * odd9 + DctCoef32x32[25 * 32 + 10] * odd10 + DctCoef32x32[25 * 32 + 11] * odd11 +
			DctCoef32x32[25 * 32 + 12] * odd12 + DctCoef32x32[25 * 32 + 13] * odd13 + DctCoef32x32[25 * 32 + 14] * odd14 + DctCoef32x32[25 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[27 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[27 * 32 + 0] * odd0 + DctCoef32x32[27 * 32 + 1] * odd1 + DctCoef32x32[27 * 32 + 2] * odd2 + DctCoef32x32[27 * 32 + 3] * odd3 +
			DctCoef32x32[27 * 32 + 4] * odd4 + DctCoef32x32[27 * 32 + 5] * odd5 + DctCoef32x32[27 * 32 + 6] * odd6 + DctCoef32x32[27 * 32 + 7] * odd7 +
			DctCoef32x32[27 * 32 + 8] * odd8 + DctCoef32x32[27 * 32 + 9] * odd9 + DctCoef32x32[27 * 32 + 10] * odd10 + DctCoef32x32[27 * 32 + 11] * odd11 +
			DctCoef32x32[27 * 32 + 12] * odd12 + DctCoef32x32[27 * 32 + 13] * odd13 + DctCoef32x32[27 * 32 + 14] * odd14 + DctCoef32x32[27 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[29 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[29 * 32 + 0] * odd0 + DctCoef32x32[29 * 32 + 1] * odd1 + DctCoef32x32[29 * 32 + 2] * odd2 + DctCoef32x32[29 * 32 + 3] * odd3 +
			DctCoef32x32[29 * 32 + 4] * odd4 + DctCoef32x32[29 * 32 + 5] * odd5 + DctCoef32x32[29 * 32 + 6] * odd6 + DctCoef32x32[29 * 32 + 7] * odd7 +
			DctCoef32x32[29 * 32 + 8] * odd8 + DctCoef32x32[29 * 32 + 9] * odd9 + DctCoef32x32[29 * 32 + 10] * odd10 + DctCoef32x32[29 * 32 + 11] * odd11 +
			DctCoef32x32[29 * 32 + 12] * odd12 + DctCoef32x32[29 * 32 + 13] * odd13 + DctCoef32x32[29 * 32 + 14] * odd14 + DctCoef32x32[29 * 32 + 15] * odd15 +
			offset) >> shift);
		transformCoefficients[31 * dstStride + rowIndex] = (EB_S16)((DctCoef32x32[31 * 32 + 0] * odd0 + DctCoef32x32[31 * 32 + 1] * odd1 + DctCoef32x32[31 * 32 + 2] * odd2 + DctCoef32x32[31 * 32 + 3] * odd3 +
			DctCoef32x32[31 * 32 + 4] * odd4 + DctCoef32x32[31 * 32 + 5] * odd5 + DctCoef32x32[31 * 32 + 6] * odd6 + DctCoef32x32[31 * 32 + 7] * odd7 +
			DctCoef32x32[31 * 32 + 8] * odd8 + DctCoef32x32[31 * 32 + 9] * odd9 + DctCoef32x32[31 * 32 + 10] * odd10 + DctCoef32x32[31 * 32 + 11] * odd11 +
			DctCoef32x32[31 * 32 + 12] * odd12 + DctCoef32x32[31 * 32 + 13] * odd13 + DctCoef32x32[31 * 32 + 14] * odd14 + DctCoef32x32[31 * 32 + 15] * odd15 +
			offset) >> shift);
	}
}

/*********************************************************************
* PartialButterfly16
*   1D 16x16 forward transform using partial butterfly
*
*  residual
*   is the input pointer for pixels
*
*  transformCoefficients
*   is the output after applying the butterfly transform (the output is transposed)
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transform pointers
*
*  shift
*   is the input and shows the number of shifts to right
*********************************************************************/
inline static void PartialButterfly16(
    EB_S16       *residual,
    EB_S16       *transformCoefficients,
    const EB_U32  srcStride,
    const EB_U32  dstStride,
    const EB_U32  shift)
{
    EB_S32 even0, even1, even2, even3, even4, even5, even6, even7;
    EB_S32 odd0, odd1, odd2, odd3, odd4, odd5, odd6, odd7;
    EB_S32 evenEven0, evenEven1, evenEven2, evenEven3;
    EB_S32 evenOdd0, evenOdd1, evenOdd2, evenOdd3;
    EB_S32 evenEvenEven0, evenEvenEven1;
    EB_S32 evenEvenOdd0, evenEvenOdd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 16; rowIndex++) {
        //Calculating even and odd variables
        rowStrideIndex = rowIndex*srcStride;
        even0 = residual[rowStrideIndex + 0] + residual[rowStrideIndex + 15];
        even1 = residual[rowStrideIndex + 1] + residual[rowStrideIndex + 14];
        even2 = residual[rowStrideIndex + 2] + residual[rowStrideIndex + 13];
        even3 = residual[rowStrideIndex + 3] + residual[rowStrideIndex + 12];
        even4 = residual[rowStrideIndex + 4] + residual[rowStrideIndex + 11];
        even5 = residual[rowStrideIndex + 5] + residual[rowStrideIndex + 10];
        even6 = residual[rowStrideIndex + 6] + residual[rowStrideIndex + 9];
        even7 = residual[rowStrideIndex + 7] + residual[rowStrideIndex + 8];

        odd0 = residual[rowStrideIndex + 0] - residual[rowStrideIndex + 15];
        odd1 = residual[rowStrideIndex + 1] - residual[rowStrideIndex + 14];
        odd2 = residual[rowStrideIndex + 2] - residual[rowStrideIndex + 13];
        odd3 = residual[rowStrideIndex + 3] - residual[rowStrideIndex + 12];
        odd4 = residual[rowStrideIndex + 4] - residual[rowStrideIndex + 11];
        odd5 = residual[rowStrideIndex + 5] - residual[rowStrideIndex + 10];
        odd6 = residual[rowStrideIndex + 6] - residual[rowStrideIndex + 9];
        odd7 = residual[rowStrideIndex + 7] - residual[rowStrideIndex + 8];

        //Calculating evenEven and evenOdd variables
        evenEven0 = even0 + even7;
        evenEven1 = even1 + even6;
        evenEven2 = even2 + even5;
        evenEven3 = even3 + even4;

        evenOdd0 = even0 - even7;
        evenOdd1 = even1 - even6;
        evenOdd2 = even2 - even5;
        evenOdd3 = even3 - even4;

        //Calculating evenEvenEven and evenEvenOdd variables
        evenEvenEven0 = evenEven0 + evenEven3;
        evenEvenEven1 = evenEven1 + evenEven2;

        evenEvenOdd0 = evenEven0 - evenEven3;
        evenEvenOdd1 = evenEven1 - evenEven2;

        transformCoefficients[0 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[0 * 16 + 0] * evenEvenEven0 + DctCoef16x16[0 * 16 + 1] * evenEvenEven1 + offset) >> shift);
        transformCoefficients[4 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[4 * 16 + 0] * evenEvenOdd0 + DctCoef16x16[4 * 16 + 1] * evenEvenOdd1 + offset) >> shift);
        transformCoefficients[8 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[8 * 16 + 0] * evenEvenEven0 + DctCoef16x16[8 * 16 + 1] * evenEvenEven1 + offset) >> shift);
        transformCoefficients[12 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[12 * 16 + 0] * evenEvenOdd0 + DctCoef16x16[12 * 16 + 1] * evenEvenOdd1 + offset) >> shift);

        transformCoefficients[2 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[2 * 16 + 0] * evenOdd0 + DctCoef16x16[2 * 16 + 1] * evenOdd1 +
            DctCoef16x16[2 * 16 + 2] * evenOdd2 + DctCoef16x16[2 * 16 + 3] * evenOdd3 + offset) >> shift);
        transformCoefficients[6 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[6 * 16 + 0] * evenOdd0 + DctCoef16x16[6 * 16 + 1] * evenOdd1 +
            DctCoef16x16[6 * 16 + 2] * evenOdd2 + DctCoef16x16[6 * 16 + 3] * evenOdd3 + offset) >> shift);
        transformCoefficients[10 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[10 * 16 + 0] * evenOdd0 + DctCoef16x16[10 * 16 + 1] * evenOdd1 +
            DctCoef16x16[10 * 16 + 2] * evenOdd2 + DctCoef16x16[10 * 16 + 3] * evenOdd3 + offset) >> shift);
        transformCoefficients[14 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[14 * 16 + 0] * evenOdd0 + DctCoef16x16[14 * 16 + 1] * evenOdd1 +
            DctCoef16x16[14 * 16 + 2] * evenOdd2 + DctCoef16x16[14 * 16 + 3] * evenOdd3 + offset) >> shift);

        transformCoefficients[1 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[1 * 16 + 0] * odd0 + DctCoef16x16[1 * 16 + 1] * odd1 + DctCoef16x16[1 * 16 + 2] * odd2 + DctCoef16x16[1 * 16 + 3] * odd3 +
            DctCoef16x16[1 * 16 + 4] * odd4 + DctCoef16x16[1 * 16 + 5] * odd5 + DctCoef16x16[1 * 16 + 6] * odd6 + DctCoef16x16[1 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[3 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[3 * 16 + 0] * odd0 + DctCoef16x16[3 * 16 + 1] * odd1 + DctCoef16x16[3 * 16 + 2] * odd2 + DctCoef16x16[3 * 16 + 3] * odd3 +
            DctCoef16x16[3 * 16 + 4] * odd4 + DctCoef16x16[3 * 16 + 5] * odd5 + DctCoef16x16[3 * 16 + 6] * odd6 + DctCoef16x16[3 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[5 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[5 * 16 + 0] * odd0 + DctCoef16x16[5 * 16 + 1] * odd1 + DctCoef16x16[5 * 16 + 2] * odd2 + DctCoef16x16[5 * 16 + 3] * odd3 +
            DctCoef16x16[5 * 16 + 4] * odd4 + DctCoef16x16[5 * 16 + 5] * odd5 + DctCoef16x16[5 * 16 + 6] * odd6 + DctCoef16x16[5 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[7 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[7 * 16 + 0] * odd0 + DctCoef16x16[7 * 16 + 1] * odd1 + DctCoef16x16[7 * 16 + 2] * odd2 + DctCoef16x16[7 * 16 + 3] * odd3 +
            DctCoef16x16[7 * 16 + 4] * odd4 + DctCoef16x16[7 * 16 + 5] * odd5 + DctCoef16x16[7 * 16 + 6] * odd6 + DctCoef16x16[7 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[9 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[9 * 16 + 0] * odd0 + DctCoef16x16[9 * 16 + 1] * odd1 + DctCoef16x16[9 * 16 + 2] * odd2 + DctCoef16x16[9 * 16 + 3] * odd3 +
            DctCoef16x16[9 * 16 + 4] * odd4 + DctCoef16x16[9 * 16 + 5] * odd5 + DctCoef16x16[9 * 16 + 6] * odd6 + DctCoef16x16[9 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[11 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[11 * 16 + 0] * odd0 + DctCoef16x16[11 * 16 + 1] * odd1 + DctCoef16x16[11 * 16 + 2] * odd2 + DctCoef16x16[11 * 16 + 3] * odd3 +
            DctCoef16x16[11 * 16 + 4] * odd4 + DctCoef16x16[11 * 16 + 5] * odd5 + DctCoef16x16[11 * 16 + 6] * odd6 + DctCoef16x16[11 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[13 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[13 * 16 + 0] * odd0 + DctCoef16x16[13 * 16 + 1] * odd1 + DctCoef16x16[13 * 16 + 2] * odd2 + DctCoef16x16[13 * 16 + 3] * odd3 +
            DctCoef16x16[13 * 16 + 4] * odd4 + DctCoef16x16[13 * 16 + 5] * odd5 + DctCoef16x16[13 * 16 + 6] * odd6 + DctCoef16x16[13 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[15 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[15 * 16 + 0] * odd0 + DctCoef16x16[15 * 16 + 1] * odd1 + DctCoef16x16[15 * 16 + 2] * odd2 + DctCoef16x16[15 * 16 + 3] * odd3 +
            DctCoef16x16[15 * 16 + 4] * odd4 + DctCoef16x16[15 * 16 + 5] * odd5 + DctCoef16x16[15 * 16 + 6] * odd6 + DctCoef16x16[15 * 16 + 7] * odd7 +
            offset) >> shift);
    }
}

inline static void PartialButterfly16Estimate(
    EB_S16       *residual,
    EB_S16       *transformCoefficients,
    const EB_U32  srcStride,
    const EB_U32  dstStride,
    const EB_U32  shift)
{

    EB_S16 even0, even1, even2, even3, even4, even5, even6, even7;
    EB_S16 odd0, odd1, odd2, odd3, odd4, odd5, odd6, odd7;
    EB_S32 evenEven0, evenEven1, evenEven2, evenEven3;
    EB_S32 evenOdd0, evenOdd1, evenOdd2, evenOdd3;
    EB_S32 evenEvenEven0, evenEvenEven1;
    EB_S32 evenEvenOdd0, evenEvenOdd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 16; rowIndex++) {
        //Calculating even and odd variables
        rowStrideIndex = rowIndex*srcStride;
        even0 = residual[rowStrideIndex + 0] + residual[rowStrideIndex + 15];
        even1 = residual[rowStrideIndex + 1] + residual[rowStrideIndex + 14];
        even2 = residual[rowStrideIndex + 2] + residual[rowStrideIndex + 13];
        even3 = residual[rowStrideIndex + 3] + residual[rowStrideIndex + 12];
        even4 = residual[rowStrideIndex + 4] + residual[rowStrideIndex + 11];
        even5 = residual[rowStrideIndex + 5] + residual[rowStrideIndex + 10];
        even6 = residual[rowStrideIndex + 6] + residual[rowStrideIndex + 9];
        even7 = residual[rowStrideIndex + 7] + residual[rowStrideIndex + 8];

        odd0 = residual[rowStrideIndex + 0] - residual[rowStrideIndex + 15];
        odd1 = residual[rowStrideIndex + 1] - residual[rowStrideIndex + 14];
        odd2 = residual[rowStrideIndex + 2] - residual[rowStrideIndex + 13];
        odd3 = residual[rowStrideIndex + 3] - residual[rowStrideIndex + 12];
        odd4 = residual[rowStrideIndex + 4] - residual[rowStrideIndex + 11];
        odd5 = residual[rowStrideIndex + 5] - residual[rowStrideIndex + 10];
        odd6 = residual[rowStrideIndex + 6] - residual[rowStrideIndex + 9];
        odd7 = residual[rowStrideIndex + 7] - residual[rowStrideIndex + 8];

        //Calculating evenEven and evenOdd variables
        evenEven0 = even0 + even7;
        evenEven1 = even1 + even6;
        evenEven2 = even2 + even5;
        evenEven3 = even3 + even4;

        evenOdd0 = even0 - even7;
        evenOdd1 = even1 - even6;
        evenOdd2 = even2 - even5;
        evenOdd3 = even3 - even4;

        //Calculating evenEvenEven and evenEvenOdd variables
        evenEvenEven0 = evenEven0 + evenEven3;
        evenEvenEven1 = evenEven1 + evenEven2;

        evenEvenOdd0 = evenEven0 - evenEven3;
        evenEvenOdd1 = evenEven1 - evenEven2;

        transformCoefficients[0 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[0 * 16 + 0] * evenEvenEven0 + DctCoef16x16[0 * 16 + 1] * evenEvenEven1 + offset) >> shift);
        transformCoefficients[4 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[4 * 16 + 0] * evenEvenOdd0 + DctCoef16x16[4 * 16 + 1] * evenEvenOdd1 + offset) >> shift);
        transformCoefficients[8 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[8 * 16 + 0] * evenEvenEven0 + DctCoef16x16[8 * 16 + 1] * evenEvenEven1 + offset) >> shift);
        transformCoefficients[12 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[12 * 16 + 0] * evenEvenOdd0 + DctCoef16x16[12 * 16 + 1] * evenEvenOdd1 + offset) >> shift);

        transformCoefficients[2 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[2 * 16 + 0] * evenOdd0 + DctCoef16x16[2 * 16 + 1] * evenOdd1 +
            DctCoef16x16[2 * 16 + 2] * evenOdd2 + DctCoef16x16[2 * 16 + 3] * evenOdd3 + offset) >> shift);
        transformCoefficients[6 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[6 * 16 + 0] * evenOdd0 + DctCoef16x16[6 * 16 + 1] * evenOdd1 +
            DctCoef16x16[6 * 16 + 2] * evenOdd2 + DctCoef16x16[6 * 16 + 3] * evenOdd3 + offset) >> shift);
        transformCoefficients[10 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[10 * 16 + 0] * evenOdd0 + DctCoef16x16[10 * 16 + 1] * evenOdd1 +
            DctCoef16x16[10 * 16 + 2] * evenOdd2 + DctCoef16x16[10 * 16 + 3] * evenOdd3 + offset) >> shift);
        transformCoefficients[14 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[14 * 16 + 0] * evenOdd0 + DctCoef16x16[14 * 16 + 1] * evenOdd1 +
            DctCoef16x16[14 * 16 + 2] * evenOdd2 + DctCoef16x16[14 * 16 + 3] * evenOdd3 + offset) >> shift);

        transformCoefficients[1 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[1 * 16 + 0] * odd0 + DctCoef16x16[1 * 16 + 1] * odd1 + DctCoef16x16[1 * 16 + 2] * odd2 + DctCoef16x16[1 * 16 + 3] * odd3 +
            DctCoef16x16[1 * 16 + 4] * odd4 + DctCoef16x16[1 * 16 + 5] * odd5 + DctCoef16x16[1 * 16 + 6] * odd6 + DctCoef16x16[1 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[3 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[3 * 16 + 0] * odd0 + DctCoef16x16[3 * 16 + 1] * odd1 + DctCoef16x16[3 * 16 + 2] * odd2 + DctCoef16x16[3 * 16 + 3] * odd3 +
            DctCoef16x16[3 * 16 + 4] * odd4 + DctCoef16x16[3 * 16 + 5] * odd5 + DctCoef16x16[3 * 16 + 6] * odd6 + DctCoef16x16[3 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[5 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[5 * 16 + 0] * odd0 + DctCoef16x16[5 * 16 + 1] * odd1 + DctCoef16x16[5 * 16 + 2] * odd2 + DctCoef16x16[5 * 16 + 3] * odd3 +
            DctCoef16x16[5 * 16 + 4] * odd4 + DctCoef16x16[5 * 16 + 5] * odd5 + DctCoef16x16[5 * 16 + 6] * odd6 + DctCoef16x16[5 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[7 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[7 * 16 + 0] * odd0 + DctCoef16x16[7 * 16 + 1] * odd1 + DctCoef16x16[7 * 16 + 2] * odd2 + DctCoef16x16[7 * 16 + 3] * odd3 +
            DctCoef16x16[7 * 16 + 4] * odd4 + DctCoef16x16[7 * 16 + 5] * odd5 + DctCoef16x16[7 * 16 + 6] * odd6 + DctCoef16x16[7 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[9 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[9 * 16 + 0] * odd0 + DctCoef16x16[9 * 16 + 1] * odd1 + DctCoef16x16[9 * 16 + 2] * odd2 + DctCoef16x16[9 * 16 + 3] * odd3 +
            DctCoef16x16[9 * 16 + 4] * odd4 + DctCoef16x16[9 * 16 + 5] * odd5 + DctCoef16x16[9 * 16 + 6] * odd6 + DctCoef16x16[9 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[11 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[11 * 16 + 0] * odd0 + DctCoef16x16[11 * 16 + 1] * odd1 + DctCoef16x16[11 * 16 + 2] * odd2 + DctCoef16x16[11 * 16 + 3] * odd3 +
            DctCoef16x16[11 * 16 + 4] * odd4 + DctCoef16x16[11 * 16 + 5] * odd5 + DctCoef16x16[11 * 16 + 6] * odd6 + DctCoef16x16[11 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[13 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[13 * 16 + 0] * odd0 + DctCoef16x16[13 * 16 + 1] * odd1 + DctCoef16x16[13 * 16 + 2] * odd2 + DctCoef16x16[13 * 16 + 3] * odd3 +
            DctCoef16x16[13 * 16 + 4] * odd4 + DctCoef16x16[13 * 16 + 5] * odd5 + DctCoef16x16[13 * 16 + 6] * odd6 + DctCoef16x16[13 * 16 + 7] * odd7 +
            offset) >> shift);
        transformCoefficients[15 * dstStride + rowIndex] = (EB_S16)((DctCoef16x16[15 * 16 + 0] * odd0 + DctCoef16x16[15 * 16 + 1] * odd1 + DctCoef16x16[15 * 16 + 2] * odd2 + DctCoef16x16[15 * 16 + 3] * odd3 +
            DctCoef16x16[15 * 16 + 4] * odd4 + DctCoef16x16[15 * 16 + 5] * odd5 + DctCoef16x16[15 * 16 + 6] * odd6 + DctCoef16x16[15 * 16 + 7] * odd7 +
            offset) >> shift);
    }
}

/*********************************************************************
* PartialButterfly8
*   1D 8x8 forward transform using partial butterfly
*
*  residual
*   is the input pointer for pixels
*
*  transformCoefficients
*   is the output after applying the butterfly transform (the output is transposed)
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transform pointers
*
*  shift
*   is the input and shows the number of shifts to right
*********************************************************************/
inline static void PartialButterfly8(
    EB_S16       *residual,
    EB_S16       *transformCoefficients,
    const EB_U32  srcStride,
    const EB_U32  dstStride,
    const EB_U32  shift)
{
    EB_S32 even0, even1, even2, even3;
    EB_S32 odd0, odd1, odd2, odd3;
    EB_S32 evenEven0, evenEven1;
    EB_S32 evenOdd0, evenOdd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 8; rowIndex++) {
        rowStrideIndex = rowIndex*srcStride;

        //Calculating even and odd variables
        even0 = residual[rowStrideIndex + 0] + residual[rowStrideIndex + 7];
        even1 = residual[rowStrideIndex + 1] + residual[rowStrideIndex + 6];
        even2 = residual[rowStrideIndex + 2] + residual[rowStrideIndex + 5];
        even3 = residual[rowStrideIndex + 3] + residual[rowStrideIndex + 4];

        odd0 = residual[rowStrideIndex + 0] - residual[rowStrideIndex + 7];
        odd1 = residual[rowStrideIndex + 1] - residual[rowStrideIndex + 6];
        odd2 = residual[rowStrideIndex + 2] - residual[rowStrideIndex + 5];
        odd3 = residual[rowStrideIndex + 3] - residual[rowStrideIndex + 4];

        //Calculating evenEven and evenOdd variables
        evenEven0 = even0 + even3;
        evenEven1 = even1 + even2;
        evenOdd0 = even0 - even3;
        evenOdd1 = even1 - even2;

        transformCoefficients[0 * dstStride + rowIndex] = (EB_S16)((DctCoef8x8[0 * 8 + 0] * evenEven0 + DctCoef8x8[0 * 8 + 1] * evenEven1 + offset) >> shift);
        transformCoefficients[2 * dstStride + rowIndex] = (EB_S16)((DctCoef8x8[2 * 8 + 0] * evenOdd0 + DctCoef8x8[2 * 8 + 1] * evenOdd1 + offset) >> shift);
        transformCoefficients[4 * dstStride + rowIndex] = (EB_S16)((DctCoef8x8[4 * 8 + 0] * evenEven0 + DctCoef8x8[4 * 8 + 1] * evenEven1 + offset) >> shift);
        transformCoefficients[6 * dstStride + rowIndex] = (EB_S16)((DctCoef8x8[6 * 8 + 0] * evenOdd0 + DctCoef8x8[6 * 8 + 1] * evenOdd1 + offset) >> shift);

        transformCoefficients[1 * dstStride + rowIndex] = (EB_S16)((DctCoef8x8[1 * 8 + 0] * odd0 + DctCoef8x8[1 * 8 + 1] * odd1 + DctCoef8x8[1 * 8 + 2] * odd2 + DctCoef8x8[1 * 8 + 3] * odd3 + offset) >> shift);
        transformCoefficients[3 * dstStride + rowIndex] = (EB_S16)((DctCoef8x8[3 * 8 + 0] * odd0 + DctCoef8x8[3 * 8 + 1] * odd1 + DctCoef8x8[3 * 8 + 2] * odd2 + DctCoef8x8[3 * 8 + 3] * odd3 + offset) >> shift);
        transformCoefficients[5 * dstStride + rowIndex] = (EB_S16)((DctCoef8x8[5 * 8 + 0] * odd0 + DctCoef8x8[5 * 8 + 1] * odd1 + DctCoef8x8[5 * 8 + 2] * odd2 + DctCoef8x8[5 * 8 + 3] * odd3 + offset) >> shift);
        transformCoefficients[7 * dstStride + rowIndex] = (EB_S16)((DctCoef8x8[7 * 8 + 0] * odd0 + DctCoef8x8[7 * 8 + 1] * odd1 + DctCoef8x8[7 * 8 + 2] * odd2 + DctCoef8x8[7 * 8 + 3] * odd3 + offset) >> shift);
    }
}

/*********************************************************************
* PartialButterfly4
*   1D 4x4 forward transform using partial butterfly
*
*  residual
*   is the input pointer for pixels
*
*  transformCoefficients
*   is the output after applying the butterfly transform (the output is transposed)
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transform pointers
*
*  shift
*   is the input and shows the number of shifts to right
*********************************************************************/
//TODO define a function pointer for choosing between DCT and DST and in higher level select one based on the mode -AN
inline static void PartialButterfly4(
    EB_S16        *residual,
    EB_S16        *transformCoefficients,
    const EB_U32   srcStride,
    const EB_U32   dstStride,
    const EB_U32   shift)
{
    EB_S32 even0, even1, odd0, odd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 4; rowIndex++) {
        /* even and odd */
        rowStrideIndex = rowIndex*srcStride;
        even0 = residual[rowStrideIndex + 0] + residual[rowStrideIndex + 3];
        odd0 = residual[rowStrideIndex + 0] - residual[rowStrideIndex + 3];
        even1 = residual[rowStrideIndex + 1] + residual[rowStrideIndex + 2];
        odd1 = residual[rowStrideIndex + 1] - residual[rowStrideIndex + 2];

        transformCoefficients[0 * dstStride + rowIndex] = (EB_S16)((DctCoef4x4[0 * 4 + 0] * even0 + DctCoef4x4[0 * 4 + 1] * even1 + offset) >> shift);
        transformCoefficients[2 * dstStride + rowIndex] = (EB_S16)((DctCoef4x4[2 * 4 + 0] * even0 + DctCoef4x4[2 * 4 + 1] * even1 + offset) >> shift);
        transformCoefficients[1 * dstStride + rowIndex] = (EB_S16)((DctCoef4x4[1 * 4 + 0] * odd0 + DctCoef4x4[1 * 4 + 1] * odd1 + offset) >> shift);
        transformCoefficients[3 * dstStride + rowIndex] = (EB_S16)((DctCoef4x4[3 * 4 + 0] * odd0 + DctCoef4x4[3 * 4 + 1] * odd1 + offset) >> shift);
    }
}

/*********************************************************************
* Dst4
*********************************************************************/

inline static void Dst4(
    EB_S16        *residual,
    EB_S16        *transformCoefficients,
    const EB_U32   srcStride,
    const EB_U32   dstStride,
    const EB_U32   shift)
{
    EB_S32 even0, even1, odd0, odd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 4; rowIndex++) {
        /* even and odd */
        rowStrideIndex = rowIndex*srcStride;
        even0 = residual[rowStrideIndex + 0] + residual[rowStrideIndex + 3];
        odd0 = residual[rowStrideIndex + 1] + residual[rowStrideIndex + 3];
        even1 = residual[rowStrideIndex + 0] - residual[rowStrideIndex + 1];
        odd1 = 74 * residual[rowStrideIndex + 2];

        transformCoefficients[0 * dstStride + rowIndex] = (EB_S16)((29 * even0 + 55 * odd0 + odd1 + offset) >> shift);
        transformCoefficients[2 * dstStride + rowIndex] = (EB_S16)((29 * even1 + 55 * even0 - odd1 + offset) >> shift);
        transformCoefficients[1 * dstStride + rowIndex] = (EB_S16)(((74 * (residual[rowStrideIndex + 0] + residual[rowStrideIndex + 1] - residual[rowStrideIndex + 3])) + offset) >> shift);
        transformCoefficients[3 * dstStride + rowIndex] = (EB_S16)((55 * even1 - 29 * odd0 + odd1 + offset) >> shift);
    }
}

/*********************************************************************
* PartialButterflyInverse32
*   1D 32x32 inverse transform using partial butterfly
*
*  transformCoefficients
*   is the input pointer for transformed pixels
*
*  residual
*   is the output after applying the inverse butterfly transform. (the output is transposed)
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*********************************************************************/
inline static void PartialButterflyInverse32(
    EB_S16        *transformCoefficients,
    EB_S16        *residual,
    const EB_U32   srcStride,
    const EB_U32   dstStride,
    const EB_U32   shift)
{
    EB_S32 even0, even1, even2, even3, even4, even5, even6, even7, even8, even9, even10, even11, even12, even13, even14, even15;
    EB_S32 odd0, odd1, odd2, odd3, odd4, odd5, odd6, odd7, odd8, odd9, odd10, odd11, odd12, odd13, odd14, odd15;
    EB_S32 evenEven0, evenEven1, evenEven2, evenEven3, evenEven4, evenEven5, evenEven6, evenEven7;
    EB_S32 evenOdd0, evenOdd1, evenOdd2, evenOdd3, evenOdd4, evenOdd5, evenOdd6, evenOdd7;
    EB_S32 evenEvenEven0, evenEvenEven1, evenEvenEven2, evenEvenEven3;
    EB_S32 evenEvenOdd0, evenEvenOdd1, evenEvenOdd2, evenEvenOdd3;
    EB_S32 evenEvenEvenEven0, evenEvenEvenEven1;
    EB_S32 evenEvenEvenOdd0, evenEvenEvenOdd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 32; rowIndex++) {
        rowStrideIndex = rowIndex*dstStride;
        odd0 = DctCoef32x32[1 * 32 + 0] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 0] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 0] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 0] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 0] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 0] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 0] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 0] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 0] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 0] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 0] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 0] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 0] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 0] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 0] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 0] * transformCoefficients[31 * srcStride + rowIndex];
        odd1 = DctCoef32x32[1 * 32 + 1] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 1] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 1] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 1] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 1] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 1] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 1] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 1] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 1] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 1] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 1] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 1] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 1] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 1] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 1] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 1] * transformCoefficients[31 * srcStride + rowIndex];
        odd2 = DctCoef32x32[1 * 32 + 2] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 2] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 2] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 2] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 2] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 2] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 2] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 2] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 2] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 2] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 2] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 2] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 2] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 2] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 2] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 2] * transformCoefficients[31 * srcStride + rowIndex];
        odd3 = DctCoef32x32[1 * 32 + 3] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 3] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 3] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 3] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 3] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 3] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 3] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 3] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 3] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 3] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 3] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 3] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 3] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 3] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 3] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 3] * transformCoefficients[31 * srcStride + rowIndex];
        odd4 = DctCoef32x32[1 * 32 + 4] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 4] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 4] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 4] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 4] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 4] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 4] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 4] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 4] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 4] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 4] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 4] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 4] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 4] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 4] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 4] * transformCoefficients[31 * srcStride + rowIndex];
        odd5 = DctCoef32x32[1 * 32 + 5] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 5] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 5] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 5] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 5] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 5] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 5] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 5] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 5] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 5] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 5] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 5] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 5] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 5] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 5] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 5] * transformCoefficients[31 * srcStride + rowIndex];
        odd6 = DctCoef32x32[1 * 32 + 6] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 6] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 6] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 6] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 6] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 6] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 6] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 6] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 6] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 6] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 6] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 6] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 6] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 6] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 6] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 6] * transformCoefficients[31 * srcStride + rowIndex];
        odd7 = DctCoef32x32[1 * 32 + 7] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 7] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 7] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 7] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 7] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 7] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 7] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 7] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 7] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 7] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 7] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 7] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 7] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 7] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 7] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 7] * transformCoefficients[31 * srcStride + rowIndex];
        odd8 = DctCoef32x32[1 * 32 + 8] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 8] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 8] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 8] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 8] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 8] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 8] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 8] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 8] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 8] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 8] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 8] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 8] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 8] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 8] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 8] * transformCoefficients[31 * srcStride + rowIndex];
        odd9 = DctCoef32x32[1 * 32 + 9] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 9] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 9] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 9] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 9] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 9] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 9] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 9] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 9] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 9] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 9] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 9] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 9] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 9] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 9] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 9] * transformCoefficients[31 * srcStride + rowIndex];
        odd10 = DctCoef32x32[1 * 32 + 10] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 10] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 10] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 10] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 10] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 10] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 10] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 10] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 10] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 10] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 10] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 10] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 10] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 10] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 10] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 10] * transformCoefficients[31 * srcStride + rowIndex];
        odd11 = DctCoef32x32[1 * 32 + 11] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 11] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 11] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 11] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 11] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 11] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 11] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 11] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 11] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 11] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 11] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 11] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 11] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 11] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 11] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 11] * transformCoefficients[31 * srcStride + rowIndex];
        odd12 = DctCoef32x32[1 * 32 + 12] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 12] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 12] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 12] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 12] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 12] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 12] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 12] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 12] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 12] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 12] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 12] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 12] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 12] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 12] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 12] * transformCoefficients[31 * srcStride + rowIndex];
        odd13 = DctCoef32x32[1 * 32 + 13] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 13] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 13] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 13] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 13] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 13] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 13] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 13] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 13] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 13] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 13] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 13] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 13] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 13] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 13] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 13] * transformCoefficients[31 * srcStride + rowIndex];
        odd14 = DctCoef32x32[1 * 32 + 14] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 14] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 14] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 14] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 14] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 14] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 14] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 14] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 14] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 14] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 14] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 14] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 14] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 14] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 14] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 14] * transformCoefficients[31 * srcStride + rowIndex];
        odd15 = DctCoef32x32[1 * 32 + 15] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef32x32[3 * 32 + 15] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef32x32[5 * 32 + 15] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef32x32[7 * 32 + 15] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef32x32[9 * 32 + 15] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef32x32[11 * 32 + 15] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef32x32[13 * 32 + 15] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef32x32[15 * 32 + 15] * transformCoefficients[15 * srcStride + rowIndex] +
            DctCoef32x32[17 * 32 + 15] * transformCoefficients[17 * srcStride + rowIndex] + DctCoef32x32[19 * 32 + 15] * transformCoefficients[19 * srcStride + rowIndex] +
            DctCoef32x32[21 * 32 + 15] * transformCoefficients[21 * srcStride + rowIndex] + DctCoef32x32[23 * 32 + 15] * transformCoefficients[23 * srcStride + rowIndex] +
            DctCoef32x32[25 * 32 + 15] * transformCoefficients[25 * srcStride + rowIndex] + DctCoef32x32[27 * 32 + 15] * transformCoefficients[27 * srcStride + rowIndex] +
            DctCoef32x32[29 * 32 + 15] * transformCoefficients[29 * srcStride + rowIndex] + DctCoef32x32[31 * 32 + 15] * transformCoefficients[31 * srcStride + rowIndex];

        evenOdd0 = DctCoef32x32[2 * 32 + 0] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef32x32[6 * 32 + 0] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef32x32[10 * 32 + 0] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef32x32[14 * 32 + 0] * transformCoefficients[14 * srcStride + rowIndex] +
            DctCoef32x32[18 * 32 + 0] * transformCoefficients[18 * srcStride + rowIndex] + DctCoef32x32[22 * 32 + 0] * transformCoefficients[22 * srcStride + rowIndex] +
            DctCoef32x32[26 * 32 + 0] * transformCoefficients[26 * srcStride + rowIndex] + DctCoef32x32[30 * 32 + 0] * transformCoefficients[30 * srcStride + rowIndex];
        evenOdd1 = DctCoef32x32[2 * 32 + 1] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef32x32[6 * 32 + 1] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef32x32[10 * 32 + 1] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef32x32[14 * 32 + 1] * transformCoefficients[14 * srcStride + rowIndex] +
            DctCoef32x32[18 * 32 + 1] * transformCoefficients[18 * srcStride + rowIndex] + DctCoef32x32[22 * 32 + 1] * transformCoefficients[22 * srcStride + rowIndex] +
            DctCoef32x32[26 * 32 + 1] * transformCoefficients[26 * srcStride + rowIndex] + DctCoef32x32[30 * 32 + 1] * transformCoefficients[30 * srcStride + rowIndex];
        evenOdd2 = DctCoef32x32[2 * 32 + 2] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef32x32[6 * 32 + 2] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef32x32[10 * 32 + 2] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef32x32[14 * 32 + 2] * transformCoefficients[14 * srcStride + rowIndex] +
            DctCoef32x32[18 * 32 + 2] * transformCoefficients[18 * srcStride + rowIndex] + DctCoef32x32[22 * 32 + 2] * transformCoefficients[22 * srcStride + rowIndex] +
            DctCoef32x32[26 * 32 + 2] * transformCoefficients[26 * srcStride + rowIndex] + DctCoef32x32[30 * 32 + 2] * transformCoefficients[30 * srcStride + rowIndex];
        evenOdd3 = DctCoef32x32[2 * 32 + 3] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef32x32[6 * 32 + 3] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef32x32[10 * 32 + 3] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef32x32[14 * 32 + 3] * transformCoefficients[14 * srcStride + rowIndex] +
            DctCoef32x32[18 * 32 + 3] * transformCoefficients[18 * srcStride + rowIndex] + DctCoef32x32[22 * 32 + 3] * transformCoefficients[22 * srcStride + rowIndex] +
            DctCoef32x32[26 * 32 + 3] * transformCoefficients[26 * srcStride + rowIndex] + DctCoef32x32[30 * 32 + 3] * transformCoefficients[30 * srcStride + rowIndex];
        evenOdd4 = DctCoef32x32[2 * 32 + 4] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef32x32[6 * 32 + 4] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef32x32[10 * 32 + 4] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef32x32[14 * 32 + 4] * transformCoefficients[14 * srcStride + rowIndex] +
            DctCoef32x32[18 * 32 + 4] * transformCoefficients[18 * srcStride + rowIndex] + DctCoef32x32[22 * 32 + 4] * transformCoefficients[22 * srcStride + rowIndex] +
            DctCoef32x32[26 * 32 + 4] * transformCoefficients[26 * srcStride + rowIndex] + DctCoef32x32[30 * 32 + 4] * transformCoefficients[30 * srcStride + rowIndex];
        evenOdd5 = DctCoef32x32[2 * 32 + 5] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef32x32[6 * 32 + 5] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef32x32[10 * 32 + 5] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef32x32[14 * 32 + 5] * transformCoefficients[14 * srcStride + rowIndex] +
            DctCoef32x32[18 * 32 + 5] * transformCoefficients[18 * srcStride + rowIndex] + DctCoef32x32[22 * 32 + 5] * transformCoefficients[22 * srcStride + rowIndex] +
            DctCoef32x32[26 * 32 + 5] * transformCoefficients[26 * srcStride + rowIndex] + DctCoef32x32[30 * 32 + 5] * transformCoefficients[30 * srcStride + rowIndex];
        evenOdd6 = DctCoef32x32[2 * 32 + 6] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef32x32[6 * 32 + 6] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef32x32[10 * 32 + 6] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef32x32[14 * 32 + 6] * transformCoefficients[14 * srcStride + rowIndex] +
            DctCoef32x32[18 * 32 + 6] * transformCoefficients[18 * srcStride + rowIndex] + DctCoef32x32[22 * 32 + 6] * transformCoefficients[22 * srcStride + rowIndex] +
            DctCoef32x32[26 * 32 + 6] * transformCoefficients[26 * srcStride + rowIndex] + DctCoef32x32[30 * 32 + 6] * transformCoefficients[30 * srcStride + rowIndex];
        evenOdd7 = DctCoef32x32[2 * 32 + 7] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef32x32[6 * 32 + 7] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef32x32[10 * 32 + 7] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef32x32[14 * 32 + 7] * transformCoefficients[14 * srcStride + rowIndex] +
            DctCoef32x32[18 * 32 + 7] * transformCoefficients[18 * srcStride + rowIndex] + DctCoef32x32[22 * 32 + 7] * transformCoefficients[22 * srcStride + rowIndex] +
            DctCoef32x32[26 * 32 + 7] * transformCoefficients[26 * srcStride + rowIndex] + DctCoef32x32[30 * 32 + 7] * transformCoefficients[30 * srcStride + rowIndex];

        evenEvenOdd0 = DctCoef32x32[4 * 32 + 0] * transformCoefficients[4 * srcStride + rowIndex] + DctCoef32x32[12 * 32 + 0] * transformCoefficients[12 * srcStride + rowIndex] +
            DctCoef32x32[20 * 32 + 0] * transformCoefficients[20 * srcStride + rowIndex] + DctCoef32x32[28 * 32 + 0] * transformCoefficients[28 * srcStride + rowIndex];
        evenEvenOdd1 = DctCoef32x32[4 * 32 + 1] * transformCoefficients[4 * srcStride + rowIndex] + DctCoef32x32[12 * 32 + 1] * transformCoefficients[12 * srcStride + rowIndex] +
            DctCoef32x32[20 * 32 + 1] * transformCoefficients[20 * srcStride + rowIndex] + DctCoef32x32[28 * 32 + 1] * transformCoefficients[28 * srcStride + rowIndex];
        evenEvenOdd2 = DctCoef32x32[4 * 32 + 2] * transformCoefficients[4 * srcStride + rowIndex] + DctCoef32x32[12 * 32 + 2] * transformCoefficients[12 * srcStride + rowIndex] +
            DctCoef32x32[20 * 32 + 2] * transformCoefficients[20 * srcStride + rowIndex] + DctCoef32x32[28 * 32 + 2] * transformCoefficients[28 * srcStride + rowIndex];
        evenEvenOdd3 = DctCoef32x32[4 * 32 + 3] * transformCoefficients[4 * srcStride + rowIndex] + DctCoef32x32[12 * 32 + 3] * transformCoefficients[12 * srcStride + rowIndex] +
            DctCoef32x32[20 * 32 + 3] * transformCoefficients[20 * srcStride + rowIndex] + DctCoef32x32[28 * 32 + 3] * transformCoefficients[28 * srcStride + rowIndex];

        evenEvenEvenOdd0 = DctCoef32x32[8 * 32 + 0] * transformCoefficients[8 * srcStride + rowIndex] + DctCoef32x32[24 * 32 + 0] * transformCoefficients[24 * srcStride + rowIndex];
        evenEvenEvenOdd1 = DctCoef32x32[8 * 32 + 1] * transformCoefficients[8 * srcStride + rowIndex] + DctCoef32x32[24 * 32 + 1] * transformCoefficients[24 * srcStride + rowIndex];
        evenEvenEvenEven0 = DctCoef32x32[0 * 32 + 0] * transformCoefficients[0 * srcStride + rowIndex] + DctCoef32x32[16 * 32 + 0] * transformCoefficients[16 * srcStride + rowIndex];
        evenEvenEvenEven1 = DctCoef32x32[0 * 32 + 1] * transformCoefficients[0 * srcStride + rowIndex] + DctCoef32x32[16 * 32 + 1] * transformCoefficients[16 * srcStride + rowIndex];

        evenEvenEven0 = evenEvenEvenEven0 + evenEvenEvenOdd0;
        evenEvenEven1 = evenEvenEvenEven1 + evenEvenEvenOdd1;
        evenEvenEven2 = evenEvenEvenEven1 - evenEvenEvenOdd1;
        evenEvenEven3 = evenEvenEvenEven0 - evenEvenEvenOdd0;

        evenEven0 = evenEvenEven0 + evenEvenOdd0;
        evenEven1 = evenEvenEven1 + evenEvenOdd1;
        evenEven2 = evenEvenEven2 + evenEvenOdd2;
        evenEven3 = evenEvenEven3 + evenEvenOdd3;
        evenEven4 = evenEvenEven3 - evenEvenOdd3;
        evenEven5 = evenEvenEven2 - evenEvenOdd2;
        evenEven6 = evenEvenEven1 - evenEvenOdd1;
        evenEven7 = evenEvenEven0 - evenEvenOdd0;

        even0 = evenEven0 + evenOdd0;
        even1 = evenEven1 + evenOdd1;
        even2 = evenEven2 + evenOdd2;
        even3 = evenEven3 + evenOdd3;
        even4 = evenEven4 + evenOdd4;
        even5 = evenEven5 + evenOdd5;
        even6 = evenEven6 + evenOdd6;
        even7 = evenEven7 + evenOdd7;
        even8 = evenEven7 - evenOdd7;
        even9 = evenEven6 - evenOdd6;
        even10 = evenEven5 - evenOdd5;
        even11 = evenEven4 - evenOdd4;
        even12 = evenEven3 - evenOdd3;
        even13 = evenEven2 - evenOdd2;
        even14 = evenEven1 - evenOdd1;
        even15 = evenEven0 - evenOdd0;

        residual[rowStrideIndex + 0] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even0 + odd0 + offset) >> shift)));
        residual[rowStrideIndex + 1] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even1 + odd1 + offset) >> shift)));
        residual[rowStrideIndex + 2] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even2 + odd2 + offset) >> shift)));
        residual[rowStrideIndex + 3] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even3 + odd3 + offset) >> shift)));
        residual[rowStrideIndex + 4] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even4 + odd4 + offset) >> shift)));
        residual[rowStrideIndex + 5] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even5 + odd5 + offset) >> shift)));
        residual[rowStrideIndex + 6] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even6 + odd6 + offset) >> shift)));
        residual[rowStrideIndex + 7] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even7 + odd7 + offset) >> shift)));
        residual[rowStrideIndex + 8] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even8 + odd8 + offset) >> shift)));
        residual[rowStrideIndex + 9] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even9 + odd9 + offset) >> shift)));
        residual[rowStrideIndex + 10] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even10 + odd10 + offset) >> shift)));
        residual[rowStrideIndex + 11] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even11 + odd11 + offset) >> shift)));
        residual[rowStrideIndex + 12] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even12 + odd12 + offset) >> shift)));
        residual[rowStrideIndex + 13] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even13 + odd13 + offset) >> shift)));
        residual[rowStrideIndex + 14] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even14 + odd14 + offset) >> shift)));
        residual[rowStrideIndex + 15] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even15 + odd15 + offset) >> shift)));
        residual[rowStrideIndex + 16] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even15 - odd15 + offset) >> shift)));
        residual[rowStrideIndex + 17] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even14 - odd14 + offset) >> shift)));
        residual[rowStrideIndex + 18] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even13 - odd13 + offset) >> shift)));
        residual[rowStrideIndex + 19] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even12 - odd12 + offset) >> shift)));
        residual[rowStrideIndex + 20] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even11 - odd11 + offset) >> shift)));
        residual[rowStrideIndex + 21] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even10 - odd10 + offset) >> shift)));
        residual[rowStrideIndex + 22] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even9 - odd9 + offset) >> shift)));
        residual[rowStrideIndex + 23] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even8 - odd8 + offset) >> shift)));
        residual[rowStrideIndex + 24] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even7 - odd7 + offset) >> shift)));
        residual[rowStrideIndex + 25] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even6 - odd6 + offset) >> shift)));
        residual[rowStrideIndex + 26] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even5 - odd5 + offset) >> shift)));
        residual[rowStrideIndex + 27] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even4 - odd4 + offset) >> shift)));
        residual[rowStrideIndex + 28] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even3 - odd3 + offset) >> shift)));
        residual[rowStrideIndex + 29] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even2 - odd2 + offset) >> shift)));
        residual[rowStrideIndex + 30] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even1 - odd1 + offset) >> shift)));
        residual[rowStrideIndex + 31] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even0 - odd0 + offset) >> shift)));
    }
}

/*********************************************************************
* PartialButterflyInverse16
*   1D 16x16 inverse transform using partial butterfly
*
*  transformCoefficients
*   is the input pointer for transformed pixels
*
*  residual
*   is the output after applying the inverse butterfly transform. (the output is transposed)
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*********************************************************************/
inline static void PartialButterflyInverse16(
    EB_S16        *transformCoefficients,
    EB_S16        *residual,
    const EB_U32   srcStride,
    const EB_U32   dstStride,
    const EB_U32   shift)
{
    EB_S32 even0, even1, even2, even3, even4, even5, even6, even7;
    EB_S32 odd0, odd1, odd2, odd3, odd4, odd5, odd6, odd7;
    EB_S32 evenEven0, evenEven1, evenEven2, evenEven3;
    EB_S32 evenOdd0, evenOdd1, evenOdd2, evenOdd3;
    EB_S32 evenEvenEven0, evenEvenEven1;
    EB_S32 evenEvenOdd0, evenEvenOdd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 16; rowIndex++) {
        rowStrideIndex = rowIndex*dstStride;
        odd0 = DctCoef16x16[1 * 16 + 0] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef16x16[3 * 16 + 0] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef16x16[5 * 16 + 0] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef16x16[7 * 16 + 0] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef16x16[9 * 16 + 0] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef16x16[11 * 16 + 0] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef16x16[13 * 16 + 0] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef16x16[15 * 16 + 0] * transformCoefficients[15 * srcStride + rowIndex];
        odd1 = DctCoef16x16[1 * 16 + 1] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef16x16[3 * 16 + 1] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef16x16[5 * 16 + 1] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef16x16[7 * 16 + 1] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef16x16[9 * 16 + 1] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef16x16[11 * 16 + 1] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef16x16[13 * 16 + 1] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef16x16[15 * 16 + 1] * transformCoefficients[15 * srcStride + rowIndex];
        odd2 = DctCoef16x16[1 * 16 + 2] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef16x16[3 * 16 + 2] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef16x16[5 * 16 + 2] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef16x16[7 * 16 + 2] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef16x16[9 * 16 + 2] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef16x16[11 * 16 + 2] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef16x16[13 * 16 + 2] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef16x16[15 * 16 + 2] * transformCoefficients[15 * srcStride + rowIndex];
        odd3 = DctCoef16x16[1 * 16 + 3] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef16x16[3 * 16 + 3] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef16x16[5 * 16 + 3] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef16x16[7 * 16 + 3] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef16x16[9 * 16 + 3] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef16x16[11 * 16 + 3] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef16x16[13 * 16 + 3] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef16x16[15 * 16 + 3] * transformCoefficients[15 * srcStride + rowIndex];
        odd4 = DctCoef16x16[1 * 16 + 4] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef16x16[3 * 16 + 4] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef16x16[5 * 16 + 4] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef16x16[7 * 16 + 4] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef16x16[9 * 16 + 4] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef16x16[11 * 16 + 4] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef16x16[13 * 16 + 4] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef16x16[15 * 16 + 4] * transformCoefficients[15 * srcStride + rowIndex];
        odd5 = DctCoef16x16[1 * 16 + 5] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef16x16[3 * 16 + 5] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef16x16[5 * 16 + 5] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef16x16[7 * 16 + 5] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef16x16[9 * 16 + 5] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef16x16[11 * 16 + 5] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef16x16[13 * 16 + 5] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef16x16[15 * 16 + 5] * transformCoefficients[15 * srcStride + rowIndex];
        odd6 = DctCoef16x16[1 * 16 + 6] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef16x16[3 * 16 + 6] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef16x16[5 * 16 + 6] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef16x16[7 * 16 + 6] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef16x16[9 * 16 + 6] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef16x16[11 * 16 + 6] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef16x16[13 * 16 + 6] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef16x16[15 * 16 + 6] * transformCoefficients[15 * srcStride + rowIndex];
        odd7 = DctCoef16x16[1 * 16 + 7] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef16x16[3 * 16 + 7] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef16x16[5 * 16 + 7] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef16x16[7 * 16 + 7] * transformCoefficients[7 * srcStride + rowIndex] +
            DctCoef16x16[9 * 16 + 7] * transformCoefficients[9 * srcStride + rowIndex] + DctCoef16x16[11 * 16 + 7] * transformCoefficients[11 * srcStride + rowIndex] +
            DctCoef16x16[13 * 16 + 7] * transformCoefficients[13 * srcStride + rowIndex] + DctCoef16x16[15 * 16 + 7] * transformCoefficients[15 * srcStride + rowIndex];

        evenOdd0 = DctCoef16x16[2 * 16 + 0] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef16x16[6 * 16 + 0] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef16x16[10 * 16 + 0] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef16x16[14 * 16 + 0] * transformCoefficients[14 * srcStride + rowIndex];
        evenOdd1 = DctCoef16x16[2 * 16 + 1] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef16x16[6 * 16 + 1] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef16x16[10 * 16 + 1] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef16x16[14 * 16 + 1] * transformCoefficients[14 * srcStride + rowIndex];
        evenOdd2 = DctCoef16x16[2 * 16 + 2] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef16x16[6 * 16 + 2] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef16x16[10 * 16 + 2] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef16x16[14 * 16 + 2] * transformCoefficients[14 * srcStride + rowIndex];
        evenOdd3 = DctCoef16x16[2 * 16 + 3] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef16x16[6 * 16 + 3] * transformCoefficients[6 * srcStride + rowIndex] +
            DctCoef16x16[10 * 16 + 3] * transformCoefficients[10 * srcStride + rowIndex] + DctCoef16x16[14 * 16 + 3] * transformCoefficients[14 * srcStride + rowIndex];

        evenEvenOdd0 = DctCoef16x16[4 * 16 + 0] * transformCoefficients[4 * srcStride + rowIndex] + DctCoef16x16[12 * 16 + 0] * transformCoefficients[12 * srcStride + rowIndex];
        evenEvenOdd1 = DctCoef16x16[4 * 16 + 1] * transformCoefficients[4 * srcStride + rowIndex] + DctCoef16x16[12 * 16 + 1] * transformCoefficients[12 * srcStride + rowIndex];
        evenEvenEven0 = DctCoef16x16[0 * 16 + 0] * transformCoefficients[0 * srcStride + rowIndex] + DctCoef16x16[8 * 16 + 0] * transformCoefficients[8 * srcStride + rowIndex];
        evenEvenEven1 = DctCoef16x16[0 * 16 + 1] * transformCoefficients[0 * srcStride + rowIndex] + DctCoef16x16[8 * 16 + 1] * transformCoefficients[8 * srcStride + rowIndex];

        evenEven0 = evenEvenEven0 + evenEvenOdd0;
        evenEven1 = evenEvenEven1 + evenEvenOdd1;
        evenEven2 = evenEvenEven1 - evenEvenOdd1;
        evenEven3 = evenEvenEven0 - evenEvenOdd0;

        even0 = evenEven0 + evenOdd0;
        even1 = evenEven1 + evenOdd1;
        even2 = evenEven2 + evenOdd2;
        even3 = evenEven3 + evenOdd3;
        even4 = evenEven3 - evenOdd3;
        even5 = evenEven2 - evenOdd2;
        even6 = evenEven1 - evenOdd1;
        even7 = evenEven0 - evenOdd0;

        residual[rowStrideIndex + 0] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even0 + odd0 + offset) >> shift)));
        residual[rowStrideIndex + 1] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even1 + odd1 + offset) >> shift)));
        residual[rowStrideIndex + 2] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even2 + odd2 + offset) >> shift)));
        residual[rowStrideIndex + 3] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even3 + odd3 + offset) >> shift)));
        residual[rowStrideIndex + 4] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even4 + odd4 + offset) >> shift)));
        residual[rowStrideIndex + 5] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even5 + odd5 + offset) >> shift)));
        residual[rowStrideIndex + 6] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even6 + odd6 + offset) >> shift)));
        residual[rowStrideIndex + 7] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even7 + odd7 + offset) >> shift)));
        residual[rowStrideIndex + 8] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even7 - odd7 + offset) >> shift)));
        residual[rowStrideIndex + 9] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even6 - odd6 + offset) >> shift)));
        residual[rowStrideIndex + 10] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even5 - odd5 + offset) >> shift)));
        residual[rowStrideIndex + 11] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even4 - odd4 + offset) >> shift)));
        residual[rowStrideIndex + 12] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even3 - odd3 + offset) >> shift)));
        residual[rowStrideIndex + 13] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even2 - odd2 + offset) >> shift)));
        residual[rowStrideIndex + 14] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even1 - odd1 + offset) >> shift)));
        residual[rowStrideIndex + 15] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even0 - odd0 + offset) >> shift)));
    }
}

/*********************************************************************
* PartialButterflyInverse8
*   1D 8x8 inverse transform using partial butterfly
*
*  transformCoefficients
*   is the input pointer for transformed pixels
*
*  residual
*   is the output after applying the inverse butterfly transform. (the output is transposed)
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*********************************************************************/
inline static void PartialButterflyInverse8(
    EB_S16        *transformCoefficients,
    EB_S16        *residual,
    const EB_U32   srcStride,
    const EB_U32   dstStride,
    const EB_U32   shift)
{
    EB_S32 even0, even1, even2, even3;
    EB_S32 odd0, odd1, odd2, odd3;
    EB_S32 evenEven0, evenEven1;
    EB_S32 evenOdd0, evenOdd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);

    for (rowIndex = 0; rowIndex < 8; rowIndex++) {
        rowStrideIndex = rowIndex*dstStride;
        odd0 = DctCoef8x8[1 * 8 + 0] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef8x8[3 * 8 + 0] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef8x8[5 * 8 + 0] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef8x8[7 * 8 + 0] * transformCoefficients[7 * srcStride + rowIndex];
        odd1 = DctCoef8x8[1 * 8 + 1] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef8x8[3 * 8 + 1] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef8x8[5 * 8 + 1] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef8x8[7 * 8 + 1] * transformCoefficients[7 * srcStride + rowIndex];
        odd2 = DctCoef8x8[1 * 8 + 2] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef8x8[3 * 8 + 2] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef8x8[5 * 8 + 2] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef8x8[7 * 8 + 2] * transformCoefficients[7 * srcStride + rowIndex];
        odd3 = DctCoef8x8[1 * 8 + 3] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef8x8[3 * 8 + 3] * transformCoefficients[3 * srcStride + rowIndex] +
            DctCoef8x8[5 * 8 + 3] * transformCoefficients[5 * srcStride + rowIndex] + DctCoef8x8[7 * 8 + 3] * transformCoefficients[7 * srcStride + rowIndex];

        evenOdd0 = DctCoef8x8[2 * 8 + 0] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef8x8[6 * 8 + 0] * transformCoefficients[6 * srcStride + rowIndex];
        evenOdd1 = DctCoef8x8[2 * 8 + 1] * transformCoefficients[2 * srcStride + rowIndex] + DctCoef8x8[6 * 8 + 1] * transformCoefficients[6 * srcStride + rowIndex];
        evenEven0 = DctCoef8x8[0 * 8 + 0] * transformCoefficients[0 * srcStride + rowIndex] + DctCoef8x8[4 * 8 + 0] * transformCoefficients[4 * srcStride + rowIndex];
        evenEven1 = DctCoef8x8[0 * 8 + 1] * transformCoefficients[0 * srcStride + rowIndex] + DctCoef8x8[4 * 8 + 1] * transformCoefficients[4 * srcStride + rowIndex];

        even0 = evenEven0 + evenOdd0;
        even1 = evenEven1 + evenOdd1;
        even2 = evenEven1 - evenOdd1;
        even3 = evenEven0 - evenOdd0;

        residual[rowStrideIndex + 0] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even0 + odd0 + offset) >> shift)));
        residual[rowStrideIndex + 1] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even1 + odd1 + offset) >> shift)));
        residual[rowStrideIndex + 2] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even2 + odd2 + offset) >> shift)));
        residual[rowStrideIndex + 3] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even3 + odd3 + offset) >> shift)));
        residual[rowStrideIndex + 4] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even3 - odd3 + offset) >> shift)));
        residual[rowStrideIndex + 5] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even2 - odd2 + offset) >> shift)));
        residual[rowStrideIndex + 6] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even1 - odd1 + offset) >> shift)));
        residual[rowStrideIndex + 7] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even0 - odd0 + offset) >> shift)));
    }
}


/*********************************************************************
* PartialButterflyInverse4
*   1D 4x4 inverse transform using partial butterfly
*
*  transformCoefficients
*   is the input pointer for transformed pixels
*
*  residual
*   is the output after applying the inverse butterfly transform. (the output is transposed)
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*********************************************************************/
inline static void PartialButterflyInverse4(
    EB_S16        *transformCoefficients,
    EB_S16        *residual,
    const EB_U32   srcStride,
    const EB_U32   dstStride,
    const EB_U32   shift)
{
    EB_S32 even0, even1, odd0, odd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);
    for (rowIndex = 0; rowIndex < 4; rowIndex++) {
        rowStrideIndex = rowIndex*dstStride;

        odd0 = DctCoef4x4[1 * 4 + 0] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef4x4[3 * 4 + 0] * transformCoefficients[3 * srcStride + rowIndex];
        odd1 = DctCoef4x4[1 * 4 + 1] * transformCoefficients[1 * srcStride + rowIndex] + DctCoef4x4[3 * 4 + 1] * transformCoefficients[3 * srcStride + rowIndex];
        even0 = DctCoef4x4[0 * 4 + 0] * transformCoefficients[0 * srcStride + rowIndex] + DctCoef4x4[2 * 4 + 0] * transformCoefficients[2 * srcStride + rowIndex];
        even1 = DctCoef4x4[0 * 4 + 1] * transformCoefficients[0 * srcStride + rowIndex] + DctCoef4x4[2 * 4 + 1] * transformCoefficients[2 * srcStride + rowIndex];

        residual[rowStrideIndex + 0] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even0 + odd0 + offset) >> shift)));
        residual[rowStrideIndex + 1] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even1 + odd1 + offset) >> shift)));
        residual[rowStrideIndex + 2] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even1 - odd1 + offset) >> shift)));
        residual[rowStrideIndex + 3] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((even0 - odd0 + offset) >> shift)));
    }
}

/*********************************************************************
* DstInverse4
*********************************************************************/
inline static void DstInverse4(
    EB_S16        *transformCoefficients,
    EB_S16        *residual,
    const EB_U32   srcStride,
    const EB_U32   dstStride,
    const EB_U32   shift)
{
    EB_S32 even0, even1, odd0, odd1;
    EB_U32 rowIndex, rowStrideIndex;
    const EB_S16 offset = 1 << (shift - 1);
    for (rowIndex = 0; rowIndex < 4; rowIndex++) {
        rowStrideIndex = rowIndex*dstStride;

        odd0 = transformCoefficients[rowIndex] + transformCoefficients[2 * srcStride + rowIndex];
        odd1 = transformCoefficients[rowIndex] - transformCoefficients[3 * srcStride + rowIndex];
        even0 = transformCoefficients[2 * srcStride + rowIndex] + transformCoefficients[3 * srcStride + rowIndex];
        even1 = 74 * transformCoefficients[srcStride + rowIndex];

        residual[rowStrideIndex + 0] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((29 * odd0 + 55 * even0 + even1 + offset) >> shift)));
        residual[rowStrideIndex + 1] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((55 * odd1 - 29 * even0 + even1 + offset) >> shift)));
        residual[rowStrideIndex + 2] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, (((74 * (transformCoefficients[rowIndex] - transformCoefficients[2 * srcStride + rowIndex] + transformCoefficients[3 * srcStride + rowIndex])) + offset) >> shift)));
        residual[rowStrideIndex + 3] = (EB_S16)(CLIP3(MIN_NEG_16BIT_NUM, MAX_POS_16BIT_NUM, ((55 * odd0 + 29 * odd1 - even1 + offset) >> shift)));
    }
}

/*********************************************************************
* Transform32x32
*   2D 32x32  forward transform using two partial butterfly
*
*  residual
*   is the input pointer for residual
*
*  transformCoefficients
*   is the output after applying the butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transformCoefficients pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void Transform32x32(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{

    const EB_U32 shift1st = 4 + bitIncrement; // 2;//log2(trasnform size)-1+TRANS_BIT_INCREMENT
    const EB_U32 shift2nd = 11;                     // log2(trasnform size) + 6


    const EB_U32 transformInnerArrayStride = 32;

    PartialButterfly32(
        residual,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterfly32(
        transformInnerArrayPtr,
        transformCoefficients,
        transformInnerArrayStride,
        dstStride,
        shift2nd);

    return;
}

void Transform32x32Estimate(
	EB_S16                  *residual,
	const EB_U32             srcStride,
	EB_S16                  *transformCoefficients,
	const EB_U32             dstStride,
	EB_S16                  *transformInnerArrayPtr,
	EB_U32                   bitIncrement)
{

	const EB_U32 shift1st = 6 + bitIncrement; // 2;//log2(trasnform size)-1+TRANS_BIT_INCREMENT
	const EB_U32 shift2nd = 9;                     // log2(trasnform size) + 6


	const EB_U32 transformInnerArrayStride = 32;

	PartialButterfly32Estimate(
		residual,
		transformInnerArrayPtr,
		srcStride,
		transformInnerArrayStride,
		shift1st);

	PartialButterfly32Estimate(
		transformInnerArrayPtr,
		transformCoefficients,
		transformInnerArrayStride,
		dstStride,
		shift2nd);

	return;
}

/*********************************************************************
* Transform16x16
*   2D 16x16  forward transform using two partial butterfly
*
*  residual
*   is the input pointer for residual
*
*  transformCoefficients
*   is the output after applying the butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transformCoefficients pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void Transform16x16(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = 3 + bitIncrement; ////2;log2(trasnform size)-1+TRANS_BIT_INCREMENT
    const EB_U32 shift2nd = 10;                     // log2(trasnform size) + 6
    const EB_U32 transformInnerArrayStride = 16;


    PartialButterfly16(
        residual,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterfly16(
        transformInnerArrayPtr,
        transformCoefficients,
        transformInnerArrayStride,
        dstStride,
        shift2nd);

    return;
}

void Transform16x16Estimate(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = 4 + bitIncrement; ////2;log2(trasnform size)-1+TRANS_BIT_INCREMENT
    const EB_U32 shift2nd = 9;                     // log2(trasnform size) + 6
    const EB_U32 transformInnerArrayStride = 16;


    PartialButterfly16Estimate(
        residual,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterfly16Estimate(
        transformInnerArrayPtr,
        transformCoefficients,
        transformInnerArrayStride,
        dstStride,
        shift2nd);

    return;
}

/*********************************************************************
* Transform8x8
*   2D 8x8  forward transform using two partial butterfly
*
*  residual
*   is the input pointer for residual
*
*  transformCoefficients
*   is the output after applying the butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transformCoefficients pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void Transform8x8(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = 2 + bitIncrement; //2;// log2(trasnform size)-1+TRANS_BIT_INCREMENT
    const EB_U32 shift2nd = 9;                      // log2(trasnform size) + 6
    const EB_U32 transformInnerArrayStride = 8;

    PartialButterfly8(
        residual,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterfly8(
        transformInnerArrayPtr,
        transformCoefficients,
        transformInnerArrayStride,
        dstStride,
        shift2nd);

    return;
}

/*********************************************************************
* Transform4x4
*   2D 4x4  forward transform using two partial butterfly
*
*  residual
*   is the input pointer for residual
*
*  transformCoefficients
*   is the output after applying the butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transformCoefficients pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void Transform4x4(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = 1 + bitIncrement; //2;// log2(TransformSize)-1+TRANS_BIT_INCREMENT
    const EB_U32 shift2nd = 8;                      // log2(TransformSize) + 6
    const EB_U32 transformInnerArrayStride = 4;



    PartialButterfly4(
        residual,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterfly4(
        transformInnerArrayPtr,
        transformCoefficients,
        transformInnerArrayStride,
        dstStride,
        shift2nd);

    return;
}

/*********************************************************************
* DstTransform4x4
*
*  residual
*   is the input pointer for residual
*
*  transformCoefficients
*   is the output after applying the butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for residual and transformCoefficients pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void DstTransform4x4(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = 1 + bitIncrement; // log2(TransformSize)-1+TRANS_BIT_INCREMENT
    const EB_U32 shift2nd = 8;                      // log2(TransformSize) + 6
    const EB_U32 transformInnerArrayStride = 4;

    Dst4(
        residual,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    Dst4(
        transformInnerArrayPtr,
        transformCoefficients,
        transformInnerArrayStride,
        dstStride,
        shift2nd);


    return;
}

/*********************************************************************
* InvTransform32x32
*   2D 32x32 inverse transform using two partial butterfly
*
*  transformCoefficients
*   is the input pointer for transformed residual
*
*  residual
*   is the output after applying the inverse butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void InvTransform32x32(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = SHIFT_INV_1ST;
    const EB_U32 shift2nd = SHIFT_INV_2ND - bitIncrement;//2;//CHKN

    const EB_U32 transformInnerArrayStride = 32;

    PartialButterflyInverse32(
        transformCoefficients,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterflyInverse32(
        transformInnerArrayPtr,
        residual,
        transformInnerArrayStride,
        dstStride,
        shift2nd);
}

/*********************************************************************
* InvTransform16x16
*   2D 16x16 inverse transform using two partial butterfly
*
*  transformCoefficients
*   is the input pointer for transformed residual
*
*  residual
*   is the output after applying the inverse butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void InvTransform16x16(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = SHIFT_INV_1ST;
    const EB_U32 shift2nd = SHIFT_INV_2ND - bitIncrement;//2;//CHKN TRANS_BIT_INCREMENT;
    const EB_U32 transformInnerArrayStride = 16;

    PartialButterflyInverse16(
        transformCoefficients,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterflyInverse16(
        transformInnerArrayPtr,
        residual,
        transformInnerArrayStride,
        dstStride,
        shift2nd);
}

/*********************************************************************
* InvTransform8x8
*   2D 8x8 inverse transform using two partial butterfly
*
*  transformCoefficients
*   is the input pointer for transformed residual
*
*  residual
*   is the output after applying the inverse butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void InvTransform8x8(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = SHIFT_INV_1ST;
    const EB_U32 shift2nd = SHIFT_INV_2ND - bitIncrement;//2;//CHKN TRANS_BIT_INCREMENT;-
    const EB_U32 transformInnerArrayStride = 8;

    PartialButterflyInverse8(
        transformCoefficients,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterflyInverse8(
        transformInnerArrayPtr,
        residual,
        transformInnerArrayStride,
        dstStride,
        shift2nd);
}

/*********************************************************************
* InvTransform4x4
*   2D 4x4 inverse transform using two partial butterfly
*
*  transformCoefficients
*   is the input pointer for transformed residual
*
*  residual
*   is the output after applying the inverse butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void InvTransform4x4(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = SHIFT_INV_1ST;
    const EB_U32 shift2nd = SHIFT_INV_2ND - bitIncrement;//2;//CHKN TRANS_BIT_INCREMENT;-
    const EB_U32 transformInnerArrayStride = 4;

    PartialButterflyInverse4(
        transformCoefficients,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    PartialButterflyInverse4(
        transformInnerArrayPtr,
        residual,
        transformInnerArrayStride,
        dstStride,
        shift2nd);
}

/*********************************************************************
* InvDstTransform4x4
*
*  transformCoefficients
*   is the input pointer for transformed residual
*
*  residual
*   is the output after applying the inverse butterfly transforms.
*
*  srcStride and dstStride
*   are the inputs and show the strides for transformCoefficients and residual pointers
*
*  shift
*   is the input and shows the number of shifts to right
*
*  transformInnerArrayPtr
*   is an input and a pointer to the inner temporary buffer used in Transform/Inverse Transform functions
*********************************************************************/
void InvDstTransform4x4(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement)
{
    const EB_U32 shift1st = SHIFT_INV_1ST;
    const EB_U32 shift2nd = SHIFT_INV_2ND - bitIncrement;
    const EB_U32 transformInnerArrayStride = 4;

    DstInverse4(
        transformCoefficients,
        transformInnerArrayPtr,
        srcStride,
        transformInnerArrayStride,
        shift1st);

    DstInverse4(
        transformInnerArrayPtr,
        residual,
        transformInnerArrayStride,
        dstStride,
        shift2nd);
}