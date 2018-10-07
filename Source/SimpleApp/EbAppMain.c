/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

// main.cpp
//  -Contructs the following resources needed during the encoding process
//      -memory
//      -threads
//  -Configures the encoder
//  -Calls the encoder via the API
//  -Destructs the resources

/***************************************
 * Includes
 ***************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "EbAppConfig.h"
#include "EbAppContext.h"
#include "EbTime.h"
#if !__linux
#include <Windows.h>
#endif


 /**********************************
 * Constructor
 **********************************/
void EbConfigCtor(EbConfig_t *configPtr)
{
    configPtr->inputFile = NULL;
    configPtr->bitstreamFile = NULL;;
    configPtr->encoderBitDepth = 8;
    configPtr->sourceWidth = 0;
    configPtr->sourceHeight = 0;
    configPtr->framesToBeEncoded = 0;
    configPtr->channelId = 0;
    configPtr->stopEncoder = EB_FALSE;

    return;
}

/**********************************
* Destructor
**********************************/
void EbConfigDtor(EbConfig_t *configPtr)
{

    if (configPtr->inputFile) {
        fclose(configPtr->inputFile);
        configPtr->inputFile = (FILE *)NULL;
    }

    if (configPtr->bitstreamFile) {
        fclose(configPtr->bitstreamFile);
        configPtr->bitstreamFile = (FILE *)NULL;
    }

    return;
}

 /****************************************
 * EbCreateThread
 ****************************************/
EB_HANDLETYPE EbCreateThread(
    void *threadFunction(void *),
    void *threadContext)
{
    EB_HANDLETYPE threadHandle = NULL;

#ifdef _WIN32

    threadHandle = (EB_HANDLETYPE)CreateThread(
        NULL,                           // default security attributes
        0,                              // default stack size
        (LPTHREAD_START_ROUTINE)threadFunction, // function to be tied to the new thread
        threadContext,                  // context to be tied to the new thread
        0,                              // thread active when created
        NULL);                          // new thread ID

#elif __linux__

    pthread_attr_t attr;
    struct sched_param param = {
        .sched_priority = 99
    };
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);

    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    threadHandle = (pthread_t*)malloc(sizeof(pthread_t));
    int ret = pthread_create(
        (pthread_t*)threadHandle,      // Thread handle
        &attr,                       // attributes
        threadFunction,                 // function to be run by new thread
        threadContext);

    if (ret != 0)
        if (ret == EPERM) {

            pthread_cancel(*((pthread_t*)threadHandle));
            pthread_join(*((pthread_t*)threadHandle), NULL);
            free(threadHandle);

            threadHandle = (pthread_t*)malloc(sizeof(pthread_t));

            pthread_create(
                (pthread_t*)threadHandle,      // Thread handle
                (const pthread_attr_t*)EB_NULL,                        // attributes
                threadFunction,                 // function to be run by new thread
                threadContext);
        }

#endif // _WIN32

    return threadHandle;
}

/****************************************
* EbDestroyThread
****************************************/
EB_ERRORTYPE EbDestroyThread(
    EB_HANDLETYPE threadHandle)
{
    EB_ERRORTYPE error_return = EB_ErrorNone;

#ifdef _WIN32
    error_return = TerminateThread((HANDLE)threadHandle, 0) ? EB_ErrorDestroyThreadFailed : EB_ErrorNone;
#elif __linux__
    error_return = pthread_cancel(*((pthread_t*)threadHandle)) ? EB_ErrorDestroyThreadFailed : EB_ErrorNone;
    pthread_join(*((pthread_t*)threadHandle), NULL);
    free(threadHandle);
#endif // _WIN32

    return error_return;
}

/****************************************
* EbWaitThread
****************************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_ERRORTYPE EbWaitThread(
    EB_HANDLETYPE threadHandle)
{
    EB_ERRORTYPE error_return = EB_ErrorNone;

#ifdef _WIN32
    error_return = WaitForSingleObject((HANDLE)threadHandle, INFINITE) ? EB_ErrorThreadUnresponsive : EB_ErrorNone;
#elif __linux__
    pthread_join(*((pthread_t*)threadHandle), NULL);
    free(threadHandle);
#endif // _WIN32

    return error_return;
}
volatile int keepRunning = 1; // to stop the encoder with a CTR+C signal
void EventHandler(int dummy) {
    (void)dummy;
    keepRunning = 0;
}
// Threads
void* ReceiveBitstream(void* arg);
void* SendPictures(void* arg);

// GLOBAL VARIABLES
EB_ERRORTYPE            return_error = EB_ErrorNone;            // Error Handling
APPEXITCONDITIONTYPE    exitCondition = APP_ExitConditionNone;    // Processing loop exit condition
APPEXITCONDITIONTYPE    exitConditionOutput = APP_ExitConditionNone;    // Processing loop exit condition
APPEXITCONDITIONTYPE    exitConditionInput = APP_ExitConditionNone;    // Processing loop exit condition
EbConfig_t             *config;        // Encoder Configuration

/***************************************
 * Encoder App Main
 ***************************************/
int main(int argc, char* argv[])
{

    EbAppContext_t         *appCallback;   // Instances App callback data
    EB_HANDLETYPE           outputStreamHandle; // receive thread
    EB_HANDLETYPE           inputYuvHandle;     // send thread

    signal(SIGINT, EventHandler);

    // Print Encoder Info
    printf("-------------------------------------\n");
    printf("SVT-HEVC Encoder Simple Sample Application v1.2.0\n");
    printf("Platform:   %u bit\n", (unsigned) sizeof(void*)*8);
#if ( defined( _MSC_VER ) && (_MSC_VER < 1910) ) 
	printf("Compiler: VS13\n");
#elif ( defined( _MSC_VER ) && (_MSC_VER >= 1910) ) 
	printf("Compiler: VS17\n");
#elif defined(__INTEL_COMPILER)
	printf("Compiler: Intel\n");
#elif defined(__GNUC__)
	printf("Compiler: GCC\n");
#else
	printf("Compiler: unknown\n");
#endif

    printf("APP Build date: %s %s\n",__DATE__,__TIME__);
    fflush(stdout);
    {
        // Initialize config
        config = (EbConfig_t*)malloc(sizeof(EbConfig_t));
        EbConfigCtor(config);
        if (argc != 6) {
            printf("Usage: ./HevcEncoderSimpleApp in.yuv out.265 width height bitdepth \n");
            return_error = EB_ErrorBadParameter;
        }
        else {
            // Get info for config
            FILE * fin = fopen(argv[1], "rb");
            if (!fin) {
                printf("Invalid input file \n");
                return_error = EB_ErrorBadParameter;
            }
            else
                config->inputFile = fin;

            FILE * fout = fopen(argv[2], "wb");
            if (!fout) {
                printf("Invalid input file \n");
                return_error = EB_ErrorBadParameter;
            }
            else
                config->bitstreamFile = fout;

            EB_U32 width = 0, height = 0;
            
            width = strtoul(argv[3], NULL, 0);
            height = strtoul(argv[4], NULL, 0);
            if ((width&&height) == 0) { printf("Invalid video dimensions\n"); return_error = EB_ErrorBadParameter; }

            config->inputPaddedWidth  = config->sourceWidth = width;
            config->inputPaddedHeight = config->sourceHeight = height;

            EB_U32 bdepth = width = strtoul(argv[5], NULL, 0);
            if ((bdepth != 8) && (bdepth != 10)) {printf("Invalid bit depth\n"); return_error = EB_ErrorBadParameter; }
            config->encoderBitDepth = bdepth;
        }
        if (return_error == EB_ErrorNone) {

            // Initialize appCallback
            appCallback = (EbAppContext_t*)malloc(sizeof(EbAppContext_t));
            EbAppContextCtor(appCallback,config);
            
            return_error = InitEncoder(config, appCallback, 0);
            return_error = EbStartEncoder(appCallback->svtEncoderHandle, 0);

            printf("Encoding          ");
            fflush(stdout);

            outputStreamHandle  = EbCreateThread(ReceiveBitstream,  (void*)(appCallback));
            inputYuvHandle      = EbCreateThread(SendPictures,      (void*)(appCallback)); // one extra thread to be removed later

            EbWaitThread (inputYuvHandle);
            EbWaitThread (outputStreamHandle);

            EbStopEncoder(appCallback->svtEncoderHandle, 0);
            exitCondition = (APPEXITCONDITIONTYPE)(exitConditionOutput || exitConditionInput);

            printf("\n");
            fflush(stdout);

            // DeInit Encoder
            return_error = DeInitEncoder(appCallback, 0, return_error);

            // Destruct the App memory variables
            EbConfigDtor(config);
            free(config);
            free(appCallback);
        }
        else {
            printf("Error in configuration, could not begin encoding! ... \n");
        }

   }
        printf("Encoder finished\n");

    

    return (return_error == 0) ? 0 : 1;
}

