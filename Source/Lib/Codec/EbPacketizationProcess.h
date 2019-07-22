/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPacketization_h
#define EbPacketization_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Type Declarations
 **************************************/
typedef struct EbPPSConfig_s
{
    EB_U8       ppsId;
    EB_U8       constrainedFlag;
    
} EbPPSConfig_t;

/**************************************
 * Context
 **************************************/
typedef struct PacketizationContext_s
{
    EbFifo_t                *entropyCodingInputFifoPtr;
    EbFifo_t                *rateControlTasksOutputFifoPtr;
    EbPPSConfig_t           *ppsConfig;
    EbFifo_t                *pictureManagerOutputFifoPtr;   // to picture-manager
    
} PacketizationContext_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE PacketizationContextCtor(
    PacketizationContext_t **contextDblPtr,
    EbFifo_t                *entropyCodingInputFifoPtr,
    EbFifo_t                *rateControlTasksOutputFifoPtr,
    EbFifo_t                *pictureManagerOutputFifoPtr
);
    
    
extern void* PacketizationKernel(void *inputPtr);
#ifdef __cplusplus
}
#endif
#endif // EbPacketization_h
