/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbCodingUnit.h"
#include "EbUtility.h"
#include "EbTransformUnit.h"
#include "EbPictureControlSet.h"


static void LargestCodingUnitDctor(EB_PTR p)
{
    LargestCodingUnit_t* obj = (LargestCodingUnit_t*)p;
    EB_DELETE(obj->quantizedCoeff);
    EB_FREE_PTR_ARRAY(obj->codedLeafArrayPtr, CU_MAX_COUNT);
}
/*
Tasks & Questions
    -Need a GetEmptyChain function for testing sub partitions.  Tie it to an Itr?
    -How many empty CUs do we need?  We need to have enough for the max CU count AND
       enough for temp storage when calculating a subdivision.
    -Where do we store temp reconstructed picture data while deciding to subdivide or not?
    -Need a ReconPicture for each candidate.
    -I don't see a way around doing the copies in temp memory and then copying it in...
*/
EB_ERRORTYPE LargestCodingUnitCtor(
    LargestCodingUnit_t         *largestCodingUnitPtr,
    EB_U8                        lcuSize,
    EB_U32                       pictureWidth,
    EB_U32                       pictureHeight,
    EB_U16                       lcuOriginX,
    EB_U16                       lcuOriginY,
    EB_U16                       lcuIndex,
    struct PictureControlSet_s  *pictureControlSet)

{
    EB_U32 borderLargestCuSize;
    EbPictureBufferDescInitData_t coeffInitData;

    largestCodingUnitPtr->dctor = LargestCodingUnitDctor;

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

    EB_ALLOC_PTR_ARRAY(largestCodingUnitPtr->codedLeafArrayPtr, CU_MAX_COUNT);

    for(EB_U8 codedLeafIndex=0; codedLeafIndex < CU_MAX_COUNT; ++codedLeafIndex) {
        EB_MALLOC(largestCodingUnitPtr->codedLeafArrayPtr[codedLeafIndex], sizeof(CodingUnit_t));
        for(EB_U32 tuIndex = 0; tuIndex < TRANSFORM_UNIT_MAX_COUNT; ++tuIndex){
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

    EB_NEW(
        largestCodingUnitPtr->quantizedCoeff,
        EbPictureBufferDescCtor,
        (EB_PTR)&coeffInitData);

    return EB_ErrorNone;
}
