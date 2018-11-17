/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureBuffer_h
#define EbPictureBuffer_h

#include <stdio.h> 

#include "EbDefinitions.h"


#ifdef __cplusplus
extern "C" {
#endif
#define PICTURE_BUFFER_DESC_Y_FLAG              (1 << 0)
#define PICTURE_BUFFER_DESC_Cb_FLAG             (1 << 1)
#define PICTURE_BUFFER_DESC_Cr_FLAG             (1 << 2)
#define PICTURE_BUFFER_DESC_LUMA_MASK           PICTURE_BUFFER_DESC_Y_FLAG
#define PICTURE_BUFFER_DESC_CHROMA_MASK         (PICTURE_BUFFER_DESC_Cb_FLAG | PICTURE_BUFFER_DESC_Cr_FLAG)
#define PICTURE_BUFFER_DESC_FULL_MASK           (PICTURE_BUFFER_DESC_Y_FLAG | PICTURE_BUFFER_DESC_Cb_FLAG | PICTURE_BUFFER_DESC_Cr_FLAG)

/************************************
 * EbPictureBufferDesc 
 ************************************/
typedef struct EbPictureBufferDesc_s
{        
	// Buffer Ptrs
	EB_BYTE         bufferY;        // Pointer to the Y luma buffer
	EB_BYTE         bufferCb;       // Pointer to the U chroma buffer
	EB_BYTE         bufferCr;       // Pointer to the V chroma buffer 
	//Bit increment 
	EB_BYTE         bufferBitIncY;  // Pointer to the Y luma buffer Bit increment
	EB_BYTE         bufferBitIncCb; // Pointer to the U chroma buffer Bit increment
	EB_BYTE         bufferBitIncCr; // Pointer to the V chroma buffer Bit increment

	EB_U16          strideY;        // Pointer to the Y luma buffer
	EB_U16          strideCb;       // Pointer to the U chroma buffer
	EB_U16          strideCr;       // Pointer to the V chroma buffer 

	EB_U16          strideBitIncY;  // Pointer to the Y luma buffer Bit increment
	EB_U16          strideBitIncCb; // Pointer to the U chroma buffer Bit increment
	EB_U16          strideBitIncCr; // Pointer to the V chroma buffer Bit increment

	// Picture Parameters
	EB_U16          originX;        // Horizontal padding distance
	EB_U16          originY;        // Vertical padding distance
	EB_U16          width;          // Luma picture width which excludes the padding
	EB_U16          height;         // Luma picture height which excludes the padding
	EB_U16          maxWidth;       // Luma picture width
	EB_U16          maxHeight;      // Luma picture height
	EB_BITDEPTH     bitDepth;       // Pixel Bit Depth

	// Buffer Parameters
	EB_U32          lumaSize;       // Size of the luma buffer
	EB_U32          chromaSize;     // Size of the chroma buffers                
	EB_BOOL         packedFlag;     // Indicates if sample buffers are packed or not

} EbPictureBufferDesc_t;

/************************************
 * EbPictureBufferDesc Init Data
 ************************************/
typedef struct EbPictureBufferDescInitData_s
{
    EB_U16          maxWidth;
    EB_U16          maxHeight;
    EB_BITDEPTH     bitDepth;
    EB_U32          bufferEnableMask;
	EB_U16          leftPadding;
	EB_U16          rightPadding;
	EB_U16          topPadding;
	EB_U16          botPadding;
    EB_BOOL         splitMode;         //ON: allocate 8bit data separately from nbit data
	
} EbPictureBufferDescInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EbPictureBufferDescCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

extern EB_ERRORTYPE EbReconPictureBufferDescCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

#ifdef __cplusplus
}
#endif
#endif // EbPictureBuffer_h