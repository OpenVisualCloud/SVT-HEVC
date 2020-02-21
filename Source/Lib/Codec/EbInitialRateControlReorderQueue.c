/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbInitialRateControlReorderQueue.h"

EB_ERRORTYPE InitialRateControlReorderEntryCtor(
    InitialRateControlReorderEntry_t   *entryPtr,
    EB_U32                          pictureNumber)
{
    entryPtr->pictureNumber       = pictureNumber;
    return EB_ErrorNone;
}

static void HlRateControlHistogramEntryDctor(EB_PTR p)
{
    HlRateControlHistogramEntry_t* obj = (HlRateControlHistogramEntry_t*)p;
    EB_FREE_ARRAY(obj->meDistortionHistogram);
    EB_FREE_ARRAY(obj->oisDistortionHistogram);
}

EB_ERRORTYPE HlRateControlHistogramEntryCtor(
    HlRateControlHistogramEntry_t  *entryPtr,
    EB_U32                          pictureNumber)
{
    entryPtr->dctor = HlRateControlHistogramEntryDctor;
    entryPtr->pictureNumber       = pictureNumber;

    // ME and OIS Distortion Histograms
    EB_MALLOC_ARRAY(entryPtr->meDistortionHistogram, NUMBER_OF_SAD_INTERVALS);
    EB_MALLOC_ARRAY(entryPtr->oisDistortionHistogram, NUMBER_OF_INTRA_SAD_INTERVALS);

    return EB_ErrorNone;
}



