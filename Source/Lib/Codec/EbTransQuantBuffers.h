/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransQuantBuffers_h
#define EbTransQuantBuffers_h

#include "EbDefinitions.h"
#include "EbPictureBufferDesc.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct EbTransQuantBuffers_s
{
    EbDctor                        dctor;
	EbPictureBufferDesc_t         *tuTransCoeff2Nx2NPtr;
	EbPictureBufferDesc_t         *tuTransCoeffNxNPtr;
	EbPictureBufferDesc_t         *tuTransCoeffN2xN2Ptr;
	EbPictureBufferDesc_t         *tuQuantCoeffNxNPtr;
	EbPictureBufferDesc_t         *tuQuantCoeffN2xN2Ptr;

} EbTransQuantBuffers_t;

  
extern EB_ERRORTYPE EbTransQuantBuffersCtor(
	EbTransQuantBuffers_t			*transQuantBuffersPtr);  
    
   
#ifdef __cplusplus
}
#endif
#endif // EbTransQuantBuffers_h