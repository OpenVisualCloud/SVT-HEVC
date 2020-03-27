/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

// main.cpp
//  -Contructs the following resources needed during the encoding process
//      -memory
//      -threads
//      -semaphores
//      -semaphores
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
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#endif

#ifdef _WIN32
#include <io.h>     /* _setmode() */
#include <fcntl.h>  /* _O_BINARY */
#endif
/***************************************
 * External Functions
 ***************************************/
extern APPEXITCONDITIONTYPE ProcessInputBuffer(
    EbConfig_t             *config,
    EbAppContext_t         *appCallBack);

extern APPEXITCONDITIONTYPE ProcessOutputReconBuffer(
    EbConfig_t             *config,
    EbAppContext_t         *appCallBack);

extern APPEXITCONDITIONTYPE ProcessOutputStreamBuffer(
    EbConfig_t             *config,
    EbAppContext_t         *appCallBack,
    uint8_t           picSendDone);

volatile int32_t keepRunning = 1;

void EventHandler(int32_t dummy) {
    (void)dummy;
    keepRunning = 0;

    // restore default signal handler
    signal(SIGINT, SIG_DFL);
}

void AssignAppThreadGroup(uint8_t targetSocket) {
#ifdef _WIN32
    if (GetActiveProcessorGroupCount() == 2) {
        GROUP_AFFINITY           groupAffinity;
        GetThreadGroupAffinity(GetCurrentThread(), &groupAffinity);
        groupAffinity.Group = targetSocket;
        SetThreadGroupAffinity(GetCurrentThread(), &groupAffinity, NULL);
    }
#else
    (void)targetSocket;
    return;
#endif
}

void* EbAppCreateThread(
    void *threadFunction(void*),
    void *threadContext)
{
    void* threadHandle = NULL;

#ifdef _WIN32
    threadHandle = (void*)CreateThread(
        NULL,                           // default security attributes
        0,                              // default stack size
        (LPTHREAD_START_ROUTINE)threadFunction, // function to be tied to the new thread
        threadContext,                  // context to be tied to the new thread
        0,                              // thread active when created
        NULL);                          // new thread ID
#else
    pthread_attr_t attr;
    struct sched_param param = {
        .sched_priority = 99
    };
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

    threadHandle = (pthread_t*)malloc(sizeof(pthread_t));
    if (threadHandle != NULL) {
        int ret = pthread_create(
            (pthread_t*)threadHandle,     // Thread handle
            &attr,                        // attributes
            threadFunction,               // function to be run by new thread
            threadContext);

        if (ret != 0) {
            if (ret == EPERM) {
                pthread_cancel(*((pthread_t*)threadHandle));
                free(threadHandle);
                threadHandle = (pthread_t*)malloc(sizeof(pthread_t));
                if (threadHandle != NULL) {
                    pthread_create(
                        (pthread_t*)threadHandle,       // Thread handle
                        (const pthread_attr_t*)EB_NULL, // attributes
                        threadFunction,                 // function to be run by new thread
                        threadContext);
                }
            }
        }
    }
    pthread_attr_destroy(&attr);
#endif // _WIN32

    return threadHandle;
}

EB_ERRORTYPE EbAppDestroyThread(
    void* threadHandle)
{
    EB_ERRORTYPE error_return = EB_ErrorNone;
#ifdef _WIN32
    WaitForSingleObject(threadHandle, INFINITE);
    error_return = CloseHandle(threadHandle) ? EB_ErrorNone : EB_ErrorDestroyThreadFailed;
#else
    pthread_join(*((pthread_t*)threadHandle), NULL);
    free(threadHandle);
#endif // _WIN32

    return error_return;
}

typedef struct EbAppThreadContext_s {
    uint32_t         numChannels;
    EbConfig_t     **configs;
    EbAppContext_t **appCallbacks;
} EbAppThreadContext_t;