APPEXITCONDITIONTYPE ProcessOutputStreamBuffer(
    EbConfig_t             *config,
    EB_BUFFERHEADERTYPE    *headerPtr,
    EB_COMPONENTTYPE       *componentHandle
)
{
    APPEXITCONDITIONTYPE    return_value = APP_ExitConditionNone;

    // System performance variables
    static int           frameCount = 0;

    // blocking call
    EbH265GetPacket((EB_HANDLETYPE)componentHandle, headerPtr);

    fwrite(headerPtr->pBuffer + headerPtr->nOffset, 1, headerPtr->nFilledLen, config->bitstreamFile);

    // Update Output Port Activity State
    return_value = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : APP_ExitConditionNone;
    printf("\b\b\b\b\b\b\b\b\b%9d", ++frameCount);

    fflush(stdout);

    return return_value;
}

void* ReceiveBitstream(void* arg)
{
    EbAppContext_t         *appCallBack = (EbAppContext_t *)arg;
    EB_BUFFERHEADERTYPE    *headerPtr = appCallBack->outputStreamBuffer;

    // Input Loop Thread
    exitConditionInput = APP_ExitConditionNone;
    while (exitConditionInput == APP_ExitConditionNone) {
        exitConditionInput = ProcessOutputStreamBuffer(config, headerPtr, (EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle);
    }
    return NULL;
}
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height,is16bit) ( ( ((width)*(height)*3)>>1 )<<is16bit)
void ReadInputFrames(
    EbConfig_t                  *config,
    unsigned char                is16bit,
    EB_BUFFERHEADERTYPE         *headerPtr)
{

    EB_U64  readSize;
    EB_S64  inputPaddedWidth = config->inputPaddedWidth;
    EB_S64  inputPaddedHeight = config->inputPaddedHeight;
    FILE   *inputFile = config->inputFile;
    EB_U8  *ebInputPtr;
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;

    {
        if (is16bit == 0 || (is16bit == 1 && config->compressedTenBitFormat == 0)) {

            readSize = (EB_U64)SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, is16bit);

            headerPtr->nFilledLen = 0;

            {
                EB_U64 lumaReadSize = (EB_U64)inputPaddedWidth*inputPaddedHeight << is16bit;
                ebInputPtr = inputPtr->luma;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
                ebInputPtr = inputPtr->cb;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                ebInputPtr = inputPtr->cr;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING) << is16bit);
                inputPtr->cb = inputPtr->cb + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)) << is16bit);
                inputPtr->cr = inputPtr->cr + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)) << is16bit);


                if (readSize != headerPtr->nFilledLen) {
                    config->stopEncoder = EB_TRUE;
                }
            }
        }
        // 10-bit Compressed Unpacked Mode
        else if (is16bit == 1 && config->compressedTenBitFormat == 1) {

            // Fill the buffer with a complete frame
            headerPtr->nFilledLen = 0;


            EB_U64 lumaReadSize = (EB_U64)inputPaddedWidth*inputPaddedHeight;
            EB_U64 nbitlumaReadSize = (EB_U64)(inputPaddedWidth / 4)*inputPaddedHeight;

            ebInputPtr = inputPtr->luma;
            headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
            ebInputPtr = inputPtr->cb;
            headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
            ebInputPtr = inputPtr->cr;
            headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);

            inputPtr->luma = inputPtr->luma + config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING;
            inputPtr->cb = inputPtr->cb + (config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);
            inputPtr->cr = inputPtr->cr + (config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);


            ebInputPtr = inputPtr->lumaExt;
            headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, nbitlumaReadSize, inputFile);
            ebInputPtr = inputPtr->cbExt;
            headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, nbitlumaReadSize >> 2, inputFile);
            ebInputPtr = inputPtr->crExt;
            headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, nbitlumaReadSize >> 2, inputFile);

            inputPtr->lumaExt = inputPtr->lumaExt + ((config->inputPaddedWidth >> 2)*TOP_INPUT_PADDING + (LEFT_INPUT_PADDING >> 2));
            inputPtr->cbExt = inputPtr->cbExt + (((config->inputPaddedWidth >> 1) >> 2)*(TOP_INPUT_PADDING >> 1) + ((LEFT_INPUT_PADDING >> 1) >> 2));
            inputPtr->crExt = inputPtr->crExt + (((config->inputPaddedWidth >> 1) >> 2)*(TOP_INPUT_PADDING >> 1) + ((LEFT_INPUT_PADDING >> 1) >> 2));

            readSize = ((lumaReadSize * 3) >> 1) + ((nbitlumaReadSize * 3) >> 1);

            if (readSize != headerPtr->nFilledLen) {
                config->stopEncoder = EB_TRUE;
            }

        }
    }
    // If we reached the end of file, loop over again
    if (feof(inputFile) != 0) {
        //fseek(inputFile, 0, SEEK_SET);
        config->stopEncoder = EB_TRUE;
    }

    return;
}

