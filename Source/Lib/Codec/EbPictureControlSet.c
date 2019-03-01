/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbPictureControlSet.h"
#include "EbPictureBufferDesc.h"

EB_ERRORTYPE PictureControlSetCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr)
{
    PictureControlSet_t *objectPtr;
    PictureControlSetInitData_t *initDataPtr    = (PictureControlSetInitData_t*) objectInitDataPtr;

    EbPictureBufferDescInitData_t inputPictureBufferDescInitData;
    EbPictureBufferDescInitData_t coeffBufferDescInitData; 

    // Max/Min CU Sizes
    const EB_U32 maxCuSize = initDataPtr->lcuSize;
    
    // LCUs
    const EB_U16 pictureLcuWidth    = (EB_U16)((initDataPtr->pictureWidth + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    const EB_U16 pictureLcuHeight   = (EB_U16)((initDataPtr->pictureHeight + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    EB_U16 lcuIndex;
    EB_U16 lcuOriginX;
    EB_U16 lcuOriginY;
    EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_BOOL is16bit = initDataPtr->is16bit;
    EB_U16 subWidthCMinus1 = (initDataPtr->colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (initDataPtr->colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    EB_MALLOC(PictureControlSet_t*, objectPtr, sizeof(PictureControlSet_t), EB_N_PTR);
  
    // Init Picture Init data
    inputPictureBufferDescInitData.maxWidth            = initDataPtr->pictureWidth;
    inputPictureBufferDescInitData.maxHeight           = initDataPtr->pictureHeight;
    inputPictureBufferDescInitData.bitDepth            = initDataPtr->bitDepth;
    inputPictureBufferDescInitData.colorFormat         = initDataPtr->colorFormat;
    inputPictureBufferDescInitData.bufferEnableMask    = PICTURE_BUFFER_DESC_FULL_MASK;
	inputPictureBufferDescInitData.leftPadding			= 0;
	inputPictureBufferDescInitData.rightPadding		= 0;
	inputPictureBufferDescInitData.topPadding			= 0;
	inputPictureBufferDescInitData.botPadding			= 0;
    inputPictureBufferDescInitData.splitMode           = EB_FALSE;
    
    coeffBufferDescInitData.maxWidth                            = initDataPtr->pictureWidth;
    coeffBufferDescInitData.maxHeight                           = initDataPtr->pictureHeight;
    coeffBufferDescInitData.bitDepth                            = EB_16BIT;
    coeffBufferDescInitData.colorFormat                         = initDataPtr->colorFormat;
    coeffBufferDescInitData.bufferEnableMask                    = PICTURE_BUFFER_DESC_FULL_MASK;
	coeffBufferDescInitData.leftPadding							= 0;
	coeffBufferDescInitData.rightPadding						= 0;
	coeffBufferDescInitData.topPadding							= 0;
	coeffBufferDescInitData.botPadding							= 0;
    coeffBufferDescInitData.splitMode                           = EB_FALSE;


    *objectDblPtr = (EB_PTR) objectPtr;
    
    objectPtr->sequenceControlSetWrapperPtr     = (EbObjectWrapper_t *)EB_NULL;

    objectPtr->colorFormat          =  initDataPtr->colorFormat;
    objectPtr->reconPicture16bitPtr =  (EbPictureBufferDesc_t *)EB_NULL;
    objectPtr->reconPicturePtr      =  (EbPictureBufferDesc_t *)EB_NULL;
    // Reconstructed Picture Buffer
    if(initDataPtr->is16bit == EB_TRUE){
        return_error = EbReconPictureBufferDescCtor(
        (EB_PTR*) &(objectPtr->reconPicture16bitPtr),
        (EB_PTR ) &coeffBufferDescInitData);
    }
    else
    {

        return_error = EbReconPictureBufferDescCtor(
        (EB_PTR*) &(objectPtr->reconPicturePtr),
        (EB_PTR ) &inputPictureBufferDescInitData);
    }
    
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Entropy Coder
    return_error = EntropyCoderCtor(
        &objectPtr->entropyCoderPtr,
        SEGMENT_ENTROPY_BUFFER_SIZE);

	if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
	
	// Cabaccost
    EB_MALLOC(CabacCost_t*, objectPtr->cabacCost, sizeof(CabacCost_t), EB_N_PTR);

    // Packetization process Bitstream 
    return_error = BitstreamCtor(
        &objectPtr->bitstreamPtr,
        PACKETIZATION_PROCESS_BUFFER_SIZE);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Rate estimation entropy coder
    return_error = EntropyCoderCtor(
        &objectPtr->coeffEstEntropyCoderPtr,
        SEGMENT_ENTROPY_BUFFER_SIZE);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    
    // GOP
    objectPtr->pictureNumber        = 0;
    objectPtr->temporalLayerIndex   = 0;
    objectPtr->temporalId           = 0;

    // LCU Array
    objectPtr->lcuMaxDepth = (EB_U8) initDataPtr->maxDepth;
    objectPtr->lcuTotalCount = pictureLcuWidth * pictureLcuHeight;
    EB_MALLOC(LargestCodingUnit_t**, objectPtr->lcuPtrArray, sizeof(LargestCodingUnit_t*) * objectPtr->lcuTotalCount, EB_N_PTR);
    
    lcuOriginX = 0;
    lcuOriginY = 0;

    for(lcuIndex=0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {
        
        return_error = LargestCodingUnitCtor(
            &(objectPtr->lcuPtrArray[lcuIndex]),
            (EB_U8)initDataPtr->lcuSize,
            initDataPtr->pictureWidth, 
            initDataPtr->pictureHeight,
            (EB_U16)(lcuOriginX * maxCuSize),
            (EB_U16)(lcuOriginY * maxCuSize),
            (EB_U16)lcuIndex,
            objectPtr);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        // Increment the Order in coding order (Raster Scan Order)
        lcuOriginY = (lcuOriginX == pictureLcuWidth - 1) ? lcuOriginY + 1: lcuOriginY;
        lcuOriginX = (lcuOriginX == pictureLcuWidth - 1) ? 0 : lcuOriginX + 1;
    }

    // Mode Decision Control config
    EB_MALLOC(MdcLcuData_t*, objectPtr->mdcLcuArray, objectPtr->lcuTotalCount  * sizeof(MdcLcuData_t), EB_N_PTR);
    objectPtr->qpArrayStride = (EB_U16)((initDataPtr->pictureWidth +  MIN_CU_SIZE - 1) / MIN_CU_SIZE);
    objectPtr->qpArraySize   = ((initDataPtr->pictureWidth +  MIN_CU_SIZE - 1) / MIN_CU_SIZE) * 
        ((initDataPtr->pictureHeight +  MIN_CU_SIZE - 1) / MIN_CU_SIZE);
    
    // Allocate memory for vertical edge bS array
    EB_MALLOC(EB_U8**, objectPtr->verticalEdgeBSArray, sizeof(EB_U8*) * objectPtr->lcuTotalCount, EB_N_PTR);

    for(lcuIndex=0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {
        EB_MALLOC(EB_U8*, objectPtr->verticalEdgeBSArray[lcuIndex], sizeof(EB_U8) * VERTICAL_EDGE_BS_ARRAY_SIZE, EB_N_PTR);
    }

    // Allocate memory for horizontal edge bS array
    EB_MALLOC(EB_U8**, objectPtr->horizontalEdgeBSArray, sizeof(EB_U8*) * objectPtr->lcuTotalCount, EB_N_PTR);

    for(lcuIndex=0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {
        EB_MALLOC(EB_U8*, objectPtr->horizontalEdgeBSArray[lcuIndex], sizeof(EB_U8) * HORIZONTAL_EDGE_BS_ARRAY_SIZE, EB_N_PTR);
    }

	// Allocate memory for qp array (used by DLF)
    EB_MALLOC(EB_U8*, objectPtr->qpArray, sizeof(EB_U8) * objectPtr->qpArraySize, EB_N_PTR);

    EB_MALLOC(EB_U8*, objectPtr->entropyQpArray, sizeof(EB_U8) * objectPtr->qpArraySize, EB_N_PTR);
    
	// Allocate memory for cbf array (used by DLF)
    EB_MALLOC(EB_U8*, objectPtr->cbfMapArray, sizeof(EB_U8) * ((initDataPtr->pictureWidth >> 2) * (initDataPtr->pictureHeight >> 2)), EB_N_PTR);

    // Mode Decision Neighbor Arrays
    EB_U8 depth;
    for (depth = 0; depth < NEIGHBOR_ARRAY_TOTAL_COUNT; depth++) {
        return_error = NeighborArrayUnitCtor(
            &objectPtr->mdIntraLumaModeNeighborArray[depth],
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(EB_U8),
            PU_NEIGHBOR_ARRAY_GRANULARITY,
            PU_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = NeighborArrayUnitCtor(
            &objectPtr->mdMvNeighborArray[depth],
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(MvUnit_t),
            PU_NEIGHBOR_ARRAY_GRANULARITY,
            PU_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = NeighborArrayUnitCtor(
            &objectPtr->mdSkipFlagNeighborArray[depth],
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(EB_U8),
            CU_NEIGHBOR_ARRAY_GRANULARITY,
            CU_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = NeighborArrayUnitCtor(
            &objectPtr->mdModeTypeNeighborArray[depth],
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(EB_U8),
            PU_NEIGHBOR_ARRAY_GRANULARITY,
            PU_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        return_error = NeighborArrayUnitCtor(
            &objectPtr->mdLeafDepthNeighborArray[depth],
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(EB_U8),
            CU_NEIGHBOR_ARRAY_GRANULARITY,
            CU_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = NeighborArrayUnitCtor(
            &objectPtr->mdLumaReconNeighborArray[depth],
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(EB_U8),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        return_error = NeighborArrayUnitCtor(
            &objectPtr->mdCbReconNeighborArray[depth],
            MAX_PICTURE_WIDTH_SIZE >> 1,
            MAX_PICTURE_HEIGHT_SIZE >> 1,
            sizeof(EB_U8),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        return_error = NeighborArrayUnitCtor(
            &objectPtr->mdCrReconNeighborArray[depth],
            MAX_PICTURE_WIDTH_SIZE >> 1,
            MAX_PICTURE_HEIGHT_SIZE >> 1,
            sizeof(EB_U8),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

	return_error = NeighborArrayUnitCtor(
		&objectPtr->mdRefinementIntraLumaModeNeighborArray,
		MAX_PICTURE_WIDTH_SIZE,
		MAX_PICTURE_HEIGHT_SIZE,
		sizeof(EB_U8),
		PU_NEIGHBOR_ARRAY_GRANULARITY,
		PU_NEIGHBOR_ARRAY_GRANULARITY,
		NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}

	return_error = NeighborArrayUnitCtor(
		&objectPtr->mdRefinementModeTypeNeighborArray,
		MAX_PICTURE_WIDTH_SIZE,
		MAX_PICTURE_HEIGHT_SIZE,
		sizeof(EB_U8),
		PU_NEIGHBOR_ARRAY_GRANULARITY,
		PU_NEIGHBOR_ARRAY_GRANULARITY,
		NEIGHBOR_ARRAY_UNIT_FULL_MASK);

	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}

	return_error = NeighborArrayUnitCtor(
		&objectPtr->mdRefinementLumaReconNeighborArray,
		MAX_PICTURE_WIDTH_SIZE,
		MAX_PICTURE_HEIGHT_SIZE,
		sizeof(EB_U8),
		SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
		SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
		NEIGHBOR_ARRAY_UNIT_FULL_MASK);
	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}

    // Encode Pass Neighbor Arrays
    return_error = NeighborArrayUnitCtor(
        &objectPtr->epIntraLumaModeNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->epMvNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(MvUnit_t),
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->epSkipFlagNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        CU_NEIGHBOR_ARRAY_GRANULARITY,
        CU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->epModeTypeNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->epLeafDepthNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        CU_NEIGHBOR_ARRAY_GRANULARITY,
        CU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    
    return_error = NeighborArrayUnitCtor(
        &objectPtr->epLumaReconNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->epCbReconNeighborArray,
        MAX_PICTURE_WIDTH_SIZE >> subWidthCMinus1,
        MAX_PICTURE_HEIGHT_SIZE >> subHeightCMinus1,
        sizeof(EB_U8),
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->epCrReconNeighborArray,
        MAX_PICTURE_WIDTH_SIZE >> subWidthCMinus1,
        MAX_PICTURE_HEIGHT_SIZE >> subHeightCMinus1,
        sizeof(EB_U8),
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    if(is16bit){
        return_error = NeighborArrayUnitCtor(
            &objectPtr->epLumaReconNeighborArray16bit,
            MAX_PICTURE_WIDTH_SIZE,
            MAX_PICTURE_HEIGHT_SIZE,
            sizeof(EB_U16),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = NeighborArrayUnitCtor(
            &objectPtr->epCbReconNeighborArray16bit,
            MAX_PICTURE_WIDTH_SIZE >> subWidthCMinus1,
            MAX_PICTURE_HEIGHT_SIZE >> subHeightCMinus1,
             sizeof(EB_U16),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
        return_error = NeighborArrayUnitCtor(
            &objectPtr->epCrReconNeighborArray16bit,
            MAX_PICTURE_WIDTH_SIZE >> subWidthCMinus1,
            MAX_PICTURE_HEIGHT_SIZE >> subHeightCMinus1,
            sizeof(EB_U16),
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
            NEIGHBOR_ARRAY_UNIT_FULL_MASK);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    else {
        objectPtr->epLumaReconNeighborArray16bit = 0;
        objectPtr->epCbReconNeighborArray16bit = 0;
        objectPtr->epCrReconNeighborArray16bit = 0;
    }

    return_error = NeighborArrayUnitCtor(
        &objectPtr->epSaoNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(SaoParameters_t),
        LCU_NEIGHBOR_ARRAY_GRANULARITY,
        LCU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->amvpMvMergeMvNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(MvUnit_t),
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->amvpMvMergeModeTypeNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_FULL_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Entropy Coding Neighbor Arrays
    return_error = NeighborArrayUnitCtor(
        &objectPtr->modeTypeNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->leafDepthNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        CU_NEIGHBOR_ARRAY_GRANULARITY,
        CU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    return_error = NeighborArrayUnitCtor(
        &objectPtr->skipFlagNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        CU_NEIGHBOR_ARRAY_GRANULARITY,
        CU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

	if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    
    return_error = NeighborArrayUnitCtor(
        &objectPtr->intraLumaModeNeighborArray,
        MAX_PICTURE_WIDTH_SIZE,
        MAX_PICTURE_HEIGHT_SIZE,
        sizeof(EB_U8),
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        PU_NEIGHBOR_ARRAY_GRANULARITY,
        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // Note - non-zero offsets are not supported (to be fixed later in DLF chroma filtering)
    objectPtr->cbQpOffset = 0;
    objectPtr->crQpOffset = 0;

    objectPtr->sliceLevelChromaQpFlag = EB_TRUE;
	// slice level chroma QP offsets
	objectPtr->sliceCbQpOffset = 0;
	objectPtr->sliceCrQpOffset = 0;


    //objectPtr->totalNumBits = 0;

    // Error Resilience
    objectPtr->constrainedIntraFlag = EB_FALSE;

    // Segments
    return_error = EncDecSegmentsCtor(
        &objectPtr->encDecSegmentCtrl,
        initDataPtr->encDecSegmentCol,
        initDataPtr->encDecSegmentRow);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Entropy Rows
    EB_CREATEMUTEX(EB_HANDLE, objectPtr->entropyCodingMutex, sizeof(EB_HANDLE), EB_MUTEX);

    EB_CREATEMUTEX(EB_HANDLE, objectPtr->intraMutex, sizeof(EB_HANDLE), EB_MUTEX);

    return EB_ErrorNone;

}


EB_ERRORTYPE PictureParentControlSetCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr)
{
    PictureParentControlSet_t   *objectPtr;
    PictureControlSetInitData_t *initDataPtr    = (PictureControlSetInitData_t*) objectInitDataPtr;

    
    EB_ERRORTYPE return_error = EB_ErrorNone;
    const EB_U16 pictureLcuWidth    = (EB_U16)((initDataPtr->pictureWidth + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    const EB_U16 pictureLcuHeight   = (EB_U16)((initDataPtr->pictureHeight + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    EB_U16 lcuIndex;
	EB_U32 regionInPictureWidthIndex;
	EB_U32 regionInPictureHeightIndex;

    EB_MALLOC(PictureParentControlSet_t*, objectPtr, sizeof(PictureParentControlSet_t), EB_N_PTR);
    *objectDblPtr = (EB_PTR)objectPtr;

    objectPtr->sequenceControlSetWrapperPtr = (EbObjectWrapper_t *)EB_NULL;
    objectPtr->inputPictureWrapperPtr = (EbObjectWrapper_t *)EB_NULL;
    objectPtr->referencePictureWrapperPtr = (EbObjectWrapper_t *)EB_NULL;
    if (initDataPtr->colorFormat >= EB_YUV422) {
        EbPictureBufferDescInitData_t inputPictureBufferDescInitData;
        inputPictureBufferDescInitData.maxWidth     = initDataPtr->pictureWidth;
        inputPictureBufferDescInitData.maxHeight    = initDataPtr->pictureHeight;
        inputPictureBufferDescInitData.bitDepth     = EB_8BIT; // Should be 8bit
        inputPictureBufferDescInitData.bufferEnableMask = PICTURE_BUFFER_DESC_CHROMA_MASK;
        inputPictureBufferDescInitData.leftPadding  = initDataPtr->leftPadding;
        inputPictureBufferDescInitData.rightPadding = initDataPtr->rightPadding;
        inputPictureBufferDescInitData.topPadding   = initDataPtr->topPadding;
        inputPictureBufferDescInitData.botPadding   = initDataPtr->botPadding;
        inputPictureBufferDescInitData.colorFormat  = EB_YUV420;
        inputPictureBufferDescInitData.splitMode    = EB_FALSE;
        return_error = EbPictureBufferDescCtor(
                (EB_PTR*) &(objectPtr->chromaDownSamplePicturePtr),
                (EB_PTR ) &inputPictureBufferDescInitData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    } else if(initDataPtr->colorFormat == EB_YUV420) {
        objectPtr->chromaDownSamplePicturePtr = NULL;
    } else {
        return EB_ErrorBadParameter;
    }

    // GOP
    objectPtr->predStructIndex      = 0;
    objectPtr->pictureNumber        = 0;
    objectPtr->idrFlag              = EB_FALSE;
    objectPtr->temporalLayerIndex   = 0;

    objectPtr->totalNumBits = 0;

    objectPtr->lastIdrPicture = 0;

    objectPtr->lcuTotalCount            = pictureLcuWidth * pictureLcuHeight;

	EB_MALLOC(EB_U16**, objectPtr->variance, sizeof(EB_U16*) * objectPtr->lcuTotalCount, EB_N_PTR);
	EB_MALLOC(EB_U8**, objectPtr->yMean, sizeof(EB_U8*) * objectPtr->lcuTotalCount, EB_N_PTR);
	EB_MALLOC(EB_U8**, objectPtr->cbMean, sizeof(EB_U8*) * objectPtr->lcuTotalCount, EB_N_PTR);
	EB_MALLOC(EB_U8**, objectPtr->crMean, sizeof(EB_U8*) * objectPtr->lcuTotalCount, EB_N_PTR);
	for (lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {
		EB_MALLOC(EB_U16*, objectPtr->variance[lcuIndex], sizeof(EB_U16) * MAX_ME_PU_COUNT, EB_N_PTR);
		EB_MALLOC(EB_U8*, objectPtr->yMean[lcuIndex], sizeof(EB_U8) * MAX_ME_PU_COUNT, EB_N_PTR);
		EB_MALLOC(EB_U8*, objectPtr->cbMean[lcuIndex], sizeof(EB_U8) * 21, EB_N_PTR);
		EB_MALLOC(EB_U8*, objectPtr->crMean[lcuIndex], sizeof(EB_U8) * 21, EB_N_PTR);
	}
    // Histograms
    EB_U32 videoComponent;

    EB_MALLOC(EB_U32****, objectPtr->pictureHistogram, sizeof(EB_U32***) * MAX_NUMBER_OF_REGIONS_IN_WIDTH, EB_N_PTR);

	for (regionInPictureWidthIndex = 0; regionInPictureWidthIndex < MAX_NUMBER_OF_REGIONS_IN_WIDTH; regionInPictureWidthIndex++){  // loop over horizontal regions
        EB_MALLOC(EB_U32***, objectPtr->pictureHistogram[regionInPictureWidthIndex], sizeof(EB_U32**) * MAX_NUMBER_OF_REGIONS_IN_HEIGHT, EB_N_PTR);
	}

	for (regionInPictureWidthIndex = 0; regionInPictureWidthIndex < MAX_NUMBER_OF_REGIONS_IN_WIDTH; regionInPictureWidthIndex++){  // loop over horizontal regions
		for (regionInPictureHeightIndex = 0; regionInPictureHeightIndex < MAX_NUMBER_OF_REGIONS_IN_HEIGHT; regionInPictureHeightIndex++){ // loop over vertical regions
            EB_MALLOC(EB_U32**, objectPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex], sizeof(EB_U32*) * 3, EB_N_PTR);
		}
	}


	for (regionInPictureWidthIndex = 0; regionInPictureWidthIndex < MAX_NUMBER_OF_REGIONS_IN_WIDTH; regionInPictureWidthIndex++){  // loop over horizontal regions
		for (regionInPictureHeightIndex = 0; regionInPictureHeightIndex < MAX_NUMBER_OF_REGIONS_IN_HEIGHT; regionInPictureHeightIndex++){ // loop over vertical regions
			for (videoComponent = 0; videoComponent < 3; ++videoComponent) {
                EB_MALLOC(EB_U32*, objectPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][videoComponent], sizeof(EB_U32) * HISTOGRAM_NUMBER_OF_BINS, EB_N_PTR);
			}
		}
	}

    EB_U32 maxOisCand = MAX(MAX_OIS_0, MAX_OIS_2);

	EB_MALLOC(OisCu32Cu16Results_t**, objectPtr->oisCu32Cu16Results, sizeof(OisCu32Cu16Results_t*) * objectPtr->lcuTotalCount, EB_N_PTR);

	for (lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex){

		EB_MALLOC(OisCu32Cu16Results_t*, objectPtr->oisCu32Cu16Results[lcuIndex], sizeof(OisCu32Cu16Results_t), EB_N_PTR);

		OisCandidate_t* contigousCand;
		EB_MALLOC(OisCandidate_t*, contigousCand, sizeof(OisCandidate_t) * maxOisCand * 21, EB_N_PTR);

		EB_U32 cuIdx;
		for (cuIdx = 0; cuIdx < 21; ++cuIdx){
			objectPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[cuIdx] = &contigousCand[cuIdx*maxOisCand];
		}
	}

	EB_MALLOC(OisCu8Results_t**, objectPtr->oisCu8Results, sizeof(OisCu8Results_t*) * objectPtr->lcuTotalCount, EB_N_PTR);
	for (lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex){

		EB_MALLOC(OisCu8Results_t*, objectPtr->oisCu8Results[lcuIndex], sizeof(OisCu8Results_t), EB_N_PTR);

		OisCandidate_t* contigousCand;
		EB_MALLOC(OisCandidate_t*, contigousCand, sizeof(OisCandidate_t) * maxOisCand * 64, EB_N_PTR);

		EB_U32 cuIdx;
		for (cuIdx = 0; cuIdx < 64; ++cuIdx){
			objectPtr->oisCu8Results[lcuIndex]->sortedOisCandidate[cuIdx] = &contigousCand[cuIdx*maxOisCand];
		}
	}


    // Motion Estimation Results
    objectPtr->maxNumberOfPusPerLcu         =   SQUARE_PU_COUNT; 
    objectPtr->maxNumberOfMeCandidatesPerPU =   3;
   

	EB_MALLOC(MeCuResults_t**, objectPtr->meResults, sizeof(MeCuResults_t*) * objectPtr->lcuTotalCount, EB_N_PTR);
	for (lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {
		EB_MALLOC(MeCuResults_t*, objectPtr->meResults[lcuIndex], sizeof(MeCuResults_t) * 85, EB_N_PTR);
	}

	EB_MALLOC(EB_U32*, objectPtr->rcMEdistortion, sizeof(EB_U32) * objectPtr->lcuTotalCount, EB_N_PTR);
	


	// ME and OIS Distortion Histograms
    EB_MALLOC(EB_U16*, objectPtr->meDistortionHistogram, sizeof(EB_U16) * NUMBER_OF_SAD_INTERVALS, EB_N_PTR);

    EB_MALLOC(EB_U16*, objectPtr->oisDistortionHistogram, sizeof(EB_U16) * NUMBER_OF_INTRA_SAD_INTERVALS, EB_N_PTR);

    EB_MALLOC(EB_U32*, objectPtr->intraSadIntervalIndex, sizeof(EB_U32) * objectPtr->lcuTotalCount, EB_N_PTR);
    EB_MALLOC(EB_U32*, objectPtr->interSadIntervalIndex, sizeof(EB_U32) * objectPtr->lcuTotalCount, EB_N_PTR);

	// Enhance background for base layer frames:  zz SAD array
	EB_MALLOC(EB_U8*, objectPtr->zzCostArray, sizeof(EB_U8) * objectPtr->lcuTotalCount * 64, EB_N_PTR);

	// Non moving index array

    EB_MALLOC(EB_U8*, objectPtr->nonMovingIndexArray, sizeof(EB_U8) * objectPtr->lcuTotalCount, EB_N_PTR);

    // similar Colocated Lcu array
    EB_MALLOC(EB_BOOL*, objectPtr->similarColocatedLcuArray, sizeof(EB_BOOL) * objectPtr->lcuTotalCount, EB_N_PTR);

    // similar Colocated Lcu array
    EB_MALLOC(EB_BOOL*, objectPtr->similarColocatedLcuArrayAllLayers, sizeof(EB_BOOL) * objectPtr->lcuTotalCount, EB_N_PTR);

    // LCU noise variance array
	EB_MALLOC(EB_U8*, objectPtr->lcuFlatNoiseArray, sizeof(EB_U8) * objectPtr->lcuTotalCount, EB_N_PTR);
	EB_MALLOC(EB_U64*, objectPtr->lcuVarianceOfVarianceOverTime, sizeof(EB_U64) * objectPtr->lcuTotalCount, EB_N_PTR);
	EB_MALLOC(EB_BOOL*, objectPtr->isLcuHomogeneousOverTime, sizeof(EB_BOOL) * objectPtr->lcuTotalCount, EB_N_PTR);
    EB_MALLOC(EdgeLcuResults_t*, objectPtr->edgeResultsPtr, sizeof(EdgeLcuResults_t) * objectPtr->lcuTotalCount, EB_N_PTR);
    EB_MALLOC(EB_U8*, objectPtr->sharpEdgeLcuFlag, sizeof(EB_BOOL) * objectPtr->lcuTotalCount, EB_N_PTR);

    EB_MALLOC(EB_U8*, objectPtr->failingMotionLcuFlag, sizeof(EB_BOOL) * objectPtr->lcuTotalCount, EB_N_PTR);

	EB_MALLOC(EB_BOOL*, objectPtr->uncoveredAreaLcuFlag, sizeof(EB_BOOL) * objectPtr->lcuTotalCount, EB_N_PTR);
    EB_MALLOC(EB_BOOL*, objectPtr->lcuHomogeneousAreaArray, sizeof(EB_BOOL) * objectPtr->lcuTotalCount, EB_N_PTR);
    
    EB_MALLOC(EB_U64**, objectPtr->varOfVar32x32BasedLcuArray, sizeof(EB_U64*) * objectPtr->lcuTotalCount, EB_N_PTR);
    for(lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {       
        EB_MALLOC(EB_U64*, objectPtr->varOfVar32x32BasedLcuArray[lcuIndex], sizeof(EB_U64) * 4, EB_N_PTR);
    }
    EB_MALLOC(EB_U8*, objectPtr->cmplxStatusLcu, sizeof(EB_U8) * objectPtr->lcuTotalCount, EB_N_PTR); 
    EB_MALLOC(EB_BOOL*, objectPtr->lcuIsolatedNonHomogeneousAreaArray, sizeof(EB_BOOL) * objectPtr->lcuTotalCount, EB_N_PTR);

    EB_MALLOC(EB_U64**, objectPtr->lcuYSrcEnergyCuArray, sizeof(EB_U64*) * objectPtr->lcuTotalCount, EB_N_PTR);
    for(lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {       
        EB_MALLOC(EB_U64*, objectPtr->lcuYSrcEnergyCuArray[lcuIndex], sizeof(EB_U64) * 5, EB_N_PTR);
    }

    EB_MALLOC(EB_U64**, objectPtr->lcuYSrcMeanCuArray, sizeof(EB_U64*) * objectPtr->lcuTotalCount, EB_N_PTR);
    for(lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {       
        EB_MALLOC(EB_U64*, objectPtr->lcuYSrcMeanCuArray[lcuIndex], sizeof(EB_U64) * 5, EB_N_PTR);
    }

    EB_MALLOC(EB_U8*, objectPtr->lcuCmplxContrastArray, sizeof(EB_U8) * objectPtr->lcuTotalCount, EB_N_PTR);

	EB_MALLOC(LcuStat_t*, objectPtr->lcuStatArray, sizeof(LcuStat_t) * objectPtr->lcuTotalCount, EB_N_PTR);

    EB_MALLOC(EB_LCU_COMPLEXITY_STATUS*, objectPtr->complexLcuArray, sizeof(EB_LCU_COMPLEXITY_STATUS) * objectPtr->lcuTotalCount, EB_N_PTR);

    EB_CREATEMUTEX(EB_HANDLE, objectPtr->rcDistortionHistogramMutex, sizeof(EB_HANDLE), EB_MUTEX);
    

    EB_MALLOC(EB_LCU_DEPTH_MODE*, objectPtr->lcuMdModeArray, sizeof(EB_LCU_DEPTH_MODE) * objectPtr->lcuTotalCount, EB_N_PTR);

    return return_error;
}