EB_BOOL inputDone = EB_FALSE;
void* InputWorkerFunc(void *inputPtr)
{
    EbAppThreadContext_t *threadContext = (EbAppThreadContext_t*)inputPtr;
    APPEXITCONDITIONTYPE    exitConditionsInput[MAX_CHANNEL_NUMBER]; // Processing loop exit condition
    while (EB_TRUE) {
        for (int instanceCount = 0; instanceCount < threadContext->numChannels; ++instanceCount) {
            exitConditionsInput[instanceCount] = ProcessInputBuffer(threadContext->configs[instanceCount], threadContext->appCallbacks[instanceCount]);
            if (exitConditionsInput[instanceCount] != APP_ExitConditionNone) {
                inputDone = EB_TRUE;
                return;
            }
        }
    }
}

/***************************************
 * Encoder App Main
 ***************************************/
int32_t main(int32_t argc, char* argv[])
{
    if (GetSVTVersion(argc, argv) || GetHelp(argc, argv))
        return EB_ErrorNone;

#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    // GLOBAL VARIABLES
    EB_ERRORTYPE            return_error = EB_ErrorNone;            // Error Handling
    APPEXITCONDITIONTYPE    exitCondition = APP_ExitConditionNone;    // Processing loop exit condition

    EB_ERRORTYPE            return_errors[MAX_CHANNEL_NUMBER];          // Error Handling
    APPEXITCONDITIONTYPE    exitConditions[MAX_CHANNEL_NUMBER];          // Processing loop exit condition
    APPEXITCONDITIONTYPE    exitConditionsOutput[MAX_CHANNEL_NUMBER];         // Processing loop exit condition
    APPEXITCONDITIONTYPE    exitConditionsRecon[MAX_CHANNEL_NUMBER];         // Processing loop exit condition

    // input processing worker thread
    void *inputWorker = EB_NULL;
    EbAppThreadContext_t inputWorkerContext;

    EB_BOOL                 channelActive[MAX_CHANNEL_NUMBER];

    EbConfig_t             *configs[MAX_CHANNEL_NUMBER];        // Encoder Configuration

    uint32_t                numChannels = 0;
    uint32_t                instanceCount=0;
    EbAppContext_t         *appCallbacks[MAX_CHANNEL_NUMBER];   // Instances App callback data
    signal(SIGINT, EventHandler);
    printf("-------------------------------------------\n");
    printf("SVT-HEVC Encoder\n");

    // Get NumChannels
    numChannels = GetNumberOfChannels(argc, argv);
    if (numChannels == 0)
        return EB_ErrorBadParameter;

    // Initialize config
    for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
        configs[instanceCount] = (EbConfig_t*)malloc(sizeof(EbConfig_t));
        if (!configs[instanceCount])
            return EB_ErrorInsufficientResources;
        EbConfigCtor(configs[instanceCount]);
        return_errors[instanceCount] = EB_ErrorNone;
    }

    // Initialize appCallback
    for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
        appCallbacks[instanceCount] = (EbAppContext_t*)malloc(sizeof(EbAppContext_t));
        if (!appCallbacks[instanceCount])
            return EB_ErrorInsufficientResources;
    }

    for (instanceCount = 0; instanceCount < MAX_CHANNEL_NUMBER; ++instanceCount) {
        exitConditions[instanceCount] = APP_ExitConditionError;         // Processing loop exit condition
        exitConditionsOutput[instanceCount] = APP_ExitConditionError;         // Processing loop exit condition
        exitConditionsRecon[instanceCount] = APP_ExitConditionError;         // Processing loop exit condition
        channelActive[instanceCount] = EB_FALSE;
    }

    // Read all configuration files.
    return_error = ReadCommandLine(argc, argv, configs, numChannels, return_errors);

    // Process any command line options, including the configuration file

    if (return_error == EB_ErrorNone) {

        // Set main thread affinity
        if (configs[0]->targetSocket != -1)
            AssignAppThreadGroup(configs[0]->targetSocket);

        // Init the Encoder
        for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
            if (return_errors[instanceCount] == EB_ErrorNone) {

                configs[instanceCount]->activeChannelCount = numChannels;
                configs[instanceCount]->channelId = instanceCount;

                EbAppStartTime((uint64_t*)&configs[instanceCount]->performanceContext.libStartTime[0], (uint64_t*)&configs[instanceCount]->performanceContext.libStartTime[1]);

                return_errors[instanceCount] = InitEncoder(configs[instanceCount], appCallbacks[instanceCount], instanceCount);
                return_error = (EB_ERRORTYPE)(return_error | return_errors[instanceCount]);
            }
            else {
                channelActive[instanceCount] = EB_FALSE;
            }
        }

        {
            // Start the Encoder
            for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                if (return_errors[instanceCount] == EB_ErrorNone) {
                    return_error = (EB_ERRORTYPE)(return_error & return_errors[instanceCount]);
                    exitConditions[instanceCount] = APP_ExitConditionNone;
                    exitConditionsOutput[instanceCount] = APP_ExitConditionNone;
                    exitConditionsRecon[instanceCount] = configs[instanceCount]->reconFile ? APP_ExitConditionNone : APP_ExitConditionError;
                    channelActive[instanceCount] = EB_TRUE;
                    EbAppStartTime((uint64_t*)&configs[instanceCount]->performanceContext.encodeStartTime[0], (uint64_t*)&configs[instanceCount]->performanceContext.encodeStartTime[1]);
                }
                else {
                    exitConditions[instanceCount] = APP_ExitConditionError;
                    exitConditionsOutput[instanceCount] = APP_ExitConditionError;
                    exitConditionsRecon[instanceCount] = APP_ExitConditionError;
                }

#if DISPLAY_MEMORY
                EB_APP_MEMORY();
#endif
            }

            printf("Encoding          ");
            fflush(stdout);

            inputWorkerContext.numChannels = numChannels;
            inputWorkerContext.configs = configs;
            inputWorkerContext.appCallbacks = appCallbacks;
            inputWorker = EbAppCreateThread(InputWorkerFunc, &inputWorkerContext);

            while (exitCondition == APP_ExitConditionNone) {
                exitCondition = APP_ExitConditionFinished;
                for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                    if (channelActive[instanceCount] == EB_TRUE) {
                        if (exitConditionsRecon[instanceCount] == APP_ExitConditionNone)
                            exitConditionsRecon[instanceCount] = ProcessOutputReconBuffer(
                                configs[instanceCount],
                                appCallbacks[instanceCount]);
                        if (exitConditionsOutput[instanceCount] == APP_ExitConditionNone)
                            exitConditionsOutput[instanceCount] = ProcessOutputStreamBuffer(
                                configs[instanceCount],
                                appCallbacks[instanceCount],
                                inputDone || (exitConditionsRecon[instanceCount] == APP_ExitConditionNone) ? 0 : 1);
                        if (((exitConditionsRecon[instanceCount] == APP_ExitConditionFinished || !configs[instanceCount]->reconFile) && exitConditionsOutput[instanceCount] == APP_ExitConditionFinished) ||
                            ((exitConditionsRecon[instanceCount] == APP_ExitConditionError && configs[instanceCount]->reconFile) || exitConditionsOutput[instanceCount] == APP_ExitConditionError)) {
                            channelActive[instanceCount] = EB_FALSE;
                            if (configs[instanceCount]->reconFile)
                                exitConditions[instanceCount] = (APPEXITCONDITIONTYPE)(exitConditionsRecon[instanceCount] | exitConditionsOutput[instanceCount]);
                            else
                                exitConditions[instanceCount] = (APPEXITCONDITIONTYPE)(exitConditionsOutput[instanceCount]);
                        }
                    }
                }
                // check if all channels are inactive
                for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                    if (channelActive[instanceCount] == EB_TRUE)
                        exitCondition = APP_ExitConditionNone;
                }
            }

            EbAppDestroyThread(inputWorker);

            for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                if (exitConditions[instanceCount] == APP_ExitConditionFinished && return_errors[instanceCount] == EB_ErrorNone) {
                    double frameRate;

                    if ((configs[instanceCount]->frameRateNumerator != 0 && configs[instanceCount]->frameRateDenominator != 0) || configs[instanceCount]->frameRate != 0) {

                        if (configs[instanceCount]->frameRateNumerator && configs[instanceCount]->frameRateDenominator && (configs[instanceCount]->frameRateNumerator != 0 && configs[instanceCount]->frameRateDenominator != 0)) {
                            frameRate = ((double)configs[instanceCount]->frameRateNumerator) / ((double)configs[instanceCount]->frameRateDenominator);
                        }
                        else if (configs[instanceCount]->frameRate > 1000) {
                            // Correct for 16-bit fixed-point fractional precision
                            frameRate = ((double)configs[instanceCount]->frameRate) / (1 << 16);
                        }
                        else {
                            frameRate = (double)configs[instanceCount]->frameRate;
                        }
                        printf("\nSUMMARY --------------------------------- Channel %u  --------------------------------\n", instanceCount + 1);

                        // Interlaced Video
                        if (configs[instanceCount]->interlacedVideo || configs[instanceCount]->separateFields) {
                            printf("Total Fields\t\tFrame Rate\t\tByte Count\t\tBitrate\n");
                        }
                        else {
                            printf("Total Frames\t\tFrame Rate\t\tByte Count\t\tBitrate\n");
                        }
                        printf("%12d\t\t%4.2f fps\t\t%10.0f\t\t%5.2f kbps\n",
                            (int32_t)configs[instanceCount]->performanceContext.frameCount,
                            (double)frameRate,
                            (double)configs[instanceCount]->performanceContext.byteCount,
                            ((double)(configs[instanceCount]->performanceContext.byteCount << 3) * frameRate / (configs[instanceCount]->framesEncoded * 1000)));
                        fflush(stdout);
                    }
                }
            }
            printf("\n");
            fflush(stdout);
        }
        for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
            if (exitConditions[instanceCount] == APP_ExitConditionFinished && return_errors[instanceCount] == EB_ErrorNone) {

                if (configs[instanceCount]->stopEncoder == EB_FALSE) {
                    // Interlaced Video
                    if (configs[instanceCount]->interlacedVideo || configs[instanceCount]->separateFields) {

                        printf("\nChannel %u\nAverage Speed:\t\t%.0f fields per sec\nTotal Encoding Time:\t\t%.0f ms\nTotal Execution Time:\t\t%.2f ms\nAverage Latency:\t%.0f ms\nMax Latency:\t\t%u ms\n",
                            (uint32_t)(instanceCount + 1),
                            configs[instanceCount]->performanceContext.averageSpeed,
                            configs[instanceCount]->performanceContext.totalEncodeTime * 1000,
                            configs[instanceCount]->performanceContext.totalExecutionTime * 1000,
                            configs[instanceCount]->performanceContext.averageLatency,
                            (uint32_t)(configs[instanceCount]->performanceContext.maxLatency));
                    }
                    else {
                        printf("\nChannel %u\nAverage Speed:\t\t%.2f fps\nTotal Encoding Time:\t%.0f ms\nTotal Execution Time:\t%.0f ms\nAverage Latency:\t%.0f ms\nMax Latency:\t\t%u ms\n",
                            (uint32_t)(instanceCount + 1),
                            configs[instanceCount]->performanceContext.averageSpeed,
                            configs[instanceCount]->performanceContext.totalEncodeTime * 1000,
                            configs[instanceCount]->performanceContext.totalExecutionTime * 1000,
                            configs[instanceCount]->performanceContext.averageLatency,
                            (uint32_t)(configs[instanceCount]->performanceContext.maxLatency));

                    }
                }
                else {
                    printf("\nChannel %u Encoding Interrupted\n", (uint32_t)(instanceCount + 1));
                }
            }
            else if (return_errors[instanceCount] == EB_ErrorInsufficientResources) {
                printf("Could not allocate enough memory for channel %u\n", instanceCount + 1);
            }
            else {
                printf("Error encoding at channel %u! Check error log file for more details ... \n", instanceCount + 1);
            }
        }

        // DeInit Encoder
        for (instanceCount = numChannels; instanceCount > 0; --instanceCount) {
            if (return_errors[instanceCount - 1] == EB_ErrorNone)
                return_errors[instanceCount - 1] = DeInitEncoder(appCallbacks[instanceCount - 1], instanceCount - 1);
        }
    }
    else {
        printf("Error in configuration, could not begin encoding! ... \n");
        printf("Run %s --help for a list of options\n", argv[0]);
    }
    // Destruct the App memory variables
    for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
        EbConfigDtor(configs[instanceCount]);
        if (configs[instanceCount])
            free(configs[instanceCount]);
        if (appCallbacks[instanceCount])
            free(appCallbacks[instanceCount]);
    }

    printf("Encoder finished\n");


    return (return_error == 0) ? 0 : 1;
}
