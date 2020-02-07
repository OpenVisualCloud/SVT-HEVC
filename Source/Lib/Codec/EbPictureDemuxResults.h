/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureResults_h
#define EbPictureResults_h

#include "EbSystemResourceManager.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Enums
 **************************************/
typedef enum EB_PIC_TYPE {
    EB_PIC_INVALID = 0,
    EB_PIC_INPUT = 1,
    EB_PIC_REFERENCE = 2,
    EB_PIC_FEEDBACK = 3
} EB_PIC_TYPE;

/**************************************
 * Picture Demux Results
 **************************************/
typedef struct PictureDemuxResults_s
{
    EbDctor                        dctor;
    EB_PIC_TYPE                    pictureType;
    
    // Only valid for input pictures
    EbObjectWrapper_t             *pictureControlSetWrapperPtr;
    
    // Only valid for reference pictures
    EbObjectWrapper_t             *referencePictureWrapperPtr;
    EbObjectWrapper_t             *sequenceControlSetWrapperPtr;
    EB_U64                         pictureNumber;

} PictureDemuxResults_t;

typedef struct PictureResultInitData_s
{
    int junk;
} PictureResultInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE PictureResultsCreator(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

#ifdef __cplusplus
}
#endif
#endif //EbPictureResults_h
