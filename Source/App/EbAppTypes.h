/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppTypes_h
#define EbAppTypes_h

/** The APPCOMMANDTYPE type is used to by the Lib callbacks to signal to
    the App that a buffer is ready for reuse.
 */
typedef enum APPCOMMANDTYPE {
    APP_NullCommand = 0,
    APP_InputEmptyThisBuffer,
    APP_OutputStreamFillThisBuffer,
    APP_FeedBackIsComplete,
    APP_ExitNoError,
    APP_ExitError
} APPCOMMANDTYPE;

/** The APPEXITCONDITIONTYPE type is used to define the App main loop exit
    conditions.
 */
typedef enum APPEXITCONDITIONTYPE {
    APP_ExitConditionNone = 0,
    APP_ExitConditionFinished,
    APP_ExitConditionError
} APPEXITCONDITIONTYPE;

/** The APPPORTACTIVETYPE type is used to define the state of output ports in
    the App.
 */
typedef enum APPPORTACTIVETYPE {
    APP_PortActive = 0,
    APP_PortInactive
} APPPORTACTIVETYPE;

/***************************************
* Defines
***************************************/
#define APP_ENCODERRECONBUFFERCOUNT     5
#define APP_TOTALBUFFERS				APP_ENCODERRECONBUFFERCOUNT

#endif // EbAppTypes_h