/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbPictureControlSet.h"
#include "EbPictureBufferDesc.h"


static void ConfigureTileInfo(PictureParentControlSet_t *ppcsPtr, EB_U8 lcuSize, EB_U16 pic_width, EB_U16 pic_height)
{
    // Tiles Initialisation
    for (EB_U16 r = 0; r < ppcsPtr->tileRowCount; r++) {
        for (EB_U16 c = 0; c < ppcsPtr->tileColumnCount; c++) {
            unsigned tileIdx = r * ppcsPtr->tileColumnCount + c;
            ppcsPtr->tileInfoArray[tileIdx].tileLcuOriginX = ppcsPtr->tileColStartLcu[c];
            ppcsPtr->tileInfoArray[tileIdx].tileLcuOriginY = ppcsPtr->tileRowStartLcu[r];
            ppcsPtr->tileInfoArray[tileIdx].tileLcuEndX = ppcsPtr->tileColStartLcu[c + 1];
            ppcsPtr->tileInfoArray[tileIdx].tileLcuEndY = ppcsPtr->tileRowStartLcu[r + 1];

            ppcsPtr->tileInfoArray[tileIdx].tilePxlOriginX = ppcsPtr->tileColStartLcu[c] * lcuSize;
            ppcsPtr->tileInfoArray[tileIdx].tilePxlOriginY = ppcsPtr->tileRowStartLcu[r] * lcuSize;
            ppcsPtr->tileInfoArray[tileIdx].tilePxlEndX = (c < ppcsPtr->tileColumnCount - 1) ?
                ppcsPtr->tileColStartLcu[c + 1] * lcuSize : pic_width;
            ppcsPtr->tileInfoArray[tileIdx].tilePxlEndY = (r < ppcsPtr->tileRowCount - 1) ?
                ppcsPtr->tileRowStartLcu[r + 1] * lcuSize : pic_height;
        }
    }

    return;
}

static void ConfigureLcuEdgeInfo(PictureParentControlSet_t *ppcsPtr)
{
    EB_U32 tileCnt = ppcsPtr->tileRowCount * ppcsPtr->tileColumnCount;
    TileInfo_t *ti = ppcsPtr->tileInfoArray;
    for (EB_U16 tileIdx = 0; tileIdx < tileCnt; tileIdx++) {
        for (EB_U16 y = ti[tileIdx].tileLcuOriginY; y < ti[tileIdx].tileLcuEndY; y++) {
            for (EB_U16 x = ti[tileIdx].tileLcuOriginX; x < ti[tileIdx].tileLcuEndX; x++) {

                unsigned lcuIndex = y * ppcsPtr->pictureWidthInLcu + x;
                ppcsPtr->lcuEdgeInfoArray[lcuIndex].tileLeftEdgeFlag  = (x == ti[tileIdx].tileLcuOriginX);
                ppcsPtr->lcuEdgeInfoArray[lcuIndex].tileTopEdgeFlag   = (y == ti[tileIdx].tileLcuOriginY);
                ppcsPtr->lcuEdgeInfoArray[lcuIndex].tileRightEdgeFlag = (x == ti[tileIdx].tileLcuEndX - 1);
                ppcsPtr->lcuEdgeInfoArray[lcuIndex].pictureLeftEdgeFlag  = (x == 0) ? EB_TRUE : EB_FALSE;
                ppcsPtr->lcuEdgeInfoArray[lcuIndex].pictureTopEdgeFlag = (y == 0) ? EB_TRUE : EB_FALSE;
                ppcsPtr->lcuEdgeInfoArray[lcuIndex].pictureRightEdgeFlag = (x == (ppcsPtr->pictureWidthInLcu - 1)) ? EB_TRUE : EB_FALSE;
                ppcsPtr->lcuEdgeInfoArray[lcuIndex].tileIndexInRaster = tileIdx;
            }
        }
    }
    return;
}

static void PictureControlSetDctor(EB_PTR p)
{
    PictureControlSet_t* obj = (PictureControlSet_t*)p;
    EB_DELETE(obj->reconPicture16bitPtr);
    EB_DELETE(obj->reconPicturePtr);
    EB_FREE(obj->cabacCost);
    EB_DELETE(obj->bitstreamPtr);
    EB_DELETE(obj->coeffEstEntropyCoderPtr);
    EB_DELETE_PTR_ARRAY(obj->lcuPtrArray, obj->lcuTotalCount);
    EB_FREE_ARRAY(obj->mdcLcuArray);

    EB_FREE_2D(obj->verticalEdgeBSArray);
    EB_FREE_2D(obj->horizontalEdgeBSArray);
    EB_FREE_ARRAY(obj->qpArray);
    EB_FREE_ARRAY(obj->entropyQpArray);
    EB_FREE_ARRAY(obj->cbfMapArray);
    EB_DELETE_PTR_ARRAY(obj->epIntraLumaModeNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epMvNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epSkipFlagNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epModeTypeNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epLeafDepthNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epLumaReconNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epCbReconNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epCrReconNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epSaoNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epLumaReconNeighborArray16bit, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epCbReconNeighborArray16bit, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->epCrReconNeighborArray16bit, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->mdRefinementIntraLumaModeNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->mdRefinementModeTypeNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->mdRefinementLumaReconNeighborArray, obj->totalTileCountAllocation);

    EB_DELETE_PTR_ARRAY(obj->modeTypeNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->leafDepthNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->skipFlagNeighborArray, obj->totalTileCountAllocation);
    EB_DELETE_PTR_ARRAY(obj->intraLumaModeNeighborArray, obj->totalTileCountAllocation);

    for (EB_U8 depth = 0; depth < NEIGHBOR_ARRAY_TOTAL_COUNT; depth++) {
        EB_DELETE_PTR_ARRAY(obj->mdIntraLumaModeNeighborArray[depth], obj->totalTileCountAllocation);
        EB_DELETE_PTR_ARRAY(obj->mdMvNeighborArray[depth], obj->totalTileCountAllocation);
        EB_DELETE_PTR_ARRAY(obj->mdSkipFlagNeighborArray[depth], obj->totalTileCountAllocation);
        EB_DELETE_PTR_ARRAY(obj->mdModeTypeNeighborArray[depth], obj->totalTileCountAllocation);
        EB_DELETE_PTR_ARRAY(obj->mdLeafDepthNeighborArray[depth], obj->totalTileCountAllocation);
        EB_DELETE_PTR_ARRAY(obj->mdLumaReconNeighborArray[depth], obj->totalTileCountAllocation);
        EB_DELETE_PTR_ARRAY(obj->mdCbReconNeighborArray[depth], obj->totalTileCountAllocation);
        EB_DELETE_PTR_ARRAY(obj->mdCrReconNeighborArray[depth], obj->totalTileCountAllocation);
    }

    EB_DELETE_PTR_ARRAY(obj->encDecSegmentCtrl, obj->tileGroupCntAllocation);

    for (EB_U16 tileIdx = 0; tileIdx < obj->totalTileCountAllocation; tileIdx++) {
        EB_DELETE(obj->entropyCodingInfo[tileIdx]->entropyCoderPtr);
        EB_DESTROY_MUTEX(obj->entropyCodingInfo[tileIdx]->entropyCodingMutex);
    }
    EB_FREE_PTR_ARRAY(obj->entropyCodingInfo, obj->totalTileCountAllocation);

    EB_DESTROY_MUTEX(obj->entropyCodingPicMutex);
    EB_DESTROY_MUTEX(obj->intraMutex);
}


