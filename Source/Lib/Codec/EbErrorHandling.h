/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbErrorHandling_h
#define EbErrorHandling_h

#include "EbErrorCodes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define   CHECK_REPORT_ERROR(cond, appCallbackPtr, errorCode)  { if(!(cond)){(appCallbackPtr)->ErrorHandler(((appCallbackPtr)->handle),(errorCode));while(1);}  }

#define   CHECK_REPORT_ERROR_NC(appCallbackPtr, errorCode)     { {(appCallbackPtr)->ErrorHandler(((appCallbackPtr)->handle),(errorCode));while(1);} }

#ifdef __cplusplus
}
#endif
#endif // EbErrorHandling_h
