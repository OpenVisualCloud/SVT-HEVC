/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbEntropyCodingUtil.h"
#include "EbDefinitions.h"

#define ONE_BIT 32768

//LUT used for LPSxRange calculation
//8 bit unsigned is enough.
static const EB_U32 RangeLPSTable1D[] =
{
    128, 176, 208, 240,
    128, 167, 197, 227,
    128, 158, 187, 216,
    123, 150, 178, 205,
    116, 142, 169, 195,
    111, 135, 160, 185,
    105, 128, 152, 175,
    100, 122, 144, 166,
    95, 116, 137, 158,
    90, 110, 130, 150,
    85, 104, 123, 142,
    81, 99, 117, 135,
    77, 94, 111, 128,
    73, 89, 105, 122,
    69, 85, 100, 116,
    66, 80, 95, 110,
    62, 76, 90, 104,
    59, 72, 86, 99,
    56, 69, 81, 94,
    53, 65, 77, 89,
    51, 62, 73, 85,
    48, 59, 69, 80,
    46, 56, 66, 76,
    43, 53, 63, 72,
    41, 50, 59, 69,
    39, 48, 56, 65,
    37, 45, 54, 62,
    35, 43, 51, 59,
    33, 41, 48, 56,
    32, 39, 46, 53,
    30, 37, 43, 50,
    29, 35, 41, 48,
    27, 33, 39, 45,
    26, 31, 37, 43,
    24, 30, 35, 41,
    23, 28, 33, 39,
    22, 27, 32, 37,
    21, 26, 30, 35,
    20, 24, 29, 33,
    19, 23, 27, 31,
    18, 22, 26, 30,
    17, 21, 25, 28,
    16, 20, 23, 27,
    15, 19, 22, 25,
    14, 18, 21, 24,
    14, 17, 20, 23,
    13, 16, 19, 22,
    12, 15, 18, 21,
    12, 14, 17, 20,
    11, 14, 16, 19,
    11, 13, 15, 18,
    10, 12, 15, 17,
    10, 12, 14, 16,
    9, 11, 13, 15,
    9, 11, 12, 14,
    8, 10, 12, 14,
    8, 9, 11, 13,
    7, 9, 11, 12,
    7, 9, 10, 12,
    7, 8, 10, 11,
    6, 8, 9, 11,
    6, 7, 9, 10,
    6, 7, 8, 9,
    2, 2, 2, 2
};

//Table for calculating the number of shifts. The max number of shifts is 6.
static const EB_U32 ShiftNumTable[] =
{
    6, 5, 4, 4,
    3, 3, 3, 3,
    2, 2, 2, 2,
    2, 2, 2, 2,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};

/************************************************
* Bac Encoder Context:WriteOut Function
* Write a Byte or more, which won�t change because of carry any more
*
* input: bacEncContext pointer
*
************************************************/
void WriteOut(BacEncContext_t *bacEncContextPtr)
{
    if (bacEncContextPtr->bitsRemainingNum < 12)
    {
        EB_U32 carry;
        EB_U32 nextByteToWrite;

        bacEncContextPtr->bitsRemainingNum += 8;
        nextByteToWrite = bacEncContextPtr->intervalLowValue >> (32 - bacEncContextPtr->bitsRemainingNum);
        carry = nextByteToWrite >> 8;
        bacEncContextPtr->intervalLowValue &= 0xffffffffu >> bacEncContextPtr->bitsRemainingNum;

        if (nextByteToWrite == 0xff)
        {
            bacEncContextPtr->tempBufferedBytesNum++;
        }
        else
        {
            if (bacEncContextPtr->tempBufferedBytesNum > 0)
            {
                OutputBitstreamWriteByte(bacEncContextPtr->m_pcTComBitIf, bacEncContextPtr->tempBufferedByte + carry);
                bacEncContextPtr->tempBufferedByte = nextByteToWrite & 0xff;
                while (bacEncContextPtr->tempBufferedBytesNum > 1)
                {
                    bacEncContextPtr->tempBufferedBytesNum--;
                    OutputBitstreamWriteByte(bacEncContextPtr->m_pcTComBitIf, (0xff + carry) & 0xff);
                }
            }
            else
            {
                bacEncContextPtr->tempBufferedBytesNum = 1;
                bacEncContextPtr->tempBufferedByte = nextByteToWrite;
            }
        }
    }
}

/************************************************
* Bac Encoder Context: Encode One Bin Function
*
* input: bacEncContext pointer
* input: BinaryValue
* input: contextModelPtr
*
************************************************/
void EncodeOneBin(
    BacEncContext_t *bacEncContextPtr,
    const EB_U32     BinaryValue,
    EB_ContextModel *contextModelPtr){

    EB_U32 lowProbInterval;
    EB_U32 tempCond;
    EB_U32 shiftNum;


    lowProbInterval = RangeLPSTable1D[(((*contextModelPtr) >> 1) << 2) + ((bacEncContextPtr->intervalRangeValue >> 6) & 3)];
    tempCond = BinaryValue ^ (*contextModelPtr & 1);//^ can be replace by !=
    *contextModelPtr = NextStateMpsLps[(tempCond << 7) | (*contextModelPtr)];//| can be replace by +
    bacEncContextPtr->intervalRangeValue -= lowProbInterval;

    if (tempCond)
    {
        bacEncContextPtr->intervalLowValue += bacEncContextPtr->intervalRangeValue;
        bacEncContextPtr->intervalRangeValue = lowProbInterval;
        shiftNum = ShiftNumTable[lowProbInterval >> 3];
        bacEncContextPtr->intervalLowValue = bacEncContextPtr->intervalLowValue << shiftNum;
        bacEncContextPtr->intervalRangeValue = bacEncContextPtr->intervalRangeValue << shiftNum;
        bacEncContextPtr->bitsRemainingNum -= shiftNum;
        WriteOut(bacEncContextPtr);
    }
    else if (bacEncContextPtr->intervalRangeValue < 256)
    {
        bacEncContextPtr->intervalLowValue = bacEncContextPtr->intervalLowValue << 1;
        bacEncContextPtr->intervalRangeValue = bacEncContextPtr->intervalRangeValue << 1;
        bacEncContextPtr->bitsRemainingNum -= 1;
        WriteOut(bacEncContextPtr);
    }

}

/************************************************
* Bac Encoder Context: Encode Bypass One Bin Function
*
* input: bacEncContext pointer
* input: BinaryValue
*
************************************************/
void EncodeBypassOneBin(
    BacEncContext_t *bacEncContextPtr,
    const EB_U32     BinaryValue)
{
    bacEncContextPtr->intervalLowValue <<= 1;
    bacEncContextPtr->bitsRemainingNum--;
    if (BinaryValue){
        bacEncContextPtr->intervalLowValue += bacEncContextPtr->intervalRangeValue;
    }
    WriteOut(bacEncContextPtr);
}

/************************************************
* Bac Encoder Context: Encode Bypass Bins Function
*
* input: bacEncContext pointer
* input: BinaryValues
* input: binsLength: number of bins
*
************************************************/
void EncodeBypassBins(
    BacEncContext_t *bacEncContextPtr,
    const EB_U32     BinaryValues,
    EB_U32           binsLength)
{
    EB_U32 binsLengthCase;

    binsLengthCase = (binsLength > 16) ? (binsLength > 24 ? 4 : 3) : (binsLength > 8) ? 2 : 1;
    //4 binsLength >23
    //3 24> binsLength >15
    //2 16> binsLength >7
    //1 binsLength <8
    switch (binsLengthCase){
    case 4:// binsLength >24
        binsLength -= 8;
        bacEncContextPtr->bitsRemainingNum -= 8;
        bacEncContextPtr->intervalLowValue <<= 8;
        bacEncContextPtr->intervalLowValue += bacEncContextPtr->intervalRangeValue * ((BinaryValues >> binsLength) & 0xff);
        WriteOut(bacEncContextPtr);

    case 3://24> binsLength >16
        binsLength -= 8;
        bacEncContextPtr->bitsRemainingNum -= 8;
        bacEncContextPtr->intervalLowValue <<= 8;
        bacEncContextPtr->intervalLowValue += bacEncContextPtr->intervalRangeValue * ((BinaryValues >> binsLength) & 0xff);
        WriteOut(bacEncContextPtr);

    case 2://16> binsLength >8
        binsLength -= 8;
        bacEncContextPtr->bitsRemainingNum -= 8;
        bacEncContextPtr->intervalLowValue <<= 8;
        bacEncContextPtr->intervalLowValue += bacEncContextPtr->intervalRangeValue * ((BinaryValues >> binsLength) & 0xff);
        WriteOut(bacEncContextPtr);

    case 1:// binsLength <9
        bacEncContextPtr->intervalLowValue <<= binsLength;
        bacEncContextPtr->intervalLowValue += bacEncContextPtr->intervalRangeValue * (BinaryValues & (0xff >> (8 - binsLength)));
        bacEncContextPtr->bitsRemainingNum -= binsLength;
        WriteOut(bacEncContextPtr);

    default:
        break;
    }
}