EB_ERRORTYPE PictureControlSetCtor(
    PictureControlSet_t *objectPtr,
    EB_PTR objectInitDataPtr)
{
    PictureControlSetInitData_t *initDataPtr    = (PictureControlSetInitData_t*) objectInitDataPtr;

    EbPictureBufferDescInitData_t inputPictureBufferDescInitData;
    EbPictureBufferDescInitData_t coeffBufferDescInitData; 

    // Max/Min CU Sizes
    const EB_U32 maxCuSize = initDataPtr->lcuSize;
    EB_U32 encDecSegRow = initDataPtr->encDecSegmentRow;
    EB_U32 encDecSegCol = initDataPtr->encDecSegmentCol;
    EB_U16 pictureWidthInLcu = (EB_U16)((initDataPtr->pictureWidth + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    EB_U16 pictureHeightInLcu = (EB_U16)((initDataPtr->pictureHeight + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    EB_U32 tileGroupCnt = initDataPtr->tileGroupRow * initDataPtr->tileGroupCol;
    objectPtr->tileGroupCntAllocation = tileGroupCnt;

    EB_U16 tileIdx;
    EB_U16 r, c;

    EB_U32 totalTileCount = initDataPtr->tileRowCount * initDataPtr->tileColumnCount;
    objectPtr->totalTileCountAllocation = totalTileCount;

    // LCUs
    EB_U16 lcuIndex;
    EB_U16 lcuOriginX;
    EB_U16 lcuOriginY;
    EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_BOOL is16bit = initDataPtr->is16bit;
    EB_U16 subWidthCMinus1 = (initDataPtr->colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (initDataPtr->colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    objectPtr->dctor = PictureControlSetDctor;

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

    objectPtr->colorFormat          =  initDataPtr->colorFormat;

    // Reconstructed Picture Buffer
    if(initDataPtr->is16bit == EB_TRUE){
        EB_NEW(
            objectPtr->reconPicture16bitPtr,
            EbReconPictureBufferDescCtor,
            (EB_PTR)&coeffBufferDescInitData);
    }
    else
    {
        EB_NEW(
            objectPtr->reconPicturePtr,
            EbReconPictureBufferDescCtor,
            (EB_PTR)&inputPictureBufferDescInitData);
    }

	// Cabaccost
    EB_MALLOC(objectPtr->cabacCost, sizeof(CabacCost_t));

    // Packetization process Bitstream 
    EB_NEW(
        objectPtr->bitstreamPtr,
        BitstreamCtor,
        PACKETIZATION_PROCESS_BUFFER_SIZE);

    // Rate estimation entropy coder
    EB_NEW(
        objectPtr->coeffEstEntropyCoderPtr,
        EntropyCoderCtor,
        SEGMENT_ENTROPY_BUFFER_SIZE);

    // GOP
    objectPtr->pictureNumber        = 0;
    objectPtr->temporalLayerIndex   = 0;
    objectPtr->temporalId           = 0;

    // LCU Array
    objectPtr->lcuMaxDepth = (EB_U8) initDataPtr->maxDepth;
    objectPtr->lcuTotalCount = pictureWidthInLcu * pictureHeightInLcu;
    EB_ALLOC_PTR_ARRAY(objectPtr->lcuPtrArray, objectPtr->lcuTotalCount);

    lcuOriginX = 0;
    lcuOriginY = 0;

    for(lcuIndex=0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {
        EB_NEW(
            objectPtr->lcuPtrArray[lcuIndex],
            LargestCodingUnitCtor,
            (EB_U8)initDataPtr->lcuSize,
            initDataPtr->pictureWidth,
            initDataPtr->pictureHeight,
            (EB_U16)(lcuOriginX * maxCuSize),
            (EB_U16)(lcuOriginY * maxCuSize),
            (EB_U16)lcuIndex,
            objectPtr);
        // Increment the Order in coding order (Raster Scan Order)
        lcuOriginY = (lcuOriginX == pictureWidthInLcu - 1) ? lcuOriginY + 1: lcuOriginY;
        lcuOriginX = (lcuOriginX == pictureWidthInLcu - 1) ? 0 : lcuOriginX + 1;
    }

    //ConfigureEdges(objectPtr, maxCuSize);

    // Mode Decision Control config
    EB_MALLOC_ARRAY(objectPtr->mdcLcuArray, objectPtr->lcuTotalCount);
    objectPtr->qpArrayStride = (EB_U16)((initDataPtr->pictureWidth +  MIN_CU_SIZE - 1) / MIN_CU_SIZE);
    objectPtr->qpArraySize   = ((initDataPtr->pictureWidth +  MIN_CU_SIZE - 1) / MIN_CU_SIZE) * 
        ((initDataPtr->pictureHeight +  MIN_CU_SIZE - 1) / MIN_CU_SIZE);

    // Allocate memory for vertical edge bS array
    EB_MALLOC_2D(objectPtr->verticalEdgeBSArray, objectPtr->lcuTotalCount, VERTICAL_EDGE_BS_ARRAY_SIZE);

    // Allocate memory for horizontal edge bS array
    EB_MALLOC_2D(objectPtr->horizontalEdgeBSArray, objectPtr->lcuTotalCount, HORIZONTAL_EDGE_BS_ARRAY_SIZE);

	// Allocate memory for qp array (used by DLF)
    EB_MALLOC_ARRAY(objectPtr->qpArray, objectPtr->qpArraySize);
    EB_MALLOC_ARRAY(objectPtr->entropyQpArray, objectPtr->qpArraySize);
	// Allocate memory for cbf array (used by DLF)
    EB_MALLOC_ARRAY(objectPtr->cbfMapArray, ((initDataPtr->pictureWidth >> 2) * (initDataPtr->pictureHeight >> 2)));

    // Jing: TO enable multi-tile, need to have neighbor per tile for EncodePass
    EB_ALLOC_PTR_ARRAY(objectPtr->epIntraLumaModeNeighborArray,  totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epMvNeighborArray ,            totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epSkipFlagNeighborArray,       totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epModeTypeNeighborArray,       totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epLeafDepthNeighborArray,      totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epLumaReconNeighborArray,      totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epCbReconNeighborArray,        totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epCrReconNeighborArray,        totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epSaoNeighborArray,            totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epLumaReconNeighborArray16bit, totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epCbReconNeighborArray16bit,   totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->epCrReconNeighborArray16bit,   totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->mdRefinementIntraLumaModeNeighborArray, totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->mdRefinementModeTypeNeighborArray,      totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->mdRefinementLumaReconNeighborArray,     totalTileCount);

    // For entropy
    EB_ALLOC_PTR_ARRAY(objectPtr->modeTypeNeighborArray,     totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->leafDepthNeighborArray,    totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->skipFlagNeighborArray, totalTileCount);
    EB_ALLOC_PTR_ARRAY(objectPtr->intraLumaModeNeighborArray,totalTileCount);


    // Mode Decision Neighbor Arrays
    for (EB_U8 depth = 0; depth < NEIGHBOR_ARRAY_TOTAL_COUNT; depth++) {
        EB_ALLOC_PTR_ARRAY(objectPtr->mdIntraLumaModeNeighborArray[depth], totalTileCount);
        EB_ALLOC_PTR_ARRAY(objectPtr->mdMvNeighborArray[depth],            totalTileCount);
        EB_ALLOC_PTR_ARRAY(objectPtr->mdSkipFlagNeighborArray[depth],      totalTileCount);
        EB_ALLOC_PTR_ARRAY(objectPtr->mdModeTypeNeighborArray[depth],      totalTileCount);
        EB_ALLOC_PTR_ARRAY(objectPtr->mdLeafDepthNeighborArray[depth],     totalTileCount);
        EB_ALLOC_PTR_ARRAY(objectPtr->mdLumaReconNeighborArray[depth],     totalTileCount);
        EB_ALLOC_PTR_ARRAY(objectPtr->mdCbReconNeighborArray[depth],       totalTileCount);
        EB_ALLOC_PTR_ARRAY(objectPtr->mdCrReconNeighborArray[depth],       totalTileCount);
    }

    for (r = 0; r < initDataPtr->tileRowCount; r++) {
        for (c = 0; c < initDataPtr->tileColumnCount; c++) {
            tileIdx = r * initDataPtr->tileColumnCount + c;
            for (EB_U8 depth = 0; depth < NEIGHBOR_ARRAY_TOTAL_COUNT; depth++) {
                EB_NEW(
                    objectPtr->mdIntraLumaModeNeighborArray[depth][tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE,
                    MAX_PICTURE_HEIGHT_SIZE,
                    sizeof(EB_U8),
                    PU_NEIGHBOR_ARRAY_GRANULARITY,
                    PU_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

                EB_NEW(
                    objectPtr->mdMvNeighborArray[depth][tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE,
                    MAX_PICTURE_HEIGHT_SIZE,
                    sizeof(MvUnit_t),
                    PU_NEIGHBOR_ARRAY_GRANULARITY,
                    PU_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                EB_NEW(
                    objectPtr->mdSkipFlagNeighborArray[depth][tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE,
                    MAX_PICTURE_HEIGHT_SIZE,
                    sizeof(EB_U8),
                    CU_NEIGHBOR_ARRAY_GRANULARITY,
                    CU_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

                EB_NEW(
                    objectPtr->mdModeTypeNeighborArray[depth][tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE,
                    MAX_PICTURE_HEIGHT_SIZE,
                    sizeof(EB_U8),
                    PU_NEIGHBOR_ARRAY_GRANULARITY,
                    PU_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                EB_NEW(
                    objectPtr->mdLeafDepthNeighborArray[depth][tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE,
                    MAX_PICTURE_HEIGHT_SIZE,
                    sizeof(EB_U8),
                    CU_NEIGHBOR_ARRAY_GRANULARITY,
                    CU_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

                EB_NEW(
                    objectPtr->mdLumaReconNeighborArray[depth][tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE,
                    MAX_PICTURE_HEIGHT_SIZE,
                    sizeof(EB_U8),
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                EB_NEW(
                    objectPtr->mdCbReconNeighborArray[depth][tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE >> 1,
                    MAX_PICTURE_HEIGHT_SIZE >> 1,
                    sizeof(EB_U8),
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                EB_NEW(
                    objectPtr->mdCrReconNeighborArray[depth][tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE >> 1,
                    MAX_PICTURE_HEIGHT_SIZE >> 1,
                    sizeof(EB_U8),
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);
            }

            EB_NEW(
                objectPtr->mdRefinementIntraLumaModeNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

            EB_NEW(
                objectPtr->mdRefinementModeTypeNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            EB_NEW(
                objectPtr->mdRefinementLumaReconNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            // Encode Pass Neighbor Arrays
            EB_NEW(
                objectPtr->epIntraLumaModeNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE, //Jing: change to tile size
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

            EB_NEW(
                objectPtr->epMvNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(MvUnit_t),
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            EB_NEW(
                objectPtr->epSkipFlagNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                CU_NEIGHBOR_ARRAY_GRANULARITY,
                CU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

            EB_NEW(
                objectPtr->epModeTypeNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            EB_NEW(
                objectPtr->epLeafDepthNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                CU_NEIGHBOR_ARRAY_GRANULARITY,
                CU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

            EB_NEW(
                objectPtr->epLumaReconNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            EB_NEW(
                objectPtr->epCbReconNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE >> subWidthCMinus1,
                MAX_PICTURE_HEIGHT_SIZE >> subHeightCMinus1,
                sizeof(EB_U8),
                SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            EB_NEW(
                objectPtr->epCrReconNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE >> subWidthCMinus1,
                MAX_PICTURE_HEIGHT_SIZE >> subHeightCMinus1,
                sizeof(EB_U8),
                SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_FULL_MASK);

            if (is16bit) {
                EB_NEW(
                    objectPtr->epLumaReconNeighborArray16bit[tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE,
                    MAX_PICTURE_HEIGHT_SIZE,
                    sizeof(EB_U16),
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                EB_NEW(
                    objectPtr->epCbReconNeighborArray16bit[tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE >> subWidthCMinus1,
                    MAX_PICTURE_HEIGHT_SIZE >> subHeightCMinus1,
                    sizeof(EB_U16),
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);

                EB_NEW(
                    objectPtr->epCrReconNeighborArray16bit[tileIdx],
                    NeighborArrayUnitCtor,
                    MAX_PICTURE_WIDTH_SIZE >> subWidthCMinus1,
                    MAX_PICTURE_HEIGHT_SIZE >> subHeightCMinus1,
                    sizeof(EB_U16),
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    SAMPLE_NEIGHBOR_ARRAY_GRANULARITY,
                    NEIGHBOR_ARRAY_UNIT_FULL_MASK);
            }

            EB_NEW(
                objectPtr->epSaoNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(SaoParameters_t),
                LCU_NEIGHBOR_ARRAY_GRANULARITY,
                LCU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);


            // Entropy Coding Neighbor Arrays
            EB_NEW(
                objectPtr->modeTypeNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

            EB_NEW(
                objectPtr->leafDepthNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                CU_NEIGHBOR_ARRAY_GRANULARITY,
                CU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

            EB_NEW(
                objectPtr->skipFlagNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                CU_NEIGHBOR_ARRAY_GRANULARITY,
                CU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

            EB_NEW(
                objectPtr->intraLumaModeNeighborArray[tileIdx],
                NeighborArrayUnitCtor,
                MAX_PICTURE_WIDTH_SIZE,
                MAX_PICTURE_HEIGHT_SIZE,
                sizeof(EB_U8),
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                PU_NEIGHBOR_ARRAY_GRANULARITY,
                NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);
        }
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

    //Jing:
    //Alloc segment per tile group
    // Segments
    EB_ALLOC_PTR_ARRAY(objectPtr->encDecSegmentCtrl, tileGroupCnt);

    for (EB_U32 tileGroupIdx = 0; tileGroupIdx < tileGroupCnt; tileGroupIdx++) {
        //Can reduce encDecSegCol and encDecSegRow a bit to save memory
        EB_NEW(
            objectPtr->encDecSegmentCtrl[tileGroupIdx],
            EncDecSegmentsCtor,
            encDecSegCol,
            encDecSegRow);
    }

    EB_ALLOC_PTR_ARRAY(objectPtr->entropyCodingInfo, totalTileCount);
    for (tileIdx = 0; tileIdx < totalTileCount; tileIdx++) {
        // Entropy Rows per tile
        EB_MALLOC(objectPtr->entropyCodingInfo[tileIdx], sizeof(EntropyTileInfo));
        EB_CREATE_MUTEX(objectPtr->entropyCodingInfo[tileIdx]->entropyCodingMutex);

        // Entropy Coder
        EB_NEW(
            objectPtr->entropyCodingInfo[tileIdx]->entropyCoderPtr,
            EntropyCoderCtor,
            SEGMENT_ENTROPY_BUFFER_SIZE);
    }

    // Entropy picture level mutex
    EB_CREATE_MUTEX(objectPtr->entropyCodingPicMutex);

    EB_CREATE_MUTEX(objectPtr->intraMutex);

    objectPtr->encDecCodedLcuCount = 0;
    objectPtr->resetDone = EB_FALSE;

    return EB_ErrorNone;
}


EB_ERRORTYPE PictureControlSetCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    PictureControlSet_t   *objectPtr;

    *objectDblPtr = NULL;
    EB_NEW(objectPtr, PictureControlSetCtor, objectInitDataPtr);
    *objectDblPtr = objectPtr;

    return EB_ErrorNone;
}

static void PictureParentControlSetDctor(EB_PTR p)
{
    PictureParentControlSet_t *obj = (PictureParentControlSet_t*)p;

    EB_FREE_ARRAY(obj->tileInfoArray);
    EB_FREE_ARRAY(obj->tileGroupInfoArray);

    if(obj->isChromaDownSamplePictureOwner)
        EB_DELETE(obj->chromaDownSamplePicturePtr);

    EB_FREE_2D(obj->variance);
    EB_FREE_2D(obj->yMean);
    EB_FREE_2D(obj->cbMean);
    EB_FREE_2D(obj->crMean);

    EB_FREE_ARRAY(obj->lcuEdgeInfoArray);

    // Histograms
    if (obj->pictureHistogram) {
        for (EB_U32 regionInPictureWidthIndex = 0; regionInPictureWidthIndex < MAX_NUMBER_OF_REGIONS_IN_WIDTH; regionInPictureWidthIndex++) {
            if (obj->pictureHistogram[regionInPictureWidthIndex]) {
                for (EB_U32 regionInPictureHeightIndex = 0; regionInPictureHeightIndex < MAX_NUMBER_OF_REGIONS_IN_HEIGHT; regionInPictureHeightIndex++)
                    EB_FREE_2D(obj->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex]);
            }
        }
        EB_FREE_2D(obj->pictureHistogram);
    }

    for (EB_U16 lcuIndex = 0; lcuIndex < obj->lcuTotalCount; ++lcuIndex) {
        EB_FREE_ARRAY(obj->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[0]);
    }
    EB_FREE_2D(obj->oisCu32Cu16Results);

    for (EB_U16 lcuIndex = 0; lcuIndex < obj->lcuTotalCount; ++lcuIndex) {
        EB_FREE_ARRAY(obj->oisCu8Results[lcuIndex]->sortedOisCandidate[0]);
    }
    EB_FREE_2D(obj->oisCu8Results);

    EB_FREE_2D(obj->meResults);
    EB_FREE_ARRAY(obj->rcMEdistortion);
    EB_FREE_ARRAY(obj->meDistortionHistogram);
    EB_FREE_ARRAY(obj->oisDistortionHistogram);
    EB_FREE_ARRAY(obj->intraSadIntervalIndex);
    EB_FREE_ARRAY(obj->interSadIntervalIndex);
    EB_FREE_ARRAY(obj->zzCostArray);
    EB_FREE_ARRAY(obj->nonMovingIndexArray);
    EB_FREE_ARRAY(obj->similarColocatedLcuArray);

    // similar Colocated Lcu array
    EB_FREE_ARRAY(obj->similarColocatedLcuArrayAllLayers);

    // LCU noise variance array
    EB_FREE_ARRAY(obj->lcuFlatNoiseArray);
    EB_FREE_ARRAY(obj->lcuVarianceOfVarianceOverTime);
    EB_FREE_ARRAY(obj->isLcuHomogeneousOverTime);
    EB_FREE_ARRAY(obj->edgeResultsPtr);
    EB_FREE_ARRAY(obj->sharpEdgeLcuFlag);
    EB_FREE_ARRAY(obj->failingMotionLcuFlag);
    EB_FREE_ARRAY(obj->uncoveredAreaLcuFlag);
    EB_FREE_ARRAY(obj->lcuHomogeneousAreaArray);

    EB_FREE_2D(obj->varOfVar32x32BasedLcuArray);
    EB_FREE_ARRAY(obj->cmplxStatusLcu);
    EB_FREE_ARRAY(obj->lcuIsolatedNonHomogeneousAreaArray);
    EB_FREE_2D(obj->lcuYSrcEnergyCuArray);
    EB_FREE_2D(obj->lcuYSrcMeanCuArray);
    EB_FREE_ARRAY(obj->lcuCmplxContrastArray);
    EB_FREE_ARRAY(obj->lcuStatArray);
    EB_FREE_ARRAY(obj->complexLcuArray);
    EB_FREE_ARRAY(obj->lcuMdModeArray);
    EB_FREE_ARRAY(obj->segmentOvArray);

    EB_DESTROY_MUTEX(obj->rcDistortionHistogramMutex);
}

EB_ERRORTYPE PictureParentControlSetCtor(
    PictureParentControlSet_t *objectPtr,
    EB_PTR objectInitDataPtr)
{
    PictureControlSetInitData_t *initDataPtr    = (PictureControlSetInitData_t*) objectInitDataPtr;

    const EB_U16 pictureLcuWidth    = (EB_U16)((initDataPtr->pictureWidth + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    const EB_U16 pictureLcuHeight   = (EB_U16)((initDataPtr->pictureHeight + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    EB_U16 lcuIndex;

    objectPtr->dctor = PictureParentControlSetDctor;

    // Jing: Tiles
    EB_U32 totalTileCount = initDataPtr->tileRowCount * initDataPtr->tileColumnCount;

    EB_MALLOC_ARRAY(objectPtr->tileInfoArray, totalTileCount);
    EB_MALLOC_ARRAY(objectPtr->tileGroupInfoArray, totalTileCount);

    objectPtr->pictureWidthInLcu = (EB_U16)((initDataPtr->pictureWidth + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    objectPtr->pictureHeightInLcu = (EB_U16)((initDataPtr->pictureHeight + initDataPtr->lcuSize - 1) / initDataPtr->lcuSize);
    objectPtr->tileRowCount = initDataPtr->tileRowCount;
    objectPtr->tileColumnCount = initDataPtr->tileColumnCount;
    objectPtr->tileUniformSpacing = 1;

    for (EB_U16 c = 0; c <= objectPtr->tileColumnCount; c++) {
        objectPtr->tileColStartLcu[c] = c * objectPtr->pictureWidthInLcu / objectPtr->tileColumnCount;
    }
    for (EB_U16 r = 0; r <= objectPtr->tileRowCount; r++) {
        objectPtr->tileRowStartLcu[r] = r * objectPtr->pictureHeightInLcu / objectPtr->tileRowCount;
    }
    ConfigureTileInfo(objectPtr, initDataPtr->lcuSize, initDataPtr->pictureWidth, initDataPtr->pictureHeight);

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
        EB_NEW(
            objectPtr->chromaDownSamplePicturePtr,
            EbPictureBufferDescCtor,
            (EB_PTR)&inputPictureBufferDescInitData);
        objectPtr->isChromaDownSamplePictureOwner = EB_TRUE;
    } else if(initDataPtr->colorFormat == EB_YUV420) {
        objectPtr->chromaDownSamplePicturePtr = EB_NULL;
    } else {
        return EB_ErrorBadParameter;
    }

    // GOP
    objectPtr->lcuTotalCount        = pictureLcuWidth * pictureLcuHeight;

    EB_MALLOC_2D(objectPtr->variance, objectPtr->lcuTotalCount, MAX_ME_PU_COUNT);
    EB_MALLOC_2D(objectPtr->yMean, objectPtr->lcuTotalCount, MAX_ME_PU_COUNT);
    EB_MALLOC_2D(objectPtr->cbMean, objectPtr->lcuTotalCount, 21);
    EB_MALLOC_2D(objectPtr->crMean, objectPtr->lcuTotalCount, 21);

    //LCU edge info
    EB_MALLOC_ARRAY(objectPtr->lcuEdgeInfoArray, objectPtr->lcuTotalCount);
    ConfigureLcuEdgeInfo(objectPtr);

    // Histograms
    EB_CALLOC_2D(objectPtr->pictureHistogram, MAX_NUMBER_OF_REGIONS_IN_WIDTH, MAX_NUMBER_OF_REGIONS_IN_HEIGHT);
    for (EB_U32 regionInPictureWidthIndex = 0; regionInPictureWidthIndex < MAX_NUMBER_OF_REGIONS_IN_WIDTH; regionInPictureWidthIndex++) {  // loop over horizontal regions
        for (EB_U32 regionInPictureHeightIndex = 0; regionInPictureHeightIndex < MAX_NUMBER_OF_REGIONS_IN_HEIGHT; regionInPictureHeightIndex++) { // loop over vertical regions
            EB_MALLOC_2D(objectPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex], 3, HISTOGRAM_NUMBER_OF_BINS);
        }
    }

    EB_U32 maxOisCand = MAX(MAX_OIS_0, MAX_OIS_2);

    EB_MALLOC_2D(objectPtr->oisCu32Cu16Results, objectPtr->lcuTotalCount, 1);
    for (lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {
        OisCandidate_t* contigousCand;
        EB_MALLOC_ARRAY(contigousCand, maxOisCand * 21);
        for (EB_U32 cuIdx = 0; cuIdx < 21; ++cuIdx) {
            objectPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[cuIdx] = &contigousCand[cuIdx*maxOisCand];
        }
    }

    EB_MALLOC_2D(objectPtr->oisCu8Results, objectPtr->lcuTotalCount, 1);
    for (lcuIndex = 0; lcuIndex < objectPtr->lcuTotalCount; ++lcuIndex) {
        OisCandidate_t* contigousCand;
        EB_MALLOC_ARRAY(contigousCand, maxOisCand * 64);
        for (EB_U32 cuIdx = 0; cuIdx < 64; ++cuIdx) {
            objectPtr->oisCu8Results[lcuIndex]->sortedOisCandidate[cuIdx] = &contigousCand[cuIdx*maxOisCand];
        }
    }


    // Motion Estimation Results
    objectPtr->maxNumberOfPusPerLcu         =   SQUARE_PU_COUNT;
    objectPtr->maxNumberOfMeCandidatesPerPU =   3;

    EB_MALLOC_2D(objectPtr->meResults, objectPtr->lcuTotalCount, 85);
    EB_MALLOC_ARRAY(objectPtr->rcMEdistortion, objectPtr->lcuTotalCount);


	// ME and OIS Distortion Histograms
    EB_MALLOC_ARRAY(objectPtr->meDistortionHistogram, NUMBER_OF_SAD_INTERVALS);

    EB_MALLOC_ARRAY(objectPtr->oisDistortionHistogram, NUMBER_OF_INTRA_SAD_INTERVALS);

    EB_MALLOC_ARRAY(objectPtr->intraSadIntervalIndex, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->interSadIntervalIndex, objectPtr->lcuTotalCount);

	// Enhance background for base layer frames:  zz SAD array
	EB_MALLOC_ARRAY(objectPtr->zzCostArray, objectPtr->lcuTotalCount * 64);

	// Non moving index array

    EB_MALLOC_ARRAY(objectPtr->nonMovingIndexArray, objectPtr->lcuTotalCount);

    // similar Colocated Lcu array
    EB_MALLOC_ARRAY(objectPtr->similarColocatedLcuArray, objectPtr->lcuTotalCount);

    // similar Colocated Lcu array
    EB_MALLOC_ARRAY(objectPtr->similarColocatedLcuArrayAllLayers, objectPtr->lcuTotalCount);

    // LCU noise variance array
    EB_MALLOC_ARRAY(objectPtr->lcuFlatNoiseArray, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->lcuVarianceOfVarianceOverTime, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->isLcuHomogeneousOverTime, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->edgeResultsPtr, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->sharpEdgeLcuFlag, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->failingMotionLcuFlag, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->uncoveredAreaLcuFlag, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->lcuHomogeneousAreaArray, objectPtr->lcuTotalCount);

    EB_MALLOC_2D(objectPtr->varOfVar32x32BasedLcuArray, objectPtr->lcuTotalCount, 4);

    EB_MALLOC_ARRAY(objectPtr->cmplxStatusLcu, objectPtr->lcuTotalCount);
    EB_MALLOC_ARRAY(objectPtr->lcuIsolatedNonHomogeneousAreaArray, objectPtr->lcuTotalCount);

    EB_MALLOC_2D(objectPtr->lcuYSrcEnergyCuArray, objectPtr->lcuTotalCount, 5);

    EB_MALLOC_2D(objectPtr->lcuYSrcMeanCuArray, objectPtr->lcuTotalCount, 5);

    EB_MALLOC_ARRAY(objectPtr->lcuCmplxContrastArray, objectPtr->lcuTotalCount);

    EB_MALLOC_ARRAY(objectPtr->lcuStatArray, objectPtr->lcuTotalCount);

    EB_MALLOC_ARRAY(objectPtr->complexLcuArray, objectPtr->lcuTotalCount);

    EB_CREATE_MUTEX(objectPtr->rcDistortionHistogramMutex);

    EB_MALLOC_ARRAY(objectPtr->lcuMdModeArray, objectPtr->lcuTotalCount);

    if (initDataPtr->segmentOvEnabled) {
        EB_MALLOC_ARRAY(objectPtr->segmentOvArray, objectPtr->lcuTotalCount);
    }

    return EB_ErrorNone;
}

EB_ERRORTYPE PictureParentControlSetCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    PictureParentControlSet_t* objectPtr;

    *objectDblPtr = NULL;
    EB_NEW(objectPtr, PictureParentControlSetCtor, objectInitDataPtr);
    *objectDblPtr = objectPtr;

    return EB_ErrorNone;
}
