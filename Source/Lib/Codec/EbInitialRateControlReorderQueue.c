/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbInitialRateControlReorderQueue.h"

EB_ERRORTYPE InitialRateControlReorderEntryCtor(   
    InitialRateControlReorderEntry_t   **entryDblPtr,
    EB_U32                          pictureNumber)
{
    EB_MALLOC(InitialRateControlReorderEntry_t*, *entryDblPtr, sizeof(InitialRateControlReorderEntry_t), EB_N_PTR);

    (*entryDblPtr)->pictureNumber       = pictureNumber;
    (*entryDblPtr)->parentPcsWrapperPtr = (EbObjectWrapper_t *)EB_NULL;
    
    return EB_ErrorNone;
}


EB_ERRORTYPE HlRateControlHistogramEntryCtor(   
    HlRateControlHistogramEntry_t   **entryDblPtr,
    EB_U32                          pictureNumber)
{
    EB_CALLOC(HlRateControlHistogramEntry_t*, *entryDblPtr, sizeof(HlRateControlHistogramEntry_t), 1, EB_N_PTR);

    (*entryDblPtr)->pictureNumber       = pictureNumber;
    (*entryDblPtr)->lifeCount           = 0;

    (*entryDblPtr)->parentPcsWrapperPtr = (EbObjectWrapper_t *)EB_NULL;

	// ME and OIS Distortion Histograms
    EB_MALLOC(EB_U16*, (*entryDblPtr)->meDistortionHistogram, sizeof(EB_U16) * NUMBER_OF_SAD_INTERVALS, EB_N_PTR);

    EB_MALLOC(EB_U16*, (*entryDblPtr)->oisDistortionHistogram, sizeof(EB_U16) * NUMBER_OF_INTRA_SAD_INTERVALS, EB_N_PTR);

    return EB_ErrorNone;
}



