/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


// This file remains in ebHevcEncLib because the functions have dependencies Lib structs

#include "emmintrin.h"
#include "EbEntropyCoding.h"
#include "EbCombinedAveragingSAD_Intrinsic_AVX512.h"

#define ONE_BIT                 32

// Table of subblock scans
// Note: for 4x4, only one entry with value 0 is required (hence reusing 8x8 scan)
static const EB_U8 *sbScans[] =
{
    sbScan8, sbScan8, sbScan16, sbScan32
};

static const EB_U8 CabacEstimatedBitsV2[] =
{
  0x1f, 0x21, 0x1d, 0x23, 0x1c, 0x25, 0x1a, 0x27, 0x18, 0x2a, 0x17, 0x2c, 0x15, 0x2e, 0x14, 0x31,
  0x13, 0x33, 0x11, 0x35, 0x10, 0x38, 0x0f, 0x3a, 0x0e, 0x3d, 0x0e, 0x3f, 0x0d, 0x42, 0x0c, 0x44,
  0x0b, 0x47, 0x0b, 0x49, 0x0a, 0x4b, 0x0a, 0x4e, 0x09, 0x50, 0x08, 0x52, 0x08, 0x55, 0x08, 0x57,
  0x07, 0x5a, 0x07, 0x5c, 0x06, 0x5e, 0x06, 0x61, 0x06, 0x63, 0x05, 0x65, 0x05, 0x68, 0x05, 0x6a,
  0x05, 0x6d, 0x04, 0x6f, 0x04, 0x72, 0x04, 0x74, 0x04, 0x76, 0x04, 0x79, 0x03, 0x7b, 0x03, 0x7e,
  0x03, 0x80, 0x03, 0x82, 0x03, 0x85, 0x03, 0x88, 0x02, 0x8a, 0x02, 0x8c, 0x02, 0x8e, 0x02, 0x91,
  0x02, 0x93, 0x02, 0x96, 0x02, 0x98, 0x02, 0x9b, 0x02, 0x9c, 0x01, 0xa0, 0x01, 0xa1, 0x01, 0xa4,
  0x01, 0xa7, 0x01, 0xaa, 0x01, 0xab, 0x01, 0xad, 0x01, 0xb0, 0x01, 0xb3, 0x01, 0xb5, 0x00, 0xf0
};



/**********************************************************************
 *
 **********************************************************************/
// 60*2 for luma
// 28*2 for chroma
// luma / size / dir


static EB_U32 EncodeLastSignificantXYTemp(
	CabacCost_t            *CabacCost,
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	EB_U32                  lastSigXPos,
	EB_U32                  lastSigYPos,
	const EB_U32            size,
	const EB_U32            isChroma)
{
  EB_U32 coeffBits = 0;

  EB_S32 offset = (isChroma) ? 60*2 : 0;
  offset -= 2*4;

  // Fix for the DC path
  if (size == 1) {
      coeffBits += CabacCost->CabacBitsLast[+0];
      coeffBits += CabacCost->CabacBitsLast[+1];
  }
  else {
      coeffBits += CabacCost->CabacBitsLast[offset + 2 * (lastSigXPos + size) + 0];
      coeffBits += CabacCost->CabacBitsLast[offset + 2 * (lastSigYPos + size) + 1];
  }

  (void) cabacEncodeCtxPtr;
  return coeffBits;
}

static EB_U32 EncodeLastSignificantXTemp(
                                          CabacEncodeContext_t   *cabacEncodeCtxPtr,
                                          EB_U32                  lastSigXPos,
                                          const EB_U32            size,
                                          const EB_U32            logBlockSize,
                                          const EB_U32            isChroma)
{
  EB_U32 coeffBits = 0;
  EB_U32 xGroupIndex;
  EB_S32 blockSizeOffset;
  EB_S32 shift;
  EB_U32 contextIndex;
  EB_S32 groupCount;
  
  blockSizeOffset = isChroma ?  NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize-2)*3 + ((logBlockSize-1)>>2));
  shift           = isChroma ? (logBlockSize-2)  : ((logBlockSize+1)>>2);
  
  // X position
  xGroupIndex = lastSigXYGroupIndex[ lastSigXPos ];
  
  for (contextIndex = 0; contextIndex < xGroupIndex; contextIndex++)
  {
    coeffBits += CabacEstimatedBitsV2[ 1 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[ blockSizeOffset + (contextIndex>>shift) ] ];
  }
  
  if (xGroupIndex < lastSigXYGroupIndex[ size - 1 ])
  {
    coeffBits += CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[ blockSizeOffset + (contextIndex>>shift) ] ];
  }
  
  if (xGroupIndex > 3)
  {
    groupCount  = (xGroupIndex - 2 ) >> 1;
    coeffBits += groupCount * ONE_BIT;
  }
  
  return coeffBits;
}

static EB_U32 EncodeLastSignificantYTemp(
                                          CabacEncodeContext_t   *cabacEncodeCtxPtr,
                                          EB_U32                  lastSigYPos,
                                          const EB_U32            size,
                                          const EB_U32            logBlockSize,
                                          const EB_U32            isChroma)
{
  EB_U32 coeffBits = 0;
  EB_U32 yGroupIndex;
  EB_S32 blockSizeOffset;
  EB_S32 shift;
  EB_U32 contextIndex;
  EB_S32 groupCount;
  
  blockSizeOffset = isChroma ?  NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize-2)*3 + ((logBlockSize-1)>>2));
  shift           = isChroma ? (logBlockSize-2)  : ((logBlockSize+1)>>2);
  
  // Y position
  yGroupIndex = lastSigXYGroupIndex[ lastSigYPos ];
  
  for (contextIndex = 0; contextIndex < yGroupIndex; contextIndex++)
  {
    coeffBits += CabacEstimatedBitsV2[ 1 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[ blockSizeOffset + (contextIndex>>shift) ] ];
  }
  
  if (yGroupIndex < lastSigXYGroupIndex[ size - 1 ])
  {
    coeffBits += CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[ blockSizeOffset + (contextIndex>>shift) ] ];
  }
  
  if (yGroupIndex > 3)
  {
    groupCount  = (yGroupIndex - 2 ) >> 1;
    coeffBits += groupCount * ONE_BIT;
  }
  
  return coeffBits;
}

/*****************************************
 * PrecomputeCabacCost:
  this function precomputes the estimated
  bits for a number of syntax elements in
  the TU Coeff function.Since the Estimation
  relies on the fact that the context of these
  syntax elements changes only on a frame
  by frame basis.this fucntion precomutes all
  possibles values at the start of the frame.
  we just Look up the estimated bits when needed
  in the CU level.
 *****************************************/
void PrecomputeCabacCost(CabacCost_t            *CabacCostPtr,
                         CabacEncodeContext_t   *cabacEncodeCtxPtr)
{
  EB_S32 i;

  //LastSignificantX/Y: here we save the call of the EncodeLastSignificantXY function
  for (i = 0; i < 4; i++)
  {
    CabacCostPtr->CabacBitsLast[0+2*i] = EncodeLastSignificantXTemp(cabacEncodeCtxPtr, i, 4, 2, 0);
    CabacCostPtr->CabacBitsLast[1+2*i] = EncodeLastSignificantYTemp(cabacEncodeCtxPtr, i, 4, 2, 0);
    CabacCostPtr->CabacBitsLast[120+2*i] = EncodeLastSignificantXTemp(cabacEncodeCtxPtr, i, 4, 2, 1);
    CabacCostPtr->CabacBitsLast[121+2*i] = EncodeLastSignificantYTemp(cabacEncodeCtxPtr, i, 4, 2, 1);
  }
  
  for (i = 0; i < 8; i++)
  {
    CabacCostPtr->CabacBitsLast[8+2*i] = EncodeLastSignificantXTemp(cabacEncodeCtxPtr, i, 8, 3, 0);
    CabacCostPtr->CabacBitsLast[9+2*i] = EncodeLastSignificantYTemp(cabacEncodeCtxPtr, i, 8, 3, 0);
    CabacCostPtr->CabacBitsLast[128+2*i] = EncodeLastSignificantXTemp(cabacEncodeCtxPtr, i, 8, 3, 1);
    CabacCostPtr->CabacBitsLast[129+2*i] = EncodeLastSignificantYTemp(cabacEncodeCtxPtr, i, 8, 3, 1);
  }

  for (i = 0; i < 16; i++)
  {
    CabacCostPtr->CabacBitsLast[24+2*i] = EncodeLastSignificantXTemp(cabacEncodeCtxPtr, i, 16, 4, 0);
    CabacCostPtr->CabacBitsLast[25+2*i] = EncodeLastSignificantYTemp(cabacEncodeCtxPtr, i, 16, 4, 0);
    CabacCostPtr->CabacBitsLast[144+2*i] = EncodeLastSignificantXTemp(cabacEncodeCtxPtr, i, 16, 4, 1);
    CabacCostPtr->CabacBitsLast[145+2*i] = EncodeLastSignificantYTemp(cabacEncodeCtxPtr, i, 16, 4, 1);
  }

  for (i = 0; i < 32; i++)
  {
    CabacCostPtr->CabacBitsLast[56+2*i] = EncodeLastSignificantXTemp(cabacEncodeCtxPtr, i, 32, 5, 0);
    CabacCostPtr->CabacBitsLast[57+2*i] = EncodeLastSignificantYTemp(cabacEncodeCtxPtr, i, 32, 5, 0);
  }

  //Significance Flag bits: we save the inner Table lookup
  for (i = 0; i < TOTAL_NUMBER_OF_SIG_FLAG_CONTEXT_MODELS; i++)
  {
    CabacCostPtr->CabacBitsSig[0+2*i] = CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.significanceFlagContextModel[ i ] ];
    CabacCostPtr->CabacBitsSig[1+2*i] = CabacEstimatedBitsV2[ 1 ^ cabacEncodeCtxPtr->contextModelEncContext.significanceFlagContextModel[ i ] ];
  }

  //bits greater than One: we save the inner Table lookup
  for (i = 0; i < TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CONTEXT_MODELS; i++)
  {
    CabacCostPtr->CabacBitsG1[0+2*i] = CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[ i ] ];
    CabacCostPtr->CabacBitsG1[1+2*i] = CabacEstimatedBitsV2[ 1 ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[ i ] ];
  }
  //bits greater than Two: we save the inner Table lookup
  for (i = 0; i < TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CONTEXT_MODELS; i++)
  {
    CabacCostPtr->CabacBitsG2[0+2*i] = CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[ i ] ];
    CabacCostPtr->CabacBitsG2[1+2*i] = CabacEstimatedBitsV2[ 1 ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[ i ] ];
  }
  //Coeff group: we save the inner Table lookup
  for (i = 0; i < TOTAL_NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS; i++)
  {
    CabacCostPtr->CabacBitsSigMl[0+2*i] = CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.coeffGroupSigFlagContextModel[ i ] ];
    CabacCostPtr->CabacBitsSigMl[1+2*i] = CabacEstimatedBitsV2[ 1 ^ cabacEncodeCtxPtr->contextModelEncContext.coeffGroupSigFlagContextModel[ i ] ];
  }
  for (i = 0; i < TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CONTEXT_MODELS/4; i++)
  {
    EB_S32 j;
    CabacCostPtr->CabacBitsG1x[16*i+0] = CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[ 4*i+1 ] ];
    CabacCostPtr->CabacBitsG1x[16*i+1] = CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[ 4*i+2 ] ] + CabacCostPtr->CabacBitsG1x[16*i+0];
    for (j = 2; j < 8; j++)
    {
      CabacCostPtr->CabacBitsG1x[16*i+j] = CabacEstimatedBitsV2[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[ 4*i+3 ] ] + CabacCostPtr->CabacBitsG1x[16*i+j-1];
    }
    for (j = 8; j < 16; j++)
    {
      CabacCostPtr->CabacBitsG1x[16*i+j] = ONE_BIT + CabacCostPtr->CabacBitsG1x[16*i+j-1];
    }
  }

  for (i = 0; i < 3; i++)
  {
    EB_S32 j;
    
    for (j = 0; j < 16; j++)
    {
      CabacCostPtr->CabacBitsSigV[2*i+0][j] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap4[i][j]];
      CabacCostPtr->CabacBitsSigV[2*i+1][j] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap4[i][j]];
      CabacCostPtr->CabacBitsSigV[2*i+6][j] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap4[i][j]+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
      CabacCostPtr->CabacBitsSigV[2*i+7][j] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap4[i][j]+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
    }
  }
  
  for (i = 0; i < 16; i++)
  {
    // Index: diag 0 / non diag 1 : context, always 0 : pos
    CabacCostPtr->CabacBitsSigV[12+0][i] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap8[0][0][i]+2*9];
    CabacCostPtr->CabacBitsSigV[12+1][i] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap8[0][0][i]+2*9];
    CabacCostPtr->CabacBitsSigV[14+0][i] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap8[0][0][i]+2*12];
    CabacCostPtr->CabacBitsSigV[14+1][i] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap8[0][0][i]+2*12];
    CabacCostPtr->CabacBitsSigV[16+0][i] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap8[0][0][i]+2*21];
    CabacCostPtr->CabacBitsSigV[16+1][i] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap8[0][0][i]+2*21];
    CabacCostPtr->CabacBitsSigV[18+0][i] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap8[0][0][i]+2*24];
    CabacCostPtr->CabacBitsSigV[18+1][i] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap8[0][0][i]+2*24];
    CabacCostPtr->CabacBitsSigV[20+0][i] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap8[1][0][i]+2*15];
    CabacCostPtr->CabacBitsSigV[20+1][i] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap8[1][0][i]+2*15];
    CabacCostPtr->CabacBitsSigV[22+0][i] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap8[1][0][i]+2*18];
    CabacCostPtr->CabacBitsSigV[22+1][i] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap8[1][0][i]+2*18];
    
    CabacCostPtr->CabacBitsSigV[24+0][i] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap8[0][0][i]+2*9+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
    CabacCostPtr->CabacBitsSigV[24+1][i] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap8[0][0][i]+2*9+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
    CabacCostPtr->CabacBitsSigV[26+0][i] = CabacCostPtr->CabacBitsSigV[24+0][i];
    CabacCostPtr->CabacBitsSigV[26+1][i] = CabacCostPtr->CabacBitsSigV[24+1][i];
    CabacCostPtr->CabacBitsSigV[28+0][i] = CabacCostPtr->CabacBitsSig[1+2*contextIndexMap8[0][0][i]+2*12+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
    CabacCostPtr->CabacBitsSigV[28+1][i] = CabacCostPtr->CabacBitsSig[0+2*contextIndexMap8[0][0][i]+2*12+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
    CabacCostPtr->CabacBitsSigV[30+0][i] = CabacCostPtr->CabacBitsSigV[28+0][i];
    CabacCostPtr->CabacBitsSigV[30+1][i] = CabacCostPtr->CabacBitsSigV[28+1][i];

    // DC special case
    if (i == 0)
    {
      CabacCostPtr->CabacBitsSigV[12+0][i] = CabacCostPtr->CabacBitsSig[1+2*0];
      CabacCostPtr->CabacBitsSigV[12+1][i] = CabacCostPtr->CabacBitsSig[0+2*0];
      CabacCostPtr->CabacBitsSigV[16+0][i] = CabacCostPtr->CabacBitsSig[1+2*0];
      CabacCostPtr->CabacBitsSigV[16+1][i] = CabacCostPtr->CabacBitsSig[0+2*0];
      CabacCostPtr->CabacBitsSigV[20+0][i] = CabacCostPtr->CabacBitsSig[1+2*0];
      CabacCostPtr->CabacBitsSigV[20+1][i] = CabacCostPtr->CabacBitsSig[0+2*0];
      
      CabacCostPtr->CabacBitsSigV[24+0][i] = CabacCostPtr->CabacBitsSig[1+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
      CabacCostPtr->CabacBitsSigV[24+1][i] = CabacCostPtr->CabacBitsSig[0+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
      CabacCostPtr->CabacBitsSigV[28+0][i] = CabacCostPtr->CabacBitsSig[1+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
      CabacCostPtr->CabacBitsSigV[28+1][i] = CabacCostPtr->CabacBitsSig[0+2*NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS];
    }
  }
  
  for (i = 0; i < 32; i += 2)
  {
    EB_S32 j;
    for (j = 0; j < 16; j++)
    {
      // Xor the data as it will be retrieved as a^(b&mask) instead of (a&~mask)|(b&mask)
      CabacCostPtr->CabacBitsSigV[i+1][j] ^= CabacCostPtr->CabacBitsSigV[i][j];
    }
  }

}


/**********************************************************************
 *
 **********************************************************************/

static EB_U32 RemainingCoeffExponentialGolombCodeTemp(EB_U32 symbolValue, EB_U32 golombParam)
{
  EB_S32 codeWord = symbolValue >> golombParam;
  EB_U32 numberOfBins;
  
  numberOfBins = golombParam + 1;
  if (codeWord < COEF_REMAIN_BIN_REDUCTION)
  {
    numberOfBins += codeWord;
  }
  else
  {
    codeWord -= COEF_REMAIN_BIN_REDUCTION - 1;
    numberOfBins += 2 * Log2f(codeWord) + COEF_REMAIN_BIN_REDUCTION;
  }
  
  return ONE_BIT * numberOfBins;
}

static const EB_U8 g_mask[32] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static inline EB_U32 RemainingCoeffExponentialGolombCodeTempZeroParam(EB_U32 symbolValue)
{
	EB_S32 codeWord = symbolValue;
	EB_U32 numberOfBins;

	numberOfBins = 1;
	if (codeWord < COEF_REMAIN_BIN_REDUCTION)
	{
		numberOfBins += codeWord;
	}
	else
	{
		codeWord -= COEF_REMAIN_BIN_REDUCTION - 1;
		numberOfBins += 2 * Log2f(codeWord) + COEF_REMAIN_BIN_REDUCTION;
	}

	return ONE_BIT * numberOfBins;
}
EB_ERRORTYPE PmEstimateQuantCoeffLuma_SSE2(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 coeffBits = 0;	
	EB_S32 subSetIndex;
	EB_U32 coeffGroupPosition;
	EB_U32 posLast;
	EB_S32 lastSigXPos;
	EB_S32 lastSigYPos;


	__m128i sigBits = _mm_setzero_si128();
	__m128i cs0, cs1;
	__m128i z0 = _mm_setzero_si128();
	__m128i *sigBitsPtr;

	(void)(cabacEncodeCtxPtr);
	(void)(type);
	(void)(intraLumaMode);
	(void)(intraChromaMode);
	(void)componentType;
	(void)size;
	// Coefficients ordered according to scan order (absolute values)
    __m128i linearCoeff[MAX_TU_SIZE * MAX_TU_SIZE / (sizeof(__m128i) / sizeof(EB_U16))] = {{0}};
	EB_U16 *linearCoeffBufferPtr = (EB_U16 *)linearCoeff;
	EB_U16 greaterThan1Map;




	// DC-only fast track
	if (numNonZeroCoeffs == 1 && coeffBufferPtr[0] != 0)
	{
		EB_S32 absVal;
		EB_U32 symbolValue;

		coeffBits += CabacCost->CabacBitsLast[0];
		coeffBits += CabacCost->CabacBitsLast[1];

		absVal = ABS(coeffBufferPtr[0]);
		symbolValue = absVal > 1;
		coeffBits += CabacCost->CabacBitsG1[2 + symbolValue];

		if (symbolValue)
		{
			symbolValue = absVal > 2;

			// Add bits for coeff_abs_level_greater2_flag
			coeffBits += CabacCost->CabacBitsG2[symbolValue];

			if (symbolValue)
			{
				coeffBits += RemainingCoeffExponentialGolombCodeTempZeroParam(absVal - 3);

			}
		}

		coeffBits += ONE_BIT; // Sign bit			

		*coeffBitsLong += coeffBits << 10;

		return return_error;
	}



	//-------------------------------------------------------------------------------------------------------------------

	// Loop through subblocks to reorder coefficients according to scan order
	// Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients

	EB_S32 sigmap;

	{

		__m128i z1;
		__m128i a0, a1, a2, a3;
		__m128i b0, b1, c0, c1, d0, d1;
		__m128i s0, s1;

		EB_S16 *subblockPtr;

		// determine position of subblock within transform block
		subblockPtr = coeffBufferPtr;

		a0 = _mm_loadl_epi64((__m128i *)(subblockPtr + 0 * coeffStride)); // 00 01 02 03 -- -- -- --
		a1 = _mm_loadl_epi64((__m128i *)(subblockPtr + 1 * coeffStride)); // 10 11 12 13 -- -- -- --
		a2 = _mm_loadl_epi64((__m128i *)(subblockPtr + 2 * coeffStride)); // 20 21 22 23 -- -- -- --
		a3 = _mm_loadl_epi64((__m128i *)(subblockPtr + 3 * coeffStride)); // 30 31 32 33 -- -- -- --

		{
			EB_S32 v03, v30;

			b0 = _mm_unpacklo_epi64(a0, a3); // 00 01 02 03 30 31 32 33
			b1 = _mm_unpacklo_epi16(a1, a2); // 10 20 11 21 12 22 13 23

			c0 = _mm_unpacklo_epi16(b0, b1); // 00 10 01 20 02 11 03 21
			c1 = _mm_unpackhi_epi16(b1, b0); // 12 30 22 31 13 32 23 33

			v03 = _mm_extract_epi16(a0, 3);
			v30 = _mm_extract_epi16(a3, 0);

			d0 = _mm_shufflehi_epi16(c0, 0xe1); // 00 10 01 20 11 02 03 21
			d1 = _mm_shufflelo_epi16(c1, 0xb4); // 12 30 31 22 13 32 23 33

			d0 = _mm_insert_epi16(d0, v30, 6); // 00 10 01 20 11 02 30 21
			d1 = _mm_insert_epi16(d1, v03, 1); // 12 03 31 22 13 32 23 33
		}


		//CHKN:  use abs
		// Absolute value (note: _mm_abs_epi16 requires SSSE3)
		s0 = _mm_srai_epi16(d0, 15);
		s1 = _mm_srai_epi16(d1, 15);
		d0 = _mm_sub_epi16(_mm_xor_si128(d0, s0), s0);
		d1 = _mm_sub_epi16(_mm_xor_si128(d1, s1), s1);

		z0 = _mm_packs_epi16(d0, d1);
		z1 = _mm_cmpgt_epi8(z0, _mm_set1_epi8(1));
		z0 = _mm_cmpeq_epi8(z0, _mm_setzero_si128());


		sigmap = _mm_movemask_epi8(z0) ^ 0xffff;

		linearCoeff[0] = d0;
		linearCoeff[1] = d1;

		greaterThan1Map = (EB_U16)_mm_movemask_epi8(z1);

	}

	//-------------------------------------------------------------------------------------------------------------------
	// Obtain the last significant X and Y positions and compute their bit cost

	// subblock position

	posLast = Log2f(sigmap);

	coeffGroupPosition = scans4[0][posLast];

	lastSigYPos = coeffGroupPosition >> 2;
	lastSigXPos = coeffGroupPosition & 3;

	// Add cost of significance map
	sigBitsPtr = (__m128i *)CabacCost->CabacBitsSigV[0];
	cs0 = _mm_loadu_si128(sigBitsPtr);
	cs1 = _mm_loadu_si128(sigBitsPtr + 1);
	cs0 = _mm_xor_si128(cs0, _mm_and_si128(cs1, z0));

	// Set bit count to zero for positions that are not coded
	cs0 = _mm_and_si128(cs0, _mm_loadu_si128((__m128i *)(g_mask + 16 - posLast)));

	sigBits = _mm_add_epi64(sigBits, _mm_sad_epu8(cs0, _mm_setzero_si128()));
	// Add significance bits
	sigBits = _mm_add_epi32(sigBits, _mm_shuffle_epi32(sigBits, 0x4e)); // 01001110
	coeffBits += _mm_cvtsi128_si32(sigBits);


	// Encode the position of last significant coefficient
	coeffBits += CabacCost->CabacBitsLast[2 * lastSigXPos + 0];
	coeffBits += CabacCost->CabacBitsLast[2 * lastSigYPos + 1];

	//-------------------------------------------------------------------------------------------------------------------
	// Encode Coefficient levels



	subSetIndex = 0;
	do
	{

        EB_S32 absCoeff[16] = {0}; // Array containing list of nonzero coefficients (size given by numNonZero)		
		EB_S32 index;
		EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded	

		numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZeroCoeffs);

		coeffBits += ONE_BIT * numNonZeroCoeffs; // Add bits for coeff_sign_flag (all coefficients in subblock)

		if (greaterThan1Map == 0)
		{
			coeffBits += CabacCost->CabacBitsG1x[numNonZeroCoeffs - 1];
			continue;
		}


		sigmap = sigmap << 16;

		EB_S32 subPosition = 15;

		EB_U32 count = 0;
		do
		{
			if (sigmap < 0)
			{
				absCoeff[count++] = linearCoeffBufferPtr[subPosition];
			}
			subPosition--;
			sigmap <<= 1;
		} while (sigmap);


		// Loop over coefficients until base value of Exp-Golomb coding changes
		// Base value changes after either
		// - 8th coefficient
		// - a coefficient larger than 1
		for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for coeff_abs_level_greater1_flag

			coeffBits += CabacCost->CabacBitsG1[2 + symbolValue];


			if (symbolValue)
			{
				symbolValue = absVal > 2;

				// Add bits for coeff_abs_level_greater2_flag

				coeffBits += CabacCost->CabacBitsG2[symbolValue];

				if (symbolValue)
				{
					// Golomb Rice parameter is known to be 0 here
					coeffBits += RemainingCoeffExponentialGolombCodeTempZeroParam(absVal - 3);

				}

				index++;

				// Exit loop early as remaining coefficients are coded differently
				break;
			}


		}

		// Loop over coefficients after first one that was > 1 but before 8th one
		// Base value is know to be equal to 2
		for (; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for >1 flag

			coeffBits += CabacCost->CabacBitsG1[symbolValue];

			if (symbolValue)
			{
				coeffBits += RemainingCoeffExponentialGolombCodeTempZeroParam(absVal - 2);
			}
		}

		// Loop over remaining coefficients (8th and beyond)
		// Base value is known to be equal to 1
		for (; index < (EB_S32)numNonZeroCoeffs; index++)
		{
			EB_S32 absVal = absCoeff[index];

			coeffBits += RemainingCoeffExponentialGolombCodeTempZeroParam(absVal - 1);


		}

	} while (--subSetIndex >= 0);

	// Add local bit counter to global bit counter

	*coeffBitsLong += coeffBits << 10;


	return return_error;
}
EB_ERRORTYPE PmEstimateQuantCoeffChroma_SSE2(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 coeffBits = 0;	
	EB_S32 subSetIndex;
	EB_U32 coeffGroupPosition;	
	EB_U32 posLast;	
	EB_S32 lastSigXPos;
	EB_S32 lastSigYPos;	

	(void)(cabacEncodeCtxPtr);
	(void)(type);
	(void)(intraLumaMode);
	(void)(intraChromaMode);
	(void)componentType;
	(void)size;
	__m128i sigBits = _mm_setzero_si128();
	__m128i cs0, cs1;
	__m128i z0 = _mm_setzero_si128();
	//EB_S32 sigCtx = 0;

	__m128i *sigBitsPtr;


	// Coefficients ordered according to scan order (absolute values)
    __m128i linearCoeff[MAX_TU_SIZE * MAX_TU_SIZE / (sizeof(__m128i) / sizeof(EB_U16))] = {{0}};
	EB_U16 *linearCoeffBufferPtr = (EB_U16 *)linearCoeff;

	EB_U16 greaterThan1Map;




	// DC-only fast track
	if (numNonZeroCoeffs == 1 && coeffBufferPtr[0] != 0)
	{
		EB_S32 absVal;
		EB_U32 symbolValue;


		coeffBits += CabacCost->CabacBitsLast[120 + 0];
		coeffBits += CabacCost->CabacBitsLast[120 + 1];


		absVal = ABS(coeffBufferPtr[0]);
		symbolValue = absVal > 1;


		coeffBits += CabacCost->CabacBitsG1[2 * (16 + 1) + symbolValue];

		if (symbolValue)
		{
			symbolValue = absVal > 2;

			// Add bits for coeff_abs_level_greater2_flag

			coeffBits += CabacCost->CabacBitsG2[2 * (4) + symbolValue];

			if (symbolValue)
			{
				// Golomb Rice parameter is known to be 0 here
				coeffBits += RemainingCoeffExponentialGolombCodeTempZeroParam(absVal - 3);
			}
		}



		coeffBits += ONE_BIT; // Sign bit
		// Add local bit counter to global bit counter


		*coeffBitsLong += coeffBits << 10;

		return return_error;
	}


	//sigCtx = 0;
	//if (isChroma)
	//	sigCtx += 6;



	//-------------------------------------------------------------------------------------------------------------------

	// Loop through subblocks to reorder coefficients according to scan order
	// Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients

	EB_S32 sigmap;

	{


		__m128i z1;
		//		__m128i z2;
		__m128i a0, a1, a2, a3;
		__m128i b0, b1, c0, c1, d0, d1;
		__m128i s0, s1;

		EB_S16 *subblockPtr;

		// determine position of subblock within transform block
		subblockPtr = coeffBufferPtr;

		a0 = _mm_loadl_epi64((__m128i *)(subblockPtr + 0 * coeffStride)); // 00 01 02 03 -- -- -- --
		a1 = _mm_loadl_epi64((__m128i *)(subblockPtr + 1 * coeffStride)); // 10 11 12 13 -- -- -- --
		a2 = _mm_loadl_epi64((__m128i *)(subblockPtr + 2 * coeffStride)); // 20 21 22 23 -- -- -- --
		a3 = _mm_loadl_epi64((__m128i *)(subblockPtr + 3 * coeffStride)); // 30 31 32 33 -- -- -- --

		{
			EB_S32 v03, v30;

			b0 = _mm_unpacklo_epi64(a0, a3); // 00 01 02 03 30 31 32 33
			b1 = _mm_unpacklo_epi16(a1, a2); // 10 20 11 21 12 22 13 23

			c0 = _mm_unpacklo_epi16(b0, b1); // 00 10 01 20 02 11 03 21
			c1 = _mm_unpackhi_epi16(b1, b0); // 12 30 22 31 13 32 23 33

			v03 = _mm_extract_epi16(a0, 3);
			v30 = _mm_extract_epi16(a3, 0);

			d0 = _mm_shufflehi_epi16(c0, 0xe1); // 00 10 01 20 11 02 03 21
			d1 = _mm_shufflelo_epi16(c1, 0xb4); // 12 30 31 22 13 32 23 33

			d0 = _mm_insert_epi16(d0, v30, 6); // 00 10 01 20 11 02 30 21
			d1 = _mm_insert_epi16(d1, v03, 1); // 12 03 31 22 13 32 23 33
		}



		//CHKN:  use abs
		// Absolute value (note: _mm_abs_epi16 requires SSSE3)
		s0 = _mm_srai_epi16(d0, 15);
		s1 = _mm_srai_epi16(d1, 15);
		d0 = _mm_sub_epi16(_mm_xor_si128(d0, s0), s0);
		d1 = _mm_sub_epi16(_mm_xor_si128(d1, s1), s1);

		z0 = _mm_packs_epi16(d0, d1);
		z1 = _mm_cmpgt_epi8(z0, _mm_set1_epi8(1));
		z0 = _mm_cmpeq_epi8(z0, _mm_setzero_si128());


		sigmap = _mm_movemask_epi8(z0) ^ 0xffff;

		linearCoeff[0] = d0;
		linearCoeff[1] = d1;

		greaterThan1Map = (EB_U16)_mm_movemask_epi8(z1);

	}

	//-------------------------------------------------------------------------------------------------------------------
	// Obtain the last significant X and Y positions and compute their bit cost

	// subblock position

	posLast = Log2f(sigmap);

	coeffGroupPosition = scans4[0][posLast];

	lastSigYPos = coeffGroupPosition >> 2;
	lastSigXPos = coeffGroupPosition & 3;

	// Add cost of significance map
	sigBitsPtr = (__m128i *)CabacCost->CabacBitsSigV[6];
	cs0 = _mm_loadu_si128(sigBitsPtr);
	cs1 = _mm_loadu_si128(sigBitsPtr + 1);
	cs0 = _mm_xor_si128(cs0, _mm_and_si128(cs1, z0));

	// Set bit count to zero for positions that are not coded
	cs0 = _mm_and_si128(cs0, _mm_loadu_si128((__m128i *)(g_mask + 16 - posLast)));

	sigBits = _mm_add_epi64(sigBits, _mm_sad_epu8(cs0, _mm_setzero_si128()));
	// Add significance bits
	sigBits = _mm_add_epi32(sigBits, _mm_shuffle_epi32(sigBits, 0x4e)); // 01001110
	coeffBits += _mm_cvtsi128_si32(sigBits);


	// Encode the position of last significant coefficient	

	coeffBits += CabacCost->CabacBitsLast[120 + 2 * lastSigXPos];
	coeffBits += CabacCost->CabacBitsLast[120 + 2 * lastSigYPos + 1];


	//-------------------------------------------------------------------------------------------------------------------
	// Encode Coefficient levels



	subSetIndex = 0;
	do
	{

        EB_S32 absCoeff[16] = {0}; // Array containing list of nonzero coefficients (size given by numNonZero)	
		EB_S32 index;
		//		EB_U32 contextSet;
		EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded


		numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZeroCoeffs);


		coeffBits += ONE_BIT * numNonZeroCoeffs; // Add bits for coeff_sign_flag (all coefficients in subblock)


		if (greaterThan1Map == 0)
		{
			coeffBits += CabacCost->CabacBitsG1x[4 * 16 + numNonZeroCoeffs - 1];
			continue;
		}



		sigmap = sigmap << 16;

		EB_S32 subPosition = 15;

		EB_U32 count = 0;
		do
		{
			if (sigmap < 0)
			{
				absCoeff[count++] = linearCoeffBufferPtr[subPosition];
			}
			subPosition--;
			sigmap <<= 1;
		} while (sigmap);


		// Loop over coefficients until base value of Exp-Golomb coding changes
		// Base value changes after either
		// - 8th coefficient
		// - a coefficient larger than 1
		for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for coeff_abs_level_greater1_flag

			coeffBits += CabacCost->CabacBitsG1[2 * (16 + 1) + symbolValue];


			if (symbolValue)
			{
				symbolValue = absVal > 2;

				// Add bits for coeff_abs_level_greater2_flag

				coeffBits += CabacCost->CabacBitsG2[2 * (4) + symbolValue];

				if (symbolValue)
				{
					// Golomb Rice parameter is known to be 0 here
					coeffBits += RemainingCoeffExponentialGolombCodeTempZeroParam(absVal - 3);

				}

				index++;

				// Exit loop early as remaining coefficients are coded differently
				break;
			}


		}

		// Loop over coefficients after first one that was > 1 but before 8th one
		// Base value is know to be equal to 2
		for (; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for >1 flag

			coeffBits += CabacCost->CabacBitsG1[2 * 16 + symbolValue];

			if (symbolValue)
			{
				coeffBits += RemainingCoeffExponentialGolombCodeTempZeroParam(absVal - 2);

			}
		}

		// Loop over remaining coefficients (8th and beyond)
		// Base value is known to be equal to 1
		for (; index < (EB_S32)numNonZeroCoeffs; index++)
		{
			EB_S32 absVal = absCoeff[index];
			coeffBits += RemainingCoeffExponentialGolombCodeTempZeroParam(absVal - 1);

		}

	} while (--subSetIndex >= 0);

	// Add local bit counter to global bit counter

	*coeffBitsLong += coeffBits << 10;


	return return_error;
}

/**********************************************************************
 *
 **********************************************************************/

EB_ERRORTYPE EstimateQuantizedCoefficients_Lossy_SSE2(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong)

{
  EB_ERRORTYPE return_error = EB_ErrorNone;
  
  EB_S32 isChroma = componentType != COMPONENT_LUMA;
  EB_U32 logBlockSize = Log2f(size);
  EB_U32 coeffBits = 0;
  EB_U32 scanIndex = SCAN_DIAG2;
  EB_S32 lastScanSet = -1;
  EB_S32 subSetIndex;
  EB_U32 coeffGroupPosition;
  EB_S32 coeffGroupPositionY;
  EB_S32 coeffGroupPositionX;
  EB_U32 posLast;
  EB_S32 scanPosLast;
  EB_S32 lastSigXPos;
  EB_S32 lastSigYPos;
  EB_U32 contextOffset1;
  

  __m128i sigBits = _mm_setzero_si128();
  __m128i cs0, cs1;
  __m128i z0 = _mm_setzero_si128();
  EB_S32 sigCtx = 0;
  EB_U8 nnz[ MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4) ];
  __m128i *sigBitsPtr;
  
  // Coefficients ordered according to scan order (absolute values)
  __m128i linearCoeff[ MAX_TU_SIZE * MAX_TU_SIZE / (sizeof(__m128i) / sizeof(EB_U16)) ];
  EB_U16 *linearCoeffBufferPtr = (EB_U16 *)linearCoeff;
  EB_U16 greaterThan1Map[ MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4) ] ={0};
  
  // Significance map for each 4x4 subblock
  // 1 bit per coefficient
  // i-th bit corresponds to i-th coefficient in forward scan order
  EB_U16 subblockSigmap[ MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4) ];


  // DC-only fast track
  if (numNonZeroCoeffs == 1 && coeffBufferPtr[ 0 ] != 0)
  {
    EB_S32 absVal;
    EB_U32 symbolValue;
    EB_U32 contextOffset;
    EB_U32 contextOffset2;

    coeffBits += EncodeLastSignificantXYTemp(CabacCost,cabacEncodeCtxPtr, 0, 0, size, isChroma);
    
    absVal = ABS(coeffBufferPtr[ 0 ]);
    symbolValue =  absVal > 1;
    contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS;
    contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS;

    // Add bits for coeff_abs_level_greater1_flag
    coeffBits += CabacCost->CabacBitsG1[2*(contextOffset+1) + symbolValue];
    
    if (symbolValue)
    {
      symbolValue = absVal > 2;
      
      // Add bits for coeff_abs_level_greater2_flag
      coeffBits += CabacCost->CabacBitsG2[2*(contextOffset2) + symbolValue];
      if (symbolValue)
      {
        // Golomb Rice parameter is known to be 0 here
        coeffBits += RemainingCoeffExponentialGolombCodeTemp(absVal - 3, 0);
      }
    }
    
    coeffBits += ONE_BIT; // Sign bit
    // Add local bit counter to global bit counter
    
    *coeffBitsLong += coeffBits << 10;
    return return_error;
  }


  // Compute the scanning type
  // optimized this if statement later
  
  if (type == INTRA_MODE)
  {
    // The test on partition size should be commented out to get conformance for Intra 4x4 !
    //if (partitionSize == SIZE_2Nx2N)
    {
      // note that for Intra 2Nx2N, each CU is one PU. So this mode is the same as
      // the mode of upper-left corner of current CU
      //intraLumaMode   = candidatePtr->intraLumaMode[0];
      //intraChromaMode = candidatePtr->intraChromaMode[0];
      
      if (logBlockSize <= (EB_U32)(3 - isChroma))
      {
        EB_U32 tempIntraChromaMode = chromaMappingTable[ intraChromaMode ];
        EB_S32 intraMode = (!isChroma || tempIntraChromaMode == EB_INTRA_CHROMA_DM) ? intraLumaMode : tempIntraChromaMode;
        
        if (ABS(8 - ((intraMode - 2) & 15)) <= 4)
        {
          scanIndex = (intraMode & 16) ? SCAN_HOR2 : SCAN_VER2;
        }
      }
    }
  }
  
  if (logBlockSize == 2)
  {
    sigCtx = 2 * scanIndex; // 6 values
    if (isChroma) sigCtx += 6;
  }
  else
  {
    sigCtx = 12;
    if (isChroma) sigCtx += 12;
    
    if (logBlockSize != 3)
    {
      sigCtx += 4;
    }
    else if (scanIndex != SCAN_DIAG2)
    {
      sigCtx += 8;
    }
  }
  
  //-------------------------------------------------------------------------------------------------------------------
  
  // Loop through subblocks to reorder coefficients according to scan order
  // Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients
  subSetIndex = 0;
  while (1)
  {
    __m128i z1;
    __m128i z2;
    __m128i a0, a1, a2, a3;
    __m128i b0, b1, c0, c1, d0, d1;
    __m128i s0, s1;
    EB_S32 sigmap;
    EB_S16 *subblockPtr;
    
    // determine position of subblock within transform block
    coeffGroupPosition = sbScans[ logBlockSize-2 ][ subSetIndex ];
    coeffGroupPositionY = coeffGroupPosition >> 4;
    coeffGroupPositionX = coeffGroupPosition & 15;
    
    if (scanIndex == SCAN_HOR2)
    {
      // Subblock scan is mirrored for horizontal scan
      SWAP(coeffGroupPositionX, coeffGroupPositionY);
    }
    
    /*EB_S16 **/subblockPtr = coeffBufferPtr + 4 * coeffGroupPositionY * coeffStride + 4 * coeffGroupPositionX;
    
    a0 = _mm_loadl_epi64((__m128i *)(subblockPtr + 0 * coeffStride)); // 00 01 02 03 -- -- -- --
    a1 = _mm_loadl_epi64((__m128i *)(subblockPtr + 1 * coeffStride)); // 10 11 12 13 -- -- -- --
    a2 = _mm_loadl_epi64((__m128i *)(subblockPtr + 2 * coeffStride)); // 20 21 22 23 -- -- -- --
    a3 = _mm_loadl_epi64((__m128i *)(subblockPtr + 3 * coeffStride)); // 30 31 32 33 -- -- -- --
    
    if (scanIndex == SCAN_DIAG2)
    {
      EB_S32 v03, v30;
      
      b0 = _mm_unpacklo_epi64(a0, a3); // 00 01 02 03 30 31 32 33
      b1 = _mm_unpacklo_epi16(a1, a2); // 10 20 11 21 12 22 13 23
      
      c0 = _mm_unpacklo_epi16(b0, b1); // 00 10 01 20 02 11 03 21
      c1 = _mm_unpackhi_epi16(b1, b0); // 12 30 22 31 13 32 23 33
      
      v03 = _mm_extract_epi16(a0, 3);
      v30 = _mm_extract_epi16(a3, 0);
      
      d0 = _mm_shufflehi_epi16(c0, 0xe1); // 00 10 01 20 11 02 03 21
      d1 = _mm_shufflelo_epi16(c1, 0xb4); // 12 30 31 22 13 32 23 33
      
      d0 = _mm_insert_epi16(d0, v30, 6); // 00 10 01 20 11 02 30 21
      d1 = _mm_insert_epi16(d1, v03, 1); // 12 03 31 22 13 32 23 33
    }
    else if (scanIndex == SCAN_HOR2)
    {
      d0 = _mm_unpacklo_epi64(a0, a1); // 00 01 02 03 10 11 12 13
      d1 = _mm_unpacklo_epi64(a2, a3); // 20 21 22 23 30 31 32 33
    }
    else
    {
      b0 = _mm_unpacklo_epi16(a0, a2); // 00 20 01 21 02 22 03 23
      b1 = _mm_unpacklo_epi16(a1, a3); // 10 30 11 31 12 32 13 33
      
      d0 = _mm_unpacklo_epi16(b0, b1); // 00 10 20 30 01 11 21 31
      d1 = _mm_unpackhi_epi16(b0, b1); // 02 12 22 32 03 13 23 33
    }
    
    // Absolute value (note: _mm_abs_epi16 requires SSSE3)
    s0 = _mm_srai_epi16(d0, 15);
    s1 = _mm_srai_epi16(d1, 15);
    d0 = _mm_sub_epi16(_mm_xor_si128(d0, s0), s0);
    d1 = _mm_sub_epi16(_mm_xor_si128(d1, s1), s1);
    z0 = _mm_packs_epi16(d0, d1);
    z1 = _mm_cmpgt_epi8(z0, _mm_set1_epi8(1));
    z0 = _mm_cmpeq_epi8(z0, _mm_setzero_si128());
    
    sigmap = _mm_movemask_epi8(z0) ^ 0xffff;
    subblockSigmap[ subSetIndex ] = (EB_U16)sigmap;
    
    if (sigmap != 0)
    {
      EB_U32 num;
      
      lastScanSet = subSetIndex;
      linearCoeff[ 2 * subSetIndex + 0 ] = d0;
      linearCoeff[ 2 * subSetIndex + 1 ] = d1;
      
      greaterThan1Map[ subSetIndex ] = (EB_U16) _mm_movemask_epi8(z1);
      
      // Count number of bits set in sigmap (Hamming weight)
      z2 = _mm_sad_epu8(z0, _mm_setzero_si128());
      
      num = 16;
      num += _mm_cvtsi128_si32(z2);
      num += _mm_extract_epi16(z2, 4);
      // num is 255*(16-numCoeff) or 256*16-16-256*numCoeff+numCoeff
      num &= 0x1f;
      nnz[subSetIndex] = (EB_U8) num;
      numNonZeroCoeffs -= num;
      if (numNonZeroCoeffs == 0)
      {
        break;
      }
    }
    else
    {
      nnz[subSetIndex] = 0;
    }
    
    if (sigmap != 0 || subSetIndex == 0)
    {
      // Add cost of significance map
      sigBitsPtr = (__m128i *)CabacCost->CabacBitsSigV[sigCtx];
      cs0 = _mm_loadu_si128(sigBitsPtr);
      cs1 = _mm_loadu_si128(sigBitsPtr+1);
      cs0 = _mm_xor_si128(cs0, _mm_and_si128(cs1, z0));
      if (sigmap == 1 && subSetIndex != 0)
      {
        cs0 = _mm_and_si128(cs0, _mm_setr_epi8(0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1));
      }
      sigBits = _mm_add_epi64(sigBits, _mm_sad_epu8(cs0, _mm_setzero_si128()));
    }
    sigCtx |= 2; // differentiate between first and other blocks because of DC
    subSetIndex++;
    
    // Check that we are not going beyond block end
  }

  //-------------------------------------------------------------------------------------------------------------------
  // Obtain the last significant X and Y positions and compute their bit cost
  
  // subblock position
  coeffGroupPosition = sbScans[ logBlockSize - 2 ][ lastScanSet ];
  coeffGroupPositionY = coeffGroupPosition >> 4;
  coeffGroupPositionX = coeffGroupPosition & 15;
  lastSigYPos = 4 * coeffGroupPositionY;
  lastSigXPos = 4 * coeffGroupPositionX;
  scanPosLast = 16 * lastScanSet;
  
  // position within subblock
  posLast = Log2f(subblockSigmap[ lastScanSet ]);
  coeffGroupPosition = scans4[ scanIndex != SCAN_DIAG2 ][ posLast ];
  lastSigYPos += coeffGroupPosition >> 2;
  lastSigXPos += coeffGroupPosition & 3;
  scanPosLast += posLast;
  
  // Add cost of significance map
  sigBitsPtr = (__m128i *)CabacCost->CabacBitsSigV[sigCtx];
  cs0 = _mm_loadu_si128(sigBitsPtr);
  cs1 = _mm_loadu_si128(sigBitsPtr+1);
  cs0 = _mm_xor_si128(cs0, _mm_and_si128(cs1, z0));
  
  // Set bit count to zero for positions that are not coded
  cs0 = _mm_and_si128(cs0, _mm_loadu_si128((__m128i *)(g_mask+16-posLast)));
  
  sigBits = _mm_add_epi64(sigBits, _mm_sad_epu8(cs0, _mm_setzero_si128()));
  // Add significance bits
  sigBits = _mm_add_epi32(sigBits, _mm_shuffle_epi32(sigBits, 0x4e)); // 01001110
  coeffBits += _mm_cvtsi128_si32(sigBits);
  
  // Should swap row/col for SCAN_HOR and SCAN_VER:
  // - SCAN_HOR because the subscan order is mirrored (compared to SCAN_DIAG)
  // - SCAN_VER because of equation (7-66) in H.265 (04/2013)
  // Note that the scans4 array is adjusted to reflect this
  if (scanIndex != SCAN_DIAG2)
  {
    SWAP(lastSigXPos, lastSigYPos);
  }
  
  // Encode the position of last significant coefficient
  coeffBits += EncodeLastSignificantXYTemp(CabacCost,cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, isChroma);
  
  //-------------------------------------------------------------------------------------------------------------------
  // Encode Coefficient levels
  
  contextOffset1 = 1;
  
  // Loop over subblocks
  subSetIndex = lastScanSet;
  do
  {
    EB_S32 numNonZero = 0; // Number of nonzero coefficients in current subblock
	EB_S32 absCoeff[16] = { 0 }; // Array containing list of nonzero coefficients (size given by numNonZero)
    EB_U32 golombRiceParam = 0;
    EB_S32 index;
    EB_U32 contextSet;
    EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded
    EB_U32 contextOffset;
    EB_U32 contextOffset2;
    
    // 1. Subblock-level significance flag
        
    if (subSetIndex != 0)
    {
      
      if (subSetIndex != lastScanSet)
      {
        EB_U32 sigCoeffGroupContextIndex;
        EB_U32 significanceFlag;

        sigCoeffGroupContextIndex = 0;
        sigCoeffGroupContextIndex += isChroma * NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS;
        
        significanceFlag = (subblockSigmap[ subSetIndex ] != 0);
        
        // Add bits for coded_sub_block_flag
        coeffBits += CabacCost->CabacBitsSigMl[2*sigCoeffGroupContextIndex+significanceFlag];
        
        if (!significanceFlag)
        {
          // Nothing else to do for this subblock since all coefficients in it are zero
          continue;
        }
      }

    }
    
   
    // 3. Coefficient level values
    
    contextSet = (subSetIndex != 0 && !isChroma) ? 2 : 0;

    contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS + 4 * contextSet;
    contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS + contextSet;
    
    numNonZero = nnz[ subSetIndex ];
    numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZero);

	{
		coeffBits += ONE_BIT * numNonZero; // Add bits for coeff_sign_flag (all coefficients in subblock)
	}

    if (greaterThan1Map[subSetIndex] == 0)
    {
      if (numNonZero > 0)
      {
        coeffBits += CabacCost->CabacBitsG1x[4*contextOffset+numNonZero-1];
      }
      continue;
    }
    {
      EB_S32 sigMap = subblockSigmap[ subSetIndex ] << 16;
      EB_S32 subPosition = (subSetIndex << LOG2_SCAN_SET_SIZE) + 15;
      EB_U32 count = 0;
      do
      {
        if (sigMap < 0)
        {
          absCoeff[count++] = linearCoeffBufferPtr[subPosition];
        }
        subPosition--;
        sigMap <<= 1;
      }
      while (sigMap);
    }
    
    // Loop over coefficients until base value of Exp-Golomb coding changes
    // Base value changes after either
    // - 8th coefficient
    // - a coefficient larger than 1
    for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
    {
        if (index < 16) {
            EB_S32 absVal = absCoeff[index];
            EB_U32 symbolValue = absVal > 1;

            // Add bits for coeff_abs_level_greater1_flag
            coeffBits += CabacCost->CabacBitsG1[2 * (contextOffset + contextOffset1) + symbolValue];

            if (symbolValue)
            {
                symbolValue = absVal > 2;

                // Add bits for coeff_abs_level_greater2_flag
                coeffBits += CabacCost->CabacBitsG2[2 * (contextOffset2)+symbolValue];
                if (symbolValue)
                {
                    // Golomb Rice parameter is known to be 0 here
                    coeffBits += RemainingCoeffExponentialGolombCodeTemp(absVal - 3, 0);
                }

                index++;
                // Exit loop early as remaining coefficients are coded differently
                break;
            }
        }
    }
    
    // Loop over coefficients after first one that was > 1 but before 8th one
    // Base value is know to be equal to 2
    for ( ; index < numCoeffWithCodedGt1Flag; index++)
    {
      EB_S32 absVal = absCoeff[ index ];
      EB_U32 symbolValue = absVal > 1;
      
      // Add bits for >1 flag
      coeffBits += CabacCost->CabacBitsG1[2*contextOffset + symbolValue];
      if (symbolValue)
      {
        coeffBits += RemainingCoeffExponentialGolombCodeTemp(absVal - 2, golombRiceParam);

      }
    }
    
    // Loop over remaining coefficients (8th and beyond)
    // Base value is known to be equal to 1
    for ( ; index < numNonZero; index++ )
    {
        if (index < 16) {
            EB_S32 absVal = absCoeff[index];
            coeffBits += RemainingCoeffExponentialGolombCodeTemp(absVal - 1, golombRiceParam);
        }
    }
  }
  while (--subSetIndex >= 0);
  
  // Add local bit counter to global bit counter
  *coeffBitsLong += coeffBits << 10;
  
  return return_error;
}


EB_ERRORTYPE EstimateQuantizedCoefficients_Lossy(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong)
{
  EB_ERRORTYPE return_error = EB_ErrorNone;
  
  EB_S32 isChroma = componentType != COMPONENT_LUMA;
  EB_U32 logBlockSize = Log2f(size);
  EB_U32 coeffBits = 0;
  EB_U32 scanIndex = SCAN_DIAG2;
  EB_S32 lastScanSet = -1;
  EB_S32 subSetIndex;
  EB_U32 coeffGroupPosition;
  EB_S32 coeffGroupPositionY;
  EB_S32 coeffGroupPositionX;
  EB_U32 posLast;
  EB_S32 scanPosLast;
  EB_S32 lastSigXPos;
  EB_S32 lastSigYPos;
  EB_U32 significantFlagContextOffset;
  EB_U32 contextOffset1;
    
  // Coefficients ordered according to scan order (absolute values)
  EB_U16 linearCoeff[ MAX_TU_SIZE * MAX_TU_SIZE ];
  EB_U16 *linearCoeffBufferPtr = (EB_U16 *)linearCoeff;
  EB_U16 greaterThan1Map[ MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4) ];
  
  // Significance map for each 4x4 subblock
  // 1 bit per coefficient
  // i-th bit corresponds to i-th coefficient in forward scan order
  EB_U16 subblockSigmap[ MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4) ];
  
  // DC-only fast track
  if (numNonZeroCoeffs == 1 && coeffBufferPtr[ 0 ] != 0)
  {
    EB_S32 absVal;
    EB_U32 symbolValue;
    EB_U32 contextOffset;
    EB_U32 contextOffset2;
    
    coeffBits += EncodeLastSignificantXYTemp(CabacCost,cabacEncodeCtxPtr, 0, 0, size, isChroma);
    
    absVal = ABS(coeffBufferPtr[ 0 ]);
    symbolValue =  absVal > 1;
    contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS;
    contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS;
    
    // Add bits for coeff_abs_level_greater1_flag
    coeffBits += CabacCost->CabacBitsG1[2*(contextOffset+1) + symbolValue];
    
    if (symbolValue)
    {
      symbolValue = absVal > 2;
      
      // Add bits for coeff_abs_level_greater2_flag
      coeffBits += CabacCost->CabacBitsG2[2*(contextOffset2) + symbolValue];

      if (symbolValue)
      {
        // Golomb Rice parameter is known to be 0 here
        coeffBits += RemainingCoeffExponentialGolombCodeTemp(absVal - 3, 0);
      }
    }
    
    coeffBits += ONE_BIT; // Sign bit
    // Add local bit counter to global bit counter
    
    *coeffBitsLong += coeffBits << 10;
    return return_error;
  }
  
  // Compute the scanning type
  // optimized this if statement later
  
  if (type == INTRA_MODE)
  {
    // The test on partition size should be commented out to get conformance for Intra 4x4 !
    //if (partitionSize == SIZE_2Nx2N)
    {
      // note that for Intra 2Nx2N, each CU is one PU. So this mode is the same as
      // the mode of upper-left corner of current CU
      //intraLumaMode   = candidatePtr->intraLumaMode[0];
      //intraChromaMode = candidatePtr->intraChromaMode[0];
      
      if (logBlockSize <= (EB_U32)(3 - isChroma))
      {
        EB_U32 tempIntraChromaMode = chromaMappingTable[ intraChromaMode ];
        EB_S32 intraMode = (!isChroma || tempIntraChromaMode == EB_INTRA_CHROMA_DM) ? intraLumaMode : tempIntraChromaMode;
        
        if (ABS(8 - ((intraMode - 2) & 15)) <= 4)
        {
          scanIndex = (intraMode & 16) ? SCAN_HOR2 : SCAN_VER2;
        }
      }
    }
  }
  
  //-------------------------------------------------------------------------------------------------------------------
  
  // Loop through subblocks to reorder coefficients according to scan order
  // Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients
  subSetIndex = 0;
  while (1)
  {
	EB_S16 *subblockPtr;  
    EB_S16 k;
    EB_S32 sigmap = 0;
    EB_U32 num = 0;
    EB_S32 g1map = 0;
    // determine position of subblock within transform block
    coeffGroupPosition = sbScans[ logBlockSize-2 ][ subSetIndex ];
    coeffGroupPositionY = coeffGroupPosition >> 4;
    coeffGroupPositionX = coeffGroupPosition & 15;
    
    if (scanIndex == SCAN_HOR2)
    {
      // Subblock scan is mirrored for horizontal scan
      SWAP(coeffGroupPositionX, coeffGroupPositionY);
    }
    
    subblockPtr = coeffBufferPtr + 4 * coeffGroupPositionY * coeffStride + 4 * coeffGroupPositionX;
    
    for (k = 0; k < 16; k++)
    {
	  EB_U32 val , isNonZero;
      EB_U32 position = scans4[ scanIndex != SCAN_DIAG2 ][ k ];
      EB_U32 positionY = position >> 2;
      EB_U32 positionX = position & 3;
      if (scanIndex == SCAN_HOR2)
      {
        // Subblock scan is mirrored for horizontal scan
        SWAP(positionX, positionY);
      }
      
      /*EB_U32*/ val = ABS(subblockPtr[ coeffStride * positionY + positionX ]);
      linearCoeff[ 16 * subSetIndex + k ] = (EB_U16) val;
      /*EB_U32 */isNonZero = val != 0;
      num += isNonZero;
      sigmap |= isNonZero << k;
      if (val > 1)
      {
        g1map |= 1 << k;
      }
    }
    
    subblockSigmap[ subSetIndex ] = (EB_U16) sigmap;
    greaterThan1Map[ subSetIndex ] = (EB_U16) g1map;

    if (sigmap != 0)
    {
      lastScanSet = subSetIndex;
      
      numNonZeroCoeffs -= num;
      if (numNonZeroCoeffs == 0)
      {
        break;
      }
    }
    
    subSetIndex++;
    
    // Check that we are not going beyond block end
  }
  
  //-------------------------------------------------------------------------------------------------------------------
  // Obtain the last significant X and Y positions and compute their bit cost
  
  // subblock position
  coeffGroupPosition = sbScans[ logBlockSize - 2 ][ lastScanSet ];
  coeffGroupPositionY = coeffGroupPosition >> 4;
  coeffGroupPositionX = coeffGroupPosition & 15;
  lastSigYPos = 4 * coeffGroupPositionY;
  lastSigXPos = 4 * coeffGroupPositionX;
  scanPosLast = 16 * lastScanSet;
  
  // position within subblock
  posLast = Log2f(subblockSigmap[ lastScanSet ]);
  coeffGroupPosition = scans4[ scanIndex != SCAN_DIAG2 ][ posLast ];
  lastSigYPos += coeffGroupPosition >> 2;
  lastSigXPos += coeffGroupPosition & 3;
  scanPosLast += posLast;
  
  // Should swap row/col for SCAN_HOR and SCAN_VER:
  // - SCAN_HOR because the subscan order is mirrored (compared to SCAN_DIAG)
  // - SCAN_VER because of equation (7-66) in H.265 (04/2013)
  // Note that the scans4 array is adjusted to reflect this
  if (scanIndex != SCAN_DIAG2)
  {
    SWAP(lastSigXPos, lastSigYPos);
  }
  
  // Encode the position of last significant coefficient
  coeffBits += EncodeLastSignificantXYTemp(CabacCost,cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, isChroma);
  
  //-------------------------------------------------------------------------------------------------------------------
  // Encode Coefficient levels
  
  significantFlagContextOffset = (!isChroma)? 0 : NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS;
  
  contextOffset1 = 1;
    
  // Loop over subblocks
  subSetIndex = lastScanSet;
  do
  {
    EB_S32 numNonZero = 0; // Number of nonzero coefficients in current subblock
	EB_S32 absCoeff[16] = { 0 }; // Array containing list of nonzero coefficients (size given by numNonZero)
    EB_U32 golombRiceParam = 0;
    EB_S32 index;
    EB_U32 contextSet;
    EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded
    EB_U32 contextOffset;
    EB_U32 contextOffset2;
    
    // 1. Subblock-level significance flag
    
    if (subSetIndex != 0)
    {
      
      if (subSetIndex != lastScanSet)
      {
        EB_U32 sigCoeffGroupContextIndex;
        EB_U32 significanceFlag;
        
        sigCoeffGroupContextIndex = 0;
        sigCoeffGroupContextIndex += isChroma * NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS;
        
        significanceFlag = (subblockSigmap[ subSetIndex ] != 0);
        
        // Add bits for coded_sub_block_flag
        coeffBits += CabacCost->CabacBitsSigMl[2*sigCoeffGroupContextIndex+significanceFlag];
        
        if (!significanceFlag)
        {
          // Nothing else to do for this subblock since all coefficients in it are zero
          continue;
        }
      }
    }
    
    // 2. Coefficient significance flags
    
    // Use do {} while(0) loop to avoid goto statement (early exit)
    do
    {
      EB_S32 scanPosSig;
      EB_S32 sigMap = subblockSigmap[ subSetIndex ];
      EB_S32 subPosition = subSetIndex << LOG2_SCAN_SET_SIZE;
      EB_S32 subPosition2 = subPosition;
      EB_U32 tempOffset;
      const EB_U8 *contextIndexMapPtr;
      EB_U8 *bitsPtr;
      
      if ( subSetIndex == lastScanSet )
      {
        absCoeff[ 0 ] = linearCoeffBufferPtr[ scanPosLast ];
        numNonZero = 1;
        if (sigMap == 1)
        {
          // Nothing else to do here (only "DC" coeff within subblock)
          break;
        }
        scanPosSig = scanPosLast - 1;
        sigMap <<= 31 - (scanPosSig & 15);
      }
      else
      {
        if (sigMap == 1 && subSetIndex != 0)
        {
          subPosition2++;
          absCoeff[ 0 ] = linearCoeffBufferPtr[ subPosition ];
          numNonZero = 1;
        }
        scanPosSig = subPosition + 15;
        sigMap <<= 16;
      }
      
      if (subSetIndex == 0)
      {
        subPosition2 = 1;
      }
      
      if (logBlockSize == 2)
      {
        tempOffset = 0;
        contextIndexMapPtr = contextIndexMap4[ scanIndex ];
      }
      else
      {
        tempOffset = (logBlockSize == 3) ? (scanIndex == SCAN_DIAG2 ? 9 : 15) : (!isChroma ? 21 : 12);
        tempOffset += (!isChroma && subSetIndex != 0 ) ? 3 : 0;
        contextIndexMapPtr = contextIndexMap8[ scanIndex != SCAN_DIAG2 ][ 0 ] - subPosition;
      }
      
      bitsPtr = CabacCost->CabacBitsSig + 2 * significantFlagContextOffset;
      bitsPtr += 2 * tempOffset;

      // Loop over coefficients
      while (scanPosSig >= subPosition2)
        {
          EB_U32 sigContextIndex;
          EB_S32 sigCoeffFlag = sigMap < 0;
          
          sigContextIndex = contextIndexMapPtr[ scanPosSig ];
          
          // Add bits for sig_coeff_flag
          coeffBits += bitsPtr[2 * sigContextIndex + sigCoeffFlag];

          if (sigCoeffFlag)
          {
            absCoeff[ numNonZero ] = linearCoeffBufferPtr[ scanPosSig ];
            numNonZero++;
          }

          sigMap <<= 1;
          scanPosSig--;
        }
      
      if (scanPosSig == 0)
      {
        EB_S32 sigCoeffFlag = sigMap < 0;
        
        coeffBits += CabacCost->CabacBitsSig[ 2 * significantFlagContextOffset + sigCoeffFlag ];

        if (sigCoeffFlag)
        {
          absCoeff[ numNonZero ] = linearCoeffBufferPtr[ scanPosSig ];
          numNonZero++;
        }
      }
    }
    while (0);
    
    // 3. Coefficient level values
    
    contextSet = (subSetIndex != 0 && !isChroma) ? 2 : 0;
    contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS + 4 * contextSet;
    contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS + contextSet;
    
    numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZero);
    
    {
		coeffBits += ONE_BIT * numNonZero; // Add bits for coeff_sign_flag (all coefficients in subblock)
	}

    if (greaterThan1Map[subSetIndex] == 0)
    {
      if (numNonZero > 0)
      {
        coeffBits += CabacCost->CabacBitsG1x[4*contextOffset+numNonZero-1];
      }
      continue;
    }
    
    // Loop over coefficients until base value of Exp-Golomb coding changes
    // Base value changes after either
    // - 8th coefficient
    // - a coefficient larger than 1
    for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
    {
      EB_S32 absVal = absCoeff[ index ];
      EB_U32 symbolValue =  absVal > 1;
      
      // Add bits for coeff_abs_level_greater1_flag
      coeffBits += CabacCost->CabacBitsG1[2*(contextOffset+contextOffset1) + symbolValue];
      
      if (symbolValue)
      {
        symbolValue = absVal > 2;
        
        // Add bits for coeff_abs_level_greater2_flag
        coeffBits += CabacCost->CabacBitsG2[2*(contextOffset2) + symbolValue];

        if (symbolValue)
        {
          // Golomb Rice parameter is known to be 0 here
          coeffBits += RemainingCoeffExponentialGolombCodeTemp(absVal - 3, 0);
        }
        
        index++;

        // Exit loop early as remaining coefficients are coded differently
        break;
      }
    }
    
    // Loop over coefficients after first one that was > 1 but before 8th one
    // Base value is know to be equal to 2
    for ( ; index < numCoeffWithCodedGt1Flag; index++)
    {
      EB_S32 absVal = absCoeff[ index ];
      EB_U32 symbolValue = absVal > 1;
      
      // Add bits for >1 flag
      coeffBits += CabacCost->CabacBitsG1[2*contextOffset + symbolValue];

      if (symbolValue)
      {
        coeffBits += RemainingCoeffExponentialGolombCodeTemp(absVal - 2, golombRiceParam);
        
      }
    }
    
    // Loop over remaining coefficients (8th and beyond)
    // Base value is known to be equal to 1
    for ( ; index < numNonZero; index++ )
    {
      EB_S32 absVal = absCoeff[ index ];
      coeffBits += RemainingCoeffExponentialGolombCodeTemp(absVal - 1, golombRiceParam);

    }
  }
  while (--subSetIndex >= 0);
  
  // Add local bit counter to global bit counter
  *coeffBitsLong += coeffBits << 10;
  
  return return_error;
}