APPEXITCONDITIONTYPE ProcessInputBuffer(
    EbConfig_t                  *config,
    EB_BUFFERHEADERTYPE         *headerPtr,
    EB_COMPONENTTYPE            *componentHandle)
{
    unsigned char                is16bit = (unsigned char)(config->encoderBitDepth > 8);
    APPEXITCONDITIONTYPE         return_value = APP_ExitConditionNone;
    if (config->stopEncoder == EB_FALSE) {
        ReadInputFrames(
            config,
            is16bit,
            headerPtr);
        if (keepRunning == 0 && !config->stopEncoder) {
            config->stopEncoder = EB_TRUE;
        }
        if (config->stopEncoder == EB_FALSE) {
            // Fill in Buffers Header control data
            headerPtr->nOffset = 0;
            headerPtr->nTimeStamp = 0;
            headerPtr->nFlags = 0;
            headerPtr->pAppPrivate = (EB_PTR)EB_NULL;
            headerPtr->nFlags = 0;

            // Send the picture
            EbH265EncSendPicture((EB_HANDLETYPE)componentHandle, headerPtr);
        }
        else {

            headerPtr->nAllocLen = 0;
            headerPtr->nFilledLen = 0;
            headerPtr->nTickCount = 0;
            headerPtr->pAppPrivate = NULL;
            headerPtr->nOffset = 0;
            headerPtr->nTimeStamp = 0;
            headerPtr->nFlags = EB_BUFFERFLAG_EOS;
            headerPtr->pBuffer = NULL;

            EbH265EncSendPicture((EB_HANDLETYPE)componentHandle, headerPtr);
        }
        return_value = (headerPtr->nFlags == EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : return_value;
    }
    return return_value;
}

void* SendPictures(void* arg)
{
    EbAppContext_t         *appCallBack = (EbAppContext_t *)arg;
    EB_BUFFERHEADERTYPE    *headerPtr = appCallBack->inputPictureBuffer;

    // Input Loop Thread
    exitConditionInput = APP_ExitConditionNone;
    while (exitConditionInput == APP_ExitConditionNone) {
        exitConditionInput = ProcessInputBuffer(config, headerPtr,(EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle);
    }
    return NULL;
}
