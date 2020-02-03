/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbComputeMean_C.h"
#include "EbDefinitions.h"

#define VARIANCE_PRECISION      16

/*******************************************
* ComputeMean
*   returns the mean of a block
*******************************************/
EB_U64 ComputeMean(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight)   // input parameter, input area height
{

    EB_U32 horizontalIndex;
    EB_U32 verticalIndex;
    EB_U64 blockMean = 0;

    for (verticalIndex = 0; verticalIndex < inputAreaHeight; verticalIndex++) {
        for (horizontalIndex = 0; horizontalIndex < inputAreaWidth; horizontalIndex++) {

            blockMean += inputSamples[horizontalIndex];

        }
        inputSamples += inputStride;
    }

    blockMean = (blockMean << (VARIANCE_PRECISION >> 1)) / (inputAreaWidth * inputAreaHeight);

    return blockMean;
}


/*******************************************
* ComputeMeanOfSquaredValues
*   returns the Mean of Squared Values
*******************************************/
EB_U64 ComputeMeanOfSquaredValues(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight)   // input parameter, input area height
{

    EB_U32 horizontalIndex;
    EB_U32 verticalIndex;
    EB_U64 blockMean = 0;

    for (verticalIndex = 0; verticalIndex < inputAreaHeight; verticalIndex++) {
        for (horizontalIndex = 0; horizontalIndex < inputAreaWidth; horizontalIndex++) {

            blockMean += inputSamples[horizontalIndex] * inputSamples[horizontalIndex];;

        }
        inputSamples += inputStride;
    }

    blockMean = (blockMean << VARIANCE_PRECISION) / (inputAreaWidth * inputAreaHeight);

    return blockMean;
}

EB_U64 ComputeSubMeanOfSquaredValues(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight)   // input parameter, input area height
{

    EB_U32 horizontalIndex;
    EB_U32 verticalIndex = 0;
    EB_U64 blockMean = 0;
    EB_U16 skip = 0;


    for (verticalIndex = 0; skip < inputAreaHeight; skip =verticalIndex + verticalIndex) {
        for (horizontalIndex = 0; horizontalIndex < inputAreaWidth; horizontalIndex++) {

            blockMean += inputSamples[horizontalIndex] * inputSamples[horizontalIndex];

        }
        inputSamples += 2*inputStride;
        verticalIndex++;
    }

    blockMean = blockMean << 11; //VARIANCE_PRECISION) / (inputAreaWidth * inputAreaHeight);

    return blockMean;
}

EB_U64 ComputeSubMean8x8(
    EB_U8 *  inputSamples,      // input parameter, input samples Ptr
    EB_U32   inputStride,       // input parameter, input stride
    EB_U32   inputAreaWidth,    // input parameter, input area width
    EB_U32   inputAreaHeight)   // input parameter, input area height
{

    EB_U32 horizontalIndex;
    EB_U32 verticalIndex = 0;
    EB_U64 blockMean = 0;
    EB_U16 skip = 0;


    for (verticalIndex = 0; skip < inputAreaHeight; skip =verticalIndex + verticalIndex) {
        for (horizontalIndex = 0; horizontalIndex < inputAreaWidth; horizontalIndex++) {

            blockMean += inputSamples[horizontalIndex] * inputSamples[horizontalIndex];

        }
        inputSamples += 2*inputStride;
        verticalIndex++;
    }

    blockMean = blockMean << 11; //VARIANCE_PRECISION) / (inputAreaWidth * inputAreaHeight);

    return blockMean;
}




