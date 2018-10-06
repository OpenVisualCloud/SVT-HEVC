/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppFifo_h
#define EbAppFifo_h

#include "EbTypes.h"
#include "EbAppTypes.h"

/** The APPCOMMANDTYPE type is used to by the Lib callbacks to signal to
the App that a buffer is ready for reuse.
*/
typedef enum APPCOMMANDTYPE {
    APP_NullCommand = 0,
    APP_InputEmptyThisBuffer,
    APP_OutputStreamFillThisBuffer,
    APP_ExitNoError,
    APP_ExitError
} APPCOMMANDTYPE;

/***************************************
 * App Command Struct
 ***************************************/
typedef struct AppCommandItem_s {
    APPCOMMANDTYPE          command;
    EB_BUFFERHEADERTYPE   *headerPtr;
    EB_U32                 errorCode;
    EB_U8                  instanceIndex;
} AppCommandItem_t;

/***************************************
 * App Command Fifo
 ***************************************/
typedef struct AppCommandFifo_s {
    AppCommandItem_t *buffer;
    unsigned int      head;
    unsigned int      tail;
    unsigned int      size;
    unsigned int      totalBuffersSize;
    void             *mutexHandle;
    void             *semaphoreHandle;
} AppCommandFifo_t;

extern void AppCommandFifoCtor(
    AppCommandFifo_t *fifoPtr, 
	EB_U32 totalBuffersSize);
extern void AppCommandFifoDtor(
    AppCommandFifo_t *fifoPtr);
extern void AppCommandFifoPop(
    AppCommandFifo_t *fifoPtr,
    AppCommandItem_t *dstPtr);
extern void AppCommandFifoPush(
    AppCommandFifo_t *fifoPtr,
    AppCommandItem_t *srcPtr);
extern void WaitForCommandPost(
    AppCommandFifo_t *fifoPtr);

#endif // EbAppFifo_h