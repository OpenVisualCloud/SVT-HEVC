/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbCodingUnit.h"
#include "EbUtility.h"
#include "EbTransformUnit.h"
#include "EbPictureControlSet.h"
#include "EbThreads.h"

/*
Tasks & Questions
    -Need a GetEmptyChain function for testing sub partitions.  Tie it to an Itr?
    -How many empty CUs do we need?  We need to have enough for the max CU count AND
       enough for temp storage when calculating a subdivision.
    -Where do we store temp reconstructed picture data while deciding to subdivide or not?
    -Need a ReconPicture for each candidate.
    -I don't see a way around doing the copies in temp memory and then copying it in...
*/
EB_ERRORTYPE RCStatRowCtor(
    RCStatRow_t         **rcStatRowDblPtr,
    EB_U16                rowIndex)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    RCStatRow_t *rcStatRowPtr;
    EB_MALLOC(RCStatRow_t*, rcStatRowPtr, sizeof(RCStatRow_t), EB_N_PTR);
    *rcStatRowDblPtr = rcStatRowPtr;
    rcStatRowPtr->rowIndex = rowIndex;
    rcStatRowPtr->predictedBits = 0;
    rcStatRowPtr->encodedBits = 0;
    rcStatRowPtr->rowQp = 0;
    rcStatRowPtr->totalCUEncoded = 0;
    rcStatRowPtr->lastEncodedCU = 0;
    EB_CREATEMUTEX(EB_HANDLE, rcStatRowPtr->rowUpdateMutex, sizeof(EB_HANDLE), EB_MUTEX);
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    return EB_ErrorNone;
}

EB_ERRORTYPE LargestCodingUnitCtor(
    LargestCodingUnit_t        **largetCodingUnitDblPtr,
    EB_U8                        lcuSize,
    EB_U32                       pictureWidth,
    EB_U32                       pictureHeight,
    EB_U16                       lcuOriginX,
    EB_U16                       lcuOriginY,
    EB_U16                       lcuIndex,
    struct PictureControlSet_s  *pictureControlSet)

{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_U32 borderLargestCuSize;
    EB_U8 codedLeafIndex;
    EB_U32 tuIndex;
    EbPictureBufferDescInitData_t coeffInitData;
    
    LargestCodingUnit_t *largestCodingUnitPtr;
    EB_MALLOC(LargestCodingUnit_t*, largestCodingUnitPtr, sizeof(LargestCodingUnit_t), EB_N_PTR);

    *largetCodingUnitDblPtr = largestCodingUnitPtr;
    
    // ************ LCU ***************
    if((pictureWidth - lcuOriginX) < lcuSize) {
        borderLargestCuSize = pictureWidth - lcuOriginX;
        // Which borderLargestCuSize is not a power of two
        while((borderLargestCuSize & (borderLargestCuSize - 1)) > 0) {
            borderLargestCuSize -= (borderLargestCuSize & ((~0u) << Log2f(borderLargestCuSize)));
        }
    }
    
    if((pictureHeight - lcuOriginY) < lcuSize) {
        borderLargestCuSize = pictureHeight - lcuOriginY;
        // Which borderLargestCuSize is not a power of two
        while((borderLargestCuSize & (borderLargestCuSize - 1)) > 0) {
            borderLargestCuSize -= (borderLargestCuSize & ((~0u) << Log2f(borderLargestCuSize)));
        }
    }
    largestCodingUnitPtr->pictureControlSetPtr          = pictureControlSet;
    largestCodingUnitPtr->size                          = lcuSize;
    largestCodingUnitPtr->sizeLog2                      = (EB_U8)Log2f(lcuSize);
    largestCodingUnitPtr->originX                       = lcuOriginX;
    largestCodingUnitPtr->originY                       = lcuOriginY;
    
    largestCodingUnitPtr->index                         = lcuIndex; 
    largestCodingUnitPtr->proxytotalBits                = 0;
    largestCodingUnitPtr->rowInd                        = 0;
    largestCodingUnitPtr->intraSadInterval              = 0;
    largestCodingUnitPtr->interSadInterval              = 0;
    EB_MALLOC(CodingUnit_t**, largestCodingUnitPtr->codedLeafArrayPtr, sizeof(CodingUnit_t*) * CU_MAX_COUNT, EB_N_PTR);
    for(codedLeafIndex=0; codedLeafIndex < CU_MAX_COUNT; ++codedLeafIndex) {
        EB_MALLOC(CodingUnit_t*, largestCodingUnitPtr->codedLeafArrayPtr[codedLeafIndex], sizeof(CodingUnit_t) , EB_N_PTR);
        for(tuIndex = 0; tuIndex < TRANSFORM_UNIT_MAX_COUNT; ++tuIndex){
            largestCodingUnitPtr->codedLeafArrayPtr[codedLeafIndex]->transformUnitArray[tuIndex].tuIndex = tuIndex;
        }

       largestCodingUnitPtr->codedLeafArrayPtr[codedLeafIndex]->leafIndex = codedLeafIndex;

    }
    
    coeffInitData.bufferEnableMask  = PICTURE_BUFFER_DESC_FULL_MASK;
    coeffInitData.maxWidth          = lcuSize;
    coeffInitData.maxHeight         = lcuSize;
    coeffInitData.bitDepth          = EB_16BIT;
    coeffInitData.colorFormat       = largestCodingUnitPtr->pictureControlSetPtr->colorFormat;
	coeffInitData.leftPadding		= 0;
	coeffInitData.rightPadding		= 0;
	coeffInitData.topPadding		= 0;
	coeffInitData.botPadding		= 0;
    coeffInitData.splitMode         = EB_FALSE;


    return_error = EbPictureBufferDescCtor(
        (EB_PTR*) &(largestCodingUnitPtr->quantizedCoeff),
        (EB_PTR)  &coeffInitData);
	if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    return EB_ErrorNone;
}
