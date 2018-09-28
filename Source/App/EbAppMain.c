/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

// main.cpp
//  -Contructs the following resources needed during the encoding process
//      -memory
//      -threads
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
#if !__linux
#include <Windows.h>
#endif


/***************************************
 * External Functions
 ***************************************/
extern APPEXITCONDITIONTYPE AppProcessCommands(
    EbConfig_t **configs,
    EbParentAppContext_t *appCallback,
    EB_U64 *encodingFinishTimesSeconds,
    EB_U64 *encodingFinishTimesuSeconds,
    APPEXITCONDITIONTYPE *exitConditions);



volatile int keepRunning = 1;

void EventHandler(int dummy) {
    (void)dummy;
    keepRunning = 0;
}

void AssignAppThreadGroup(EB_U8 targetSocket) {
#ifdef _MSC_VER
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

/***************************************
 * Encoder App Main
 ***************************************/
int main(int argc, char* argv[])
{
    EB_ERRORTYPE           return_error    = EB_ErrorNone;            // Error Handling
    APPEXITCONDITIONTYPE    exitCondition   = APP_ExitConditionNone;    // Processing loop exit condition

    EB_ERRORTYPE           return_errors[MAX_CHANNEL_NUMBER];          // Error Handling
    APPEXITCONDITIONTYPE    exitConditions[MAX_CHANNEL_NUMBER];         // Processing loop exit condition

    EB_BOOL                 channelActive[MAX_CHANNEL_NUMBER];

    EbConfig_t             *configs[MAX_CHANNEL_NUMBER];        // Encoder Configuration
    EbAppContext_t         *appCallbacks[MAX_CHANNEL_NUMBER];   // Instances App callback data
    EbParentAppContext_t   *parentAppCallBack = NULL;           // App callback context

    EB_U64                  encodingStartTimesSeconds[MAX_CHANNEL_NUMBER]; // Array holding start time of each instance
    EB_U64                  encodingFinishTimesSeconds[MAX_CHANNEL_NUMBER]; // Array holding finish time of each instance
    EB_U64                  encodingStartTimesuSeconds[MAX_CHANNEL_NUMBER]; // Array holding start time of each instance
    EB_U64                  encodingFinishTimesuSeconds[MAX_CHANNEL_NUMBER]; // Array holding finish time of each instance
    unsigned int            instanceCount=0, numChannels=0;
    
    signal(SIGINT, EventHandler);

    // Print Encoder Info
    printf("-------------------------------------\n");
    printf("SVT-HEVC Encoder v1.2.0\n");
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

    if (!GetHelp(argc, argv)) {

        // Get NumChannels
        numChannels = GetNumberOfChannels(argc, argv);
        if (numChannels == 0) {
            return EB_ErrorBadParameter;
        }

        // Initialize config
        for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
            configs[instanceCount] = (EbConfig_t*)malloc(sizeof(EbConfig_t));
            EbConfigCtor(configs[instanceCount]);
            return_errors[instanceCount] = EB_ErrorNone;
        }

        // Initialize appCallback
        for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
            appCallbacks[instanceCount] = (EbAppContext_t*)malloc(sizeof(EbAppContext_t));
            EbAppContextCtor(appCallbacks[instanceCount]);
        }

        for (instanceCount = 0; instanceCount < MAX_CHANNEL_NUMBER; ++instanceCount) {
            exitConditions[instanceCount] = APP_ExitConditionError;         // Processing loop exit condition
            channelActive[instanceCount] = EB_FALSE;
        }

        // Read all configuration files.
        return_error = ReadCommandLine(argc, argv, configs, numChannels, return_errors);

        {
            EB_U32 totalBuffersSize = 0; // hard coded, to be revisited when changing buffers for multiple instances

            for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                totalBuffersSize += (configs[instanceCount]->inputOutputBufferFifoInitCount << 1);
            }
            parentAppCallBack = (EbParentAppContext_t*)malloc(sizeof(EbParentAppContext_t));
            EbParentAppContextCtor(appCallbacks, parentAppCallBack, numChannels, totalBuffersSize);
        }
        // Process any command line options, including the configuration file

        if (return_error == EB_ErrorNone) {

            // Set Default
            if (configs[0]->targetSocket != 2)
                AssignAppThreadGroup(configs[0]->targetSocket);

            // Init the Encoder
            for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                if (return_errors[instanceCount] == EB_ErrorNone) {

                    configs[instanceCount]->activeChannelCount = numChannels;
                    configs[instanceCount]->channelId = instanceCount;

                    return_errors[instanceCount] = InitEncoder(configs[instanceCount], appCallbacks[instanceCount], instanceCount);
                    return_error = (EB_ERRORTYPE)(return_error | return_errors[instanceCount]);
                }
                else {
                    channelActive[instanceCount] = EB_FALSE;
                }
            }
            // Start the Encoder
            if (return_error == EB_ErrorNone) {

                for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                    if (return_errors[instanceCount] == EB_ErrorNone) {
                        return_errors[instanceCount] = StartEncoder(appCallbacks[instanceCount]);
                        return_error = (EB_ERRORTYPE)(return_error & return_errors[instanceCount]);
                        exitConditions[instanceCount] = APP_ExitConditionNone;
                        channelActive[instanceCount] = EB_TRUE;
                        StartTime(&encodingStartTimesSeconds[instanceCount], &encodingStartTimesuSeconds[instanceCount]);
                    }
                    else {
                        exitConditions[instanceCount] = APP_ExitConditionError;
                    }

#if DISPLAY_MEMORY
                    EB_APP_MEMORY();
#endif
                }
                printf("Encoding          ");
                fflush(stdout);

                // Processing Loop
                exitCondition = APP_ExitConditionNone;
                while (exitCondition == APP_ExitConditionNone) {
                    AppProcessCommands(configs, parentAppCallBack, encodingFinishTimesSeconds, encodingFinishTimesuSeconds, exitConditions);
                    exitCondition = APP_ExitConditionError;
                    for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                        if (exitConditions[instanceCount] != APP_ExitConditionNone && channelActive[instanceCount]) {
                            return_errors[instanceCount] = StopEncoder(appCallbacks[instanceCount]);
                            channelActive[instanceCount] = EB_FALSE;
                            if (exitConditions[instanceCount] == APP_ExitConditionError) {
                                return_error = (EB_ERRORTYPE)(return_error | exitConditions[instanceCount]);
                            }

                        }
                        else if (exitConditions[instanceCount] == APP_ExitConditionNone && channelActive[instanceCount]) {
                            exitCondition = APP_ExitConditionNone;
                        }

                    }
                }
                for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
                    if (exitConditions[instanceCount] != APP_ExitConditionError) {
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
                                (EB_S32)configs[instanceCount]->framesEncoded,
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
                if (return_errors[instanceCount] == EB_ErrorNone && exitConditions[instanceCount] == APP_ExitConditionFinished) {

                    if (configs[instanceCount]->stopEncoder == EB_FALSE) {
                        // Interlaced Video
                        if (configs[instanceCount]->interlacedVideo || configs[instanceCount]->separateFields) {

                            printf("\nChannel %u\nAverage Speed:\t\t%.2f fields per sec\nAverage Latency:\t%.0f ms\nMax Latency:\t\t%u ms\n",
                                (unsigned int)(instanceCount + 1),
                                configs[instanceCount]->performanceContext.averageSpeed,
                                configs[instanceCount]->performanceContext.averageLatency,
                                (unsigned int)(configs[instanceCount]->performanceContext.maxLatency));
                        }
                        else {
                            printf("\nChannel %u\nAverage Speed:\t\t%.2f fps\nAverage Latency:\t%.0f ms\nMax Latency:\t\t%u ms\n",
                                (unsigned int)(instanceCount + 1),
                                configs[instanceCount]->performanceContext.averageSpeed,
                                configs[instanceCount]->performanceContext.averageLatency,
                                (unsigned int)(configs[instanceCount]->performanceContext.maxLatency));

                        }
                    }
                    else {
                        printf("\nChannel %u Encoding Interrupted\n", (unsigned int)(instanceCount + 1));                    
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
                return_errors[instanceCount - 1] = DeInitEncoder(appCallbacks[instanceCount - 1], instanceCount - 1, return_errors[instanceCount - 1]);

            }
        }
        else {
            printf("Error in configuration, could not begin encoding! ... \n");
        }
        // Destruct the App memory variables
        for (instanceCount = 0; instanceCount < numChannels; ++instanceCount) {
            EbConfigDtor(configs[instanceCount]);
            free(configs[instanceCount]);
            free(appCallbacks[instanceCount]);
        }
        EbParentAppContextDtor(parentAppCallBack);
        free(parentAppCallBack);

        printf("Encoder finished\n");

    }

    return (return_error == 0) ? 0 : 1;
}
