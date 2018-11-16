/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbDeblockingAsm_h
#define EbDeblockingAsm_h
#ifdef __cplusplus
extern "C" {
#endif

#include "EbDefinitions.h"

extern void Luma4SampleEdgeDLFCore_SSSE3(
    EB_BYTE                edgeStartSample, 
    EB_U32                 reconLumaPicStride,     
    EB_BOOL                isVerticalEdge,         
    EB_S32                 tc,                    
    EB_S32                 beta);

void  Chroma2SampleEdgeDLFCore_SSSE3(
    EB_BYTE                edgeStartSampleCb,          
    EB_BYTE                edgeStartSampleCr,           
    EB_U32                 reconChromaPicStride,        
    EB_BOOL                isVerticalEdge,             
    EB_U8                  cbTc,                        
    EB_U8                  crTc);

void Luma4SampleEdgeDLFCore16bit_SSSE3_INTRIN(
	EB_U16         *edgeStartFilteredSamplePtr,          
    EB_U32          reconLumaPicStride,                   
    EB_BOOL         isVerticalEdge,          
    EB_S32          tc,                     
    EB_S32          beta); 

#ifdef __cplusplus
}
#endif
#endif // EbDeblockingAsm_h