/************************************************
* Bac Encoder Context: Encode Terminate Function
*
* input: bacEncContext pointer
* input: BinaryValue
*
************************************************/
void BacEncContextTerminate(
    BacEncContext_t *bacEncContextPtr,
    const EB_U32     BinaryValue)
{
    EB_U32 shiftNum;


    bacEncContextPtr->intervalRangeValue -= 2;
    shiftNum = ShiftNumTable[bacEncContextPtr->intervalRangeValue >> 3];

    if (BinaryValue){
        bacEncContextPtr->intervalLowValue += bacEncContextPtr->intervalRangeValue;
        bacEncContextPtr->intervalRangeValue = 2;
        shiftNum = 7;
    }
    bacEncContextPtr->intervalLowValue = bacEncContextPtr->intervalLowValue << shiftNum;
    bacEncContextPtr->intervalRangeValue = bacEncContextPtr->intervalRangeValue << shiftNum;
    bacEncContextPtr->bitsRemainingNum -= shiftNum;
    WriteOut(bacEncContextPtr);

}

void RemainingCoeffExponentialGolombCode(
    CabacEncodeContext_t *cabacEncodeCtxPtr,
    EB_U32 symbolValue,
    EB_U32 golombParam)
{
    EB_S32 codeWord = symbolValue >> golombParam;
    EB_U32 numberOfBins;
    EB_U32 prefixLen;
    EB_U32 code;
    EB_U32 n;
    EB_U32 r;

    if (codeWord <= COEF_REMAIN_BIN_REDUCTION)
    {
        prefixLen = 1 + codeWord;
        code = (1 << prefixLen) - 2;
        numberOfBins = prefixLen;
    }
    else
    {
        r = codeWord - (COEF_REMAIN_BIN_REDUCTION - 1);
        n = Log2f(r);
        prefixLen = COEF_REMAIN_BIN_REDUCTION + 1 + n;
        code = (1 << prefixLen) - 2;
        // n has a max value of 19, need to write out the prefix because the entire code may be longer than 32 bits
        EncodeBypassBins(&(cabacEncodeCtxPtr->bacEncContext), code, prefixLen);

        numberOfBins = n;
        code = r - (1 << n);
    }

    numberOfBins += golombParam;
    code = (code << golombParam) + symbolValue - (codeWord << golombParam);
    EncodeBypassBins(&(cabacEncodeCtxPtr->bacEncContext), code, numberOfBins);
}

EB_U32 EstimateRemainingCoeffExponentialGolombCode(EB_U32 symbolValue, EB_U32 golombParam)
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

void EncodeLastSignificantXY(
    CabacEncodeContext_t   *cabacEncodeCtxPtr,
    EB_U32                  lastSigXPos,
    EB_U32                  lastSigYPos,
    const EB_U32            size,
    const EB_U32            logBlockSize,
    const EB_U32            isChroma)
{
    EB_U32 xGroupIndex, yGroupIndex;
    EB_S32 blockSizeOffset;
    EB_S32 shift;
    EB_U32 contextIndex;
    EB_S32 groupCount;

    blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));
    shift = isChroma ? (logBlockSize - 2) : ((logBlockSize + 1) >> 2);

    // X position
    xGroupIndex = lastSigXYGroupIndex[lastSigXPos];

    for (contextIndex = 0; contextIndex < xGroupIndex; contextIndex++)
    {
        EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), 1,
            &(cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset + (contextIndex >> shift)]));
    }

    if (xGroupIndex < lastSigXYGroupIndex[size - 1])
    {
        EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), 0,
            &(cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset + (contextIndex >> shift)]));
    }

    // Y position
    yGroupIndex = lastSigXYGroupIndex[lastSigYPos];

    for (contextIndex = 0; contextIndex < yGroupIndex; contextIndex++)
    {
        EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), 1,
            &(cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset + (contextIndex >> shift)]));
    }

    if (yGroupIndex < lastSigXYGroupIndex[size - 1])
    {
        EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), 0,
            &(cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset + (contextIndex >> shift)]));
    }

    if (xGroupIndex > 3)
    {
        groupCount = (xGroupIndex - 2) >> 1;
        EncodeBypassBins(&(cabacEncodeCtxPtr->bacEncContext), lastSigXPos - minInSigXYGroup[xGroupIndex], groupCount);
    }

    if (yGroupIndex > 3)
    {
        groupCount = (yGroupIndex - 2) >> 1;
        EncodeBypassBins(&(cabacEncodeCtxPtr->bacEncContext), lastSigYPos - minInSigXYGroup[yGroupIndex], groupCount);
    }
}


/*********************************************************************
* EstimateQuantizedCoefficients
*   Estimate quantized coefficients rate
*   This function is designed for square quantized coefficient blocks.
*   So, in NSQT is implemented, this function  should be modified
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to prediction unit
*
*  coeffBufferPtr
*   input pointer to the quantized coefficinet buffer
*
*  coeffStride
*   stride of the coefficient buffer
*
*  componentType
*   component type indicated by  0 = Luma, 1 = Chroma, 2 = Cb, 3 = Cr, 4 = ALL
*
*  numNonZeroCoeffs
*   indicates the number of non zero coefficients in the coefficient buffer
*********************************************************************/
EB_U32 EstimateLastSignificantXY(
    CabacEncodeContext_t   *cabacEncodeCtxPtr,
    EB_U32                  lastSigXPos,
    EB_U32                  lastSigYPos,
    const EB_U32            size,
    const EB_U32            logBlockSize,
    const EB_U32            isChroma)
{
    EB_U32 coeffBits = 0;
    EB_U32 xGroupIndex, yGroupIndex;
    EB_S32 blockSizeOffset;
    EB_S32 shift;
    EB_U32 contextIndex;
    EB_S32 groupCount;

    blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));
    shift = isChroma ? (logBlockSize - 2) : ((logBlockSize + 1) >> 2);

    // X position
    xGroupIndex = lastSigXYGroupIndex[lastSigXPos];

    for (contextIndex = 0; contextIndex < xGroupIndex; contextIndex++)
    {
        coeffBits += CabacEstimatedBits[1 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset + (contextIndex >> shift)]];
    }

    if (xGroupIndex < lastSigXYGroupIndex[size - 1])
    {
        coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset + (contextIndex >> shift)]];
    }

    if (xGroupIndex > 3)
    {
        groupCount = (xGroupIndex - 2) >> 1;
        coeffBits += groupCount * ONE_BIT;
    }


    // Y position
    yGroupIndex = lastSigXYGroupIndex[lastSigYPos];

    for (contextIndex = 0; contextIndex < yGroupIndex; contextIndex++)
    {
        coeffBits += CabacEstimatedBits[1 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset + (contextIndex >> shift)]];
    }

    if (yGroupIndex < lastSigXYGroupIndex[size - 1])
    {
        coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset + (contextIndex >> shift)]];
    }

    if (yGroupIndex > 3)
    {
        groupCount = (yGroupIndex - 2) >> 1;
        coeffBits += groupCount * ONE_BIT;
    }

    return coeffBits;
}
/************************************************
* Bac Estimate Context: Estimate One Bin Function
*
* input: bacEncContext pointer
* input: BinaryValue
* input: contextModelPtr
*
************************************************/
void EstimateOneBin(
    const EB_U32     BinaryValue,
    EB_ContextModel *contextModelPtr){

    *contextModelPtr = UPDATE_CONTEXT_MODEL(BinaryValue, contextModelPtr);

}
