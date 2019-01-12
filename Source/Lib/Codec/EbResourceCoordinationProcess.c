/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureBufferDesc.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

#include "EbResourceCoordinationProcess.h"

#include "EbResourceCoordinationResults.h"
#include "EbReferenceObject.h"
#include "EbTime.h"

/************************************************
 * Resource Coordination Context Constructor
 ************************************************/
EB_ERRORTYPE ResourceCoordinationContextCtor(
    ResourceCoordinationContext_t  **contextDblPtr,
    EbFifo_t                        *inputBufferFifoPtr,
    EbFifo_t                        *resourceCoordinationResultsOutputFifoPtr,
    EbFifo_t                       **pictureControlSetFifoPtrArray,
    EbSequenceControlSetInstance_t **sequenceControlSetInstanceArray,
    EbFifo_t                        *sequenceControlSetEmptyFifoPtr,
    EbCallback_t                **appCallbackPtrArray,
    EB_U32                          *computeSegmentsTotalCountArray,
    EB_U32                           encodeInstancesTotalCount)
{
    EB_U32 instanceIndex;

    ResourceCoordinationContext_t *contextPtr;
    EB_MALLOC(ResourceCoordinationContext_t*, contextPtr, sizeof(ResourceCoordinationContext_t), EB_N_PTR);

    *contextDblPtr = contextPtr;

    contextPtr->inputBufferFifoPtr                       = inputBufferFifoPtr;
    contextPtr->resourceCoordinationResultsOutputFifoPtr    = resourceCoordinationResultsOutputFifoPtr;
    contextPtr->pictureControlSetFifoPtrArray               = pictureControlSetFifoPtrArray;
    contextPtr->sequenceControlSetInstanceArray             = sequenceControlSetInstanceArray;
    contextPtr->sequenceControlSetEmptyFifoPtr              = sequenceControlSetEmptyFifoPtr;
    contextPtr->appCallbackPtrArray                         = appCallbackPtrArray;
    contextPtr->computeSegmentsTotalCountArray              = computeSegmentsTotalCountArray;
    contextPtr->encodeInstancesTotalCount                   = encodeInstancesTotalCount;

    // Allocate SequenceControlSetActiveArray
    EB_MALLOC(EbObjectWrapper_t**, contextPtr->sequenceControlSetActiveArray, sizeof(EbObjectWrapper_t*) * contextPtr->encodeInstancesTotalCount, EB_N_PTR);

    for(instanceIndex=0; instanceIndex < contextPtr->encodeInstancesTotalCount; ++instanceIndex) {
        contextPtr->sequenceControlSetActiveArray[instanceIndex] = 0;
    }

    // Picture Stats
    EB_MALLOC(EB_U64*, contextPtr->pictureNumberArray, sizeof(EB_U64) * contextPtr->encodeInstancesTotalCount, EB_N_PTR);

    for(instanceIndex=0; instanceIndex < contextPtr->encodeInstancesTotalCount; ++instanceIndex) {
        contextPtr->pictureNumberArray[instanceIndex] = 0;
    }

	contextPtr->averageEncMod = 0;
	contextPtr->prevEncMod = 0;
	contextPtr->prevEncModeDelta = 0;
	contextPtr->curSpeed = 0; // speed x 1000
	contextPtr->previousModeChangeBuffer = 0;
    contextPtr->firstInPicArrivedTimeSeconds = 0;
    contextPtr->firstInPicArrivedTimeuSeconds = 0;
	contextPtr->previousFrameInCheck1 = 0;
	contextPtr->previousFrameInCheck2 = 0;
	contextPtr->previousFrameInCheck3 = 0;
	contextPtr->previousModeChangeFrameIn = 0;
    contextPtr->prevsTimeSeconds = 0;
    contextPtr->prevsTimeuSeconds = 0;
	contextPtr->prevFrameOut = 0;
	contextPtr->startFlag = EB_FALSE;

	contextPtr->previousBufferCheck1 = 0;
	contextPtr->prevChangeCond = 0;
    return EB_ErrorNone;
}

//******************************************************************************//
// Modify the Enc mode based on the buffer Status
// Inputs: TargetSpeed, Status of the SCbuffer
// Output: EncMod
//******************************************************************************//
void SpeedBufferControl(
	ResourceCoordinationContext_t   *contextPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
	SequenceControlSet_t            *sequenceControlSetPtr)
{

    EB_U64 cursTimeSeconds = 0;
    EB_U64 cursTimeuSeconds = 0;
    double overallDuration = 0.0;
    double instDuration = 0.0;
	EB_S8  encoderModeDelta = 0;
	EB_S64 inputFramesCount = 0;
	EB_S8 changeCond        = 0;
	EB_S64 targetFps        = (sequenceControlSetPtr->staticConfig.injectorFrameRate >> 16);

    
    EB_S64 bufferTrshold1 = SC_FRAMES_INTERVAL_T1;
    EB_S64 bufferTrshold2 = SC_FRAMES_INTERVAL_T2;
    EB_S64 bufferTrshold3 = SC_FRAMES_INTERVAL_T3;
    EB_S64 bufferTrshold4 = MAX(SC_FRAMES_INTERVAL_T1, targetFps);
	EbBlockOnMutex(sequenceControlSetPtr->encodeContextPtr->scBufferMutex);

	if (sequenceControlSetPtr->encodeContextPtr->scFrameIn == 0) {
        EbStartTime((uint64_t*)&contextPtr->firstInPicArrivedTimeSeconds, (uint64_t*)&contextPtr->firstInPicArrivedTimeuSeconds);
	}
	else if (sequenceControlSetPtr->encodeContextPtr->scFrameIn == SC_FRAMES_TO_IGNORE) {
		contextPtr->startFlag = EB_TRUE;
	}

    // Compute duration since the start of the encode and since the previous checkpoint
    EbFinishTime((uint64_t*)&cursTimeSeconds, (uint64_t*)&cursTimeuSeconds);

    EbComputeOverallElapsedTimeMs(
        contextPtr->firstInPicArrivedTimeSeconds,
        contextPtr->firstInPicArrivedTimeuSeconds,
        cursTimeSeconds,
        cursTimeuSeconds,
        &overallDuration);

    EbComputeOverallElapsedTimeMs(
        contextPtr->prevsTimeSeconds,
        contextPtr->prevsTimeuSeconds,
        cursTimeSeconds,
        cursTimeuSeconds,
        &instDuration);

    inputFramesCount = (EB_S64)overallDuration *(sequenceControlSetPtr->staticConfig.injectorFrameRate >> 16) / 1000;
	sequenceControlSetPtr->encodeContextPtr->scBuffer = inputFramesCount - sequenceControlSetPtr->encodeContextPtr->scFrameIn;

	encoderModeDelta = 0;

	// Check every bufferTsshold1 for the changes (previousFrameInCheck1 variable)
	if ((sequenceControlSetPtr->encodeContextPtr->scFrameIn > contextPtr->previousFrameInCheck1 + bufferTrshold1 && sequenceControlSetPtr->encodeContextPtr->scFrameIn >= SC_FRAMES_TO_IGNORE)) {
		// Go to a slower mode based on the fullness and changes of the buffer
        if (sequenceControlSetPtr->encodeContextPtr->scBuffer < bufferTrshold4 && (contextPtr->prevEncModeDelta >-1 || (contextPtr->prevEncModeDelta < 0 && sequenceControlSetPtr->encodeContextPtr->scFrameIn > contextPtr->previousModeChangeFrameIn + bufferTrshold4 * 2))) {
			if (contextPtr->previousBufferCheck1 > sequenceControlSetPtr->encodeContextPtr->scBuffer + bufferTrshold1) {
				encoderModeDelta += -1;
				changeCond = 2;
			}
			else if (contextPtr->previousModeChangeBuffer > bufferTrshold1 + sequenceControlSetPtr->encodeContextPtr->scBuffer && sequenceControlSetPtr->encodeContextPtr->scBuffer < bufferTrshold1) {
				encoderModeDelta += -1;
				changeCond = 4;
			}
		}

		// Go to a faster mode based on the fullness and changes of the buffer
		if (sequenceControlSetPtr->encodeContextPtr->scBuffer >bufferTrshold1 + contextPtr->previousBufferCheck1) {
			encoderModeDelta += +1;
			changeCond = 1;
		}
		else if (sequenceControlSetPtr->encodeContextPtr->scBuffer > bufferTrshold1 + contextPtr->previousModeChangeBuffer) {
			encoderModeDelta += +1;
			changeCond = 3;
		}

		// Update the encode mode based on the fullness of the buffer
        // If previous ChangeCond was the same, double the threshold2
		if (sequenceControlSetPtr->encodeContextPtr->scBuffer > bufferTrshold3 &&
			(contextPtr->prevChangeCond != 7 || sequenceControlSetPtr->encodeContextPtr->scFrameIn > contextPtr->previousModeChangeFrameIn + bufferTrshold2 * 2) &&
			sequenceControlSetPtr->encodeContextPtr->scBuffer > contextPtr->previousModeChangeBuffer) {
			encoderModeDelta += 1;
			changeCond = 7;
		}
		encoderModeDelta = CLIP3(-1, 1, encoderModeDelta);
        sequenceControlSetPtr->encodeContextPtr->encMode = (EB_ENC_MODE)CLIP3(ENC_MODE_0, sequenceControlSetPtr->maxEncMode, (EB_S8)sequenceControlSetPtr->encodeContextPtr->encMode + encoderModeDelta);

		// Update previous stats
		contextPtr->previousFrameInCheck1 = sequenceControlSetPtr->encodeContextPtr->scFrameIn;
		contextPtr->previousBufferCheck1 = sequenceControlSetPtr->encodeContextPtr->scBuffer;

		if (encoderModeDelta) {
			contextPtr->previousModeChangeBuffer = sequenceControlSetPtr->encodeContextPtr->scBuffer;
			contextPtr->previousModeChangeFrameIn = sequenceControlSetPtr->encodeContextPtr->scFrameIn;
			contextPtr->prevEncModeDelta = encoderModeDelta;
		}
	}

	// Check every bufferTrshold2 for the changes (previousFrameInCheck2 variable)
	if ((sequenceControlSetPtr->encodeContextPtr->scFrameIn > contextPtr->previousFrameInCheck2 + bufferTrshold2 && sequenceControlSetPtr->encodeContextPtr->scFrameIn >= SC_FRAMES_TO_IGNORE)) {
		encoderModeDelta = 0;

		// if no change in the encoder mode and buffer is low enough and level is not increasing, switch to a slower encoder mode
        // If previous ChangeCond was the same, double the threshold2
		if (encoderModeDelta == 0 && sequenceControlSetPtr->encodeContextPtr->scFrameIn > contextPtr->previousModeChangeFrameIn + bufferTrshold2 &&
			(contextPtr->prevChangeCond != 8 || sequenceControlSetPtr->encodeContextPtr->scFrameIn > contextPtr->previousModeChangeFrameIn + bufferTrshold2 * 2) &&
            ((sequenceControlSetPtr->encodeContextPtr->scBuffer - contextPtr->previousModeChangeBuffer < (bufferTrshold4 / 3)) || contextPtr->previousModeChangeBuffer == 0) &&
			sequenceControlSetPtr->encodeContextPtr->scBuffer < bufferTrshold3) {
			encoderModeDelta = -1;
			changeCond = 8;
		}

		encoderModeDelta = CLIP3(-1, 1, encoderModeDelta);
        sequenceControlSetPtr->encodeContextPtr->encMode = (EB_ENC_MODE)CLIP3(ENC_MODE_0, sequenceControlSetPtr->maxEncMode, (EB_S8)sequenceControlSetPtr->encodeContextPtr->encMode + encoderModeDelta);
		// Update previous stats
		contextPtr->previousFrameInCheck2 = sequenceControlSetPtr->encodeContextPtr->scFrameIn;

		if (encoderModeDelta) {
			contextPtr->previousModeChangeBuffer = sequenceControlSetPtr->encodeContextPtr->scBuffer;
			contextPtr->previousModeChangeFrameIn = sequenceControlSetPtr->encodeContextPtr->scFrameIn;
			contextPtr->prevEncModeDelta = encoderModeDelta;
		}

	}
	// Check every SC_FRAMES_INTERVAL_SPEED frames for the speed calculation (previousFrameInCheck3 variable)
    if (contextPtr->startFlag || (sequenceControlSetPtr->encodeContextPtr->scFrameIn > contextPtr->previousFrameInCheck3 + SC_FRAMES_INTERVAL_SPEED && sequenceControlSetPtr->encodeContextPtr->scFrameIn >= SC_FRAMES_TO_IGNORE)) {
		if (contextPtr->startFlag) {
			contextPtr->curSpeed = (EB_U64)(sequenceControlSetPtr->encodeContextPtr->scFrameOut - 0) * 1000 / (EB_U64)(overallDuration);
		}
		else {
            if (instDuration != 0)
                contextPtr->curSpeed = (EB_U64)(sequenceControlSetPtr->encodeContextPtr->scFrameOut - contextPtr->prevFrameOut) * 1000 / (EB_U64)(instDuration);
		}
		contextPtr->startFlag = EB_FALSE;

		// Update previous stats
		contextPtr->previousFrameInCheck3 = sequenceControlSetPtr->encodeContextPtr->scFrameIn;
        contextPtr->prevsTimeSeconds = cursTimeSeconds;
        contextPtr->prevsTimeuSeconds = cursTimeuSeconds;
		contextPtr->prevFrameOut = sequenceControlSetPtr->encodeContextPtr->scFrameOut;

	}
    else if (sequenceControlSetPtr->encodeContextPtr->scFrameIn < SC_FRAMES_TO_IGNORE && (overallDuration != 0)) {
        contextPtr->curSpeed = (EB_U64)(sequenceControlSetPtr->encodeContextPtr->scFrameOut - 0) * 1000 / (EB_U64)(overallDuration);
	}

	if (changeCond) {
		contextPtr->prevChangeCond = changeCond;
	}
	sequenceControlSetPtr->encodeContextPtr->scFrameIn++;
	if (sequenceControlSetPtr->encodeContextPtr->scFrameIn >= SC_FRAMES_TO_IGNORE) {
		contextPtr->averageEncMod += sequenceControlSetPtr->encodeContextPtr->encMode;
	}
	else {
		contextPtr->averageEncMod = 0;
	}

	// Set the encoder level
	pictureControlSetPtr->encMode = sequenceControlSetPtr->encodeContextPtr->encMode;

	EbReleaseMutex(sequenceControlSetPtr->encodeContextPtr->scBufferMutex);

	contextPtr->prevEncMod = sequenceControlSetPtr->encodeContextPtr->encMode;
}


/******************************************************
* Derive Pre-Analysis settings for SQ
Input   : encoder mode and tune
Output  : Pre-Analysis signal(s)
******************************************************/
EB_ERRORTYPE SignalDerivationPreAnalysisSq(
    SequenceControlSet_t      *sequenceControlSetPtr,
    PictureParentControlSet_t *pictureControlSetPtr) {

    EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_U8 inputResolution = sequenceControlSetPtr->inputResolution;

    // Derive Noise Detection Method
    if (inputResolution == INPUT_SIZE_4K_RANGE) {
        if (pictureControlSetPtr->encMode <= ENC_MODE_8) {
            pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_FULL_PRECISION;
        }
        else {
            pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_HALF_PRECISION;
        }

    }
    else if (inputResolution == INPUT_SIZE_1080p_RANGE) {
        if (pictureControlSetPtr->encMode <= ENC_MODE_8) {
            pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_FULL_PRECISION;
        }
        else {
            pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_QUARTER_PRECISION;
        }

    }
    else {
        pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_FULL_PRECISION;
    }

	pictureControlSetPtr->enableDenoiseSrcFlag = pictureControlSetPtr->encMode <= ENC_MODE_11 ? sequenceControlSetPtr->enableDenoiseFlag :	EB_FALSE;
	pictureControlSetPtr->disableVarianceFlag =  (pictureControlSetPtr->encMode <= ENC_MODE_11) ? EB_FALSE : EB_TRUE;
    // Derive Noise Detection Threshold
    if (pictureControlSetPtr->encMode <= ENC_MODE_8) {
        pictureControlSetPtr->noiseDetectionTh = 0;
    }
    else {
        pictureControlSetPtr->noiseDetectionTh = 1;
    }

    EB_U8  hmeMeLevel       = pictureControlSetPtr->encMode;

    EB_U32 inputRatio       = sequenceControlSetPtr->lumaWidth / sequenceControlSetPtr->lumaHeight;
    EB_U8 resolutionIndex   = inputResolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ?               0   : // 480P
                                (inputResolution <= INPUT_SIZE_1080i_RANGE && inputRatio < 2) ?    1   : // 720P
                                (inputResolution <= INPUT_SIZE_1080i_RANGE && inputRatio > 3) ?    2   : // 1080I
                                (inputResolution <= INPUT_SIZE_1080p_RANGE) ?                      3   : // 1080I
                                                                                                4;    // 4K
    
    // Derive HME Flag
    if (sequenceControlSetPtr->staticConfig.useDefaultMeHme)
        pictureControlSetPtr->enableHmeFlag = EB_TRUE;
    else
        pictureControlSetPtr->enableHmeFlag = sequenceControlSetPtr->staticConfig.enableHmeFlag;
    pictureControlSetPtr->enableHmeLevel0Flag   = EnableHmeLevel0FlagSq[resolutionIndex][hmeMeLevel];
    pictureControlSetPtr->enableHmeLevel1Flag   = EnableHmeLevel1FlagSq[resolutionIndex][hmeMeLevel];
    pictureControlSetPtr->enableHmeLevel2Flag   = EnableHmeLevel2FlagSq[resolutionIndex][hmeMeLevel];

    return return_error;
}

/******************************************************
* Derive Pre-Analysis settings for OQ
Input   : encoder mode and tune
Output  : Pre-Analysis signal(s)
******************************************************/
EB_ERRORTYPE SignalDerivationPreAnalysisOq(
    SequenceControlSet_t       *sequenceControlSetPtr,
    PictureParentControlSet_t  *pictureControlSetPtr) {

    EB_ERRORTYPE return_error = EB_ErrorNone;
    
    EB_U8 inputResolution = sequenceControlSetPtr->inputResolution;


    // Derive Noise Detection Method
    if (inputResolution == INPUT_SIZE_4K_RANGE) {
        if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
            pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_FULL_PRECISION;
        }
        else {
            pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_HALF_PRECISION;
        }
    }
    else if (inputResolution == INPUT_SIZE_1080p_RANGE) {
        if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
            pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_FULL_PRECISION;
        }
        else {
            pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_QUARTER_PRECISION;
        }
    }
    else {
        pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_FULL_PRECISION;
    }
    

    // Derive Noise Detection Threshold
    if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
        pictureControlSetPtr->noiseDetectionTh = 0;
    }
	else if (pictureControlSetPtr->encMode <= ENC_MODE_8) {
		if (inputResolution <= INPUT_SIZE_1080p_RANGE) {
			pictureControlSetPtr->noiseDetectionTh = 1;
		}
		else {
			pictureControlSetPtr->noiseDetectionTh = 0;
		}
	}
    else{
        pictureControlSetPtr->noiseDetectionTh = 1;
    }

    EB_U8  hmeMeLevel       = pictureControlSetPtr->encMode;

    EB_U32 inputRatio       = sequenceControlSetPtr->lumaWidth / sequenceControlSetPtr->lumaHeight;
    EB_U8 resolutionIndex   = inputResolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ?               0   : // 480P
                                (inputResolution <= INPUT_SIZE_1080i_RANGE && inputRatio < 2) ?    1   : // 720P
                                (inputResolution <= INPUT_SIZE_1080i_RANGE && inputRatio > 3) ?    2   : // 1080I
                                (inputResolution <= INPUT_SIZE_1080p_RANGE) ?                      3   : // 1080I
                                                                                                4;    // 4K

    // Derive HME Flag
    if (sequenceControlSetPtr->staticConfig.useDefaultMeHme)
        pictureControlSetPtr->enableHmeFlag = EB_TRUE;
    else
        pictureControlSetPtr->enableHmeFlag = sequenceControlSetPtr->staticConfig.enableHmeFlag;
    pictureControlSetPtr->enableHmeLevel0Flag   = EnableHmeLevel0FlagOq[resolutionIndex][hmeMeLevel];
    pictureControlSetPtr->enableHmeLevel1Flag   = EnableHmeLevel1FlagOq[resolutionIndex][hmeMeLevel];
    pictureControlSetPtr->enableHmeLevel2Flag   = EnableHmeLevel2FlagOq[resolutionIndex][hmeMeLevel];

	pictureControlSetPtr->enableDenoiseSrcFlag = EB_FALSE;

	if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		pictureControlSetPtr->disableVarianceFlag = EB_FALSE;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_8) {
		if (inputResolution == INPUT_SIZE_4K_RANGE) {
			pictureControlSetPtr->disableVarianceFlag = EB_TRUE;
		}
		else {
			pictureControlSetPtr->disableVarianceFlag = EB_FALSE;
		}
	}
	else {
		pictureControlSetPtr->disableVarianceFlag = EB_TRUE;
	}

    return return_error;
}

/******************************************************
* Derive Pre-Analysis settings for VMAF
Input   : encoder mode and tune
Output  : Pre-Analysis signal(s)
******************************************************/
EB_ERRORTYPE SignalDerivationPreAnalysisVmaf(
	SequenceControlSet_t       *sequenceControlSetPtr,
	PictureParentControlSet_t  *pictureControlSetPtr) {

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U8 inputResolution = sequenceControlSetPtr->inputResolution;

	// Derive Noise Detection Method
	pictureControlSetPtr->noiseDetectionMethod = NOISE_DETECT_QUARTER_PRECISION;

	// Derive Noise Detection Threshold
	pictureControlSetPtr->noiseDetectionTh = 1;

	EB_U8  hmeMeLevel = pictureControlSetPtr->encMode;
	EB_U32 inputRatio = sequenceControlSetPtr->lumaWidth / sequenceControlSetPtr->lumaHeight;
	EB_U8 resolutionIndex = inputResolution <= INPUT_SIZE_576p_RANGE_OR_LOWER ? 0 : // 480P
		(inputResolution <= INPUT_SIZE_1080i_RANGE && inputRatio < 2) ? 1 : // 720P
		(inputResolution <= INPUT_SIZE_1080i_RANGE && inputRatio > 3) ? 2 : // 1080I
		(inputResolution <= INPUT_SIZE_1080p_RANGE) ? 3 : // 1080I
		4;    // 4K
	resolutionIndex = 3;
    // Derive HME Flag
    if (sequenceControlSetPtr->staticConfig.useDefaultMeHme) {
        pictureControlSetPtr->enableHmeFlag = EB_TRUE;
    }
    else {
        pictureControlSetPtr->enableHmeFlag = sequenceControlSetPtr->staticConfig.enableHmeFlag;
    }
	pictureControlSetPtr->enableHmeLevel0Flag = EnableHmeLevel0FlagVmaf[resolutionIndex][hmeMeLevel];
	pictureControlSetPtr->enableHmeLevel1Flag = EnableHmeLevel1FlagVmaf[resolutionIndex][hmeMeLevel];
    pictureControlSetPtr->enableHmeLevel2Flag = EnableHmeLevel2FlagVmaf[resolutionIndex][hmeMeLevel];

	pictureControlSetPtr->enableDenoiseSrcFlag = EB_FALSE;
	pictureControlSetPtr->disableVarianceFlag = EB_TRUE;

	return return_error;
}


/***************************************
 * ResourceCoordination Kernel
 ***************************************/
void* ResourceCoordinationKernel(void *inputPtr)
{
    ResourceCoordinationContext_t *contextPtr = (ResourceCoordinationContext_t*) inputPtr;

    EbObjectWrapper_t               *pictureControlSetWrapperPtr;

    PictureParentControlSet_t       *pictureControlSetPtr;

    SequenceControlSet_t            *sequenceControlSetPtr;

    EbObjectWrapper_t               *ebInputWrapperPtr;
    EB_BUFFERHEADERTYPE             *ebInputPtr;
    EbObjectWrapper_t               *outputWrapperPtr;
    ResourceCoordinationResults_t   *outputResultsPtr;

    EbObjectWrapper_t               *inputPictureWrapperPtr;
    EbPictureBufferDesc_t           *inputPicturePtr;
    EbObjectWrapper_t               *referencePictureWrapperPtr;

    EB_U32                           instanceIndex;

    EB_BOOL                          endOfSequenceFlag = EB_FALSE;

    EB_BOOL                          is16BitInput;

	EB_U32							inputSize = 0;
	EbObjectWrapper_t              *prevPictureControlSetWrapperPtr = 0;
    EB_U32                          chromaFormat = EB_YUV420;
    EB_U32                          subWidthCMinus1 = 1;
    EB_U32                          subHeightCMinus1 = 1;
    
    for(;;) {

        // Tie instanceIndex to zero for now...
        instanceIndex = 0;

        // Get the Next Input Buffer [BLOCKING]
        EbGetFullObject(
            contextPtr->inputBufferFifoPtr,
            &ebInputWrapperPtr);
        ebInputPtr = (EB_BUFFERHEADERTYPE*) ebInputWrapperPtr->objectPtr;
     
        sequenceControlSetPtr       = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr;

        // Get source video bit depth
        is16BitInput =  (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);

        chromaFormat = sequenceControlSetPtr->chromaFormatIdc;
        subWidthCMinus1 = (chromaFormat == EB_YUV444 ? 1 : 2) - 1;
        subHeightCMinus1 = (chromaFormat >= EB_YUV422 ? 1 : 2) - 1;
        // If config changes occured since the last picture began encoding, then
        //   prepare a new sequenceControlSetPtr containing the new changes and update the state
        //   of the previous Active SequenceControlSet
        EbBlockOnMutex(contextPtr->sequenceControlSetInstanceArray[instanceIndex]->configMutex);
        if(contextPtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->initialPicture) {
            
            // Update picture width, picture height, cropping right offset, cropping bottom offset, and conformance windows
            if(contextPtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->initialPicture) 
            
            {
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lumaWidth = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lumaHeight = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight;
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->chromaWidth = (contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth >> subWidthCMinus1);
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->chromaHeight = (contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight >> subHeightCMinus1);

                
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->padRight = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputPadRight;
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->croppingRightOffset = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->padRight;
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->padBottom = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputPadBottom;
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->croppingBottomOffset = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->padBottom;

                if(contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->padRight != 0 || contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->padBottom != 0) {
                    contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->conformanceWindowFlag = 1;
                }
                else {
                    contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->conformanceWindowFlag = 0;
                }
                
				inputSize = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lumaWidth * contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lumaHeight;
            }

            // HDR BT2020
            if (sequenceControlSetPtr->staticConfig.videoUsabilityInfo)
            {
                
                AppVideoUsabilityInfo_t    *vuiPtr = contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->videoUsabilityInfoPtr;

                if (sequenceControlSetPtr->staticConfig.highDynamicRangeInput && is16BitInput){
                    vuiPtr->aspectRatioInfoPresentFlag = EB_TRUE;
                    vuiPtr->overscanInfoPresentFlag = EB_FALSE;

                    vuiPtr->videoSignalTypePresentFlag = EB_TRUE;
                    vuiPtr->videoFormat = 5;
                    vuiPtr->videoFullRangeFlag = EB_FALSE;

                    vuiPtr->colorDescriptionPresentFlag = EB_TRUE;
                    vuiPtr->colorPrimaries = 9;   // BT2020
                    vuiPtr->transferCharacteristics = 16;  // SMPTE ST2048
                    vuiPtr->matrixCoeffs = 9;

                    vuiPtr->chromaLocInfoPresentFlag = EB_TRUE;
                    vuiPtr->chromaSampleLocTypeTopField = 2;
                    vuiPtr->chromaSampleLocTypeBottomField = 2;
                }

                vuiPtr->vuiTimingInfoPresentFlag        = EB_TRUE;
                if (sequenceControlSetPtr->staticConfig.frameRateDenominator != 0 && sequenceControlSetPtr->staticConfig.frameRateNumerator != 0) {
                    vuiPtr->vuiTimeScale = (sequenceControlSetPtr->staticConfig.frameRateNumerator);
                    vuiPtr->vuiNumUnitsInTick = sequenceControlSetPtr->staticConfig.frameRateDenominator;
                }
                else {
                    vuiPtr->vuiTimeScale = (sequenceControlSetPtr->staticConfig.frameRate) > 1000 ? (sequenceControlSetPtr->staticConfig.frameRate) : (sequenceControlSetPtr->staticConfig.frameRate)<<16;
                    vuiPtr->vuiNumUnitsInTick = 1 << 16;                
                }
                    
            }
            // Get empty SequenceControlSet [BLOCKING]
            EbGetEmptyObject(
                contextPtr->sequenceControlSetEmptyFifoPtr,
                &contextPtr->sequenceControlSetActiveArray[instanceIndex]);

            // Copy the contents of the active SequenceControlSet into the new empty SequenceControlSet
            CopySequenceControlSet(
                (SequenceControlSet_t*) contextPtr->sequenceControlSetActiveArray[instanceIndex]->objectPtr,
                contextPtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr);

        }
        EbReleaseMutex(contextPtr->sequenceControlSetInstanceArray[instanceIndex]->configMutex);

        // Sequence Control Set is released by Rate Control after passing through MDC->MD->ENCDEC->Packetization->RateControl
        //   and in the PictureManager
        EbObjectIncLiveCount(
            contextPtr->sequenceControlSetActiveArray[instanceIndex],
            2);

        // Set the current SequenceControlSet
        sequenceControlSetPtr   = (SequenceControlSet_t*) contextPtr->sequenceControlSetActiveArray[instanceIndex]->objectPtr;
        
		// Init LCU Params
        if (contextPtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->initialPicture) {
            DeriveInputResolution(
                sequenceControlSetPtr,
                inputSize);

            LcuParamsInit(sequenceControlSetPtr);
        }

        //Get a New ParentPCS where we will hold the new inputPicture
        EbGetEmptyObject(
            contextPtr->pictureControlSetFifoPtrArray[instanceIndex],
            &pictureControlSetWrapperPtr);

        // Parent PCS is released by the Rate Control after passing through MDC->MD->ENCDEC->Packetization
        EbObjectIncLiveCount(
            pictureControlSetWrapperPtr,
            1);

        pictureControlSetPtr        = (PictureParentControlSet_t*) pictureControlSetWrapperPtr->objectPtr;

        pictureControlSetPtr->pPcsWrapperPtr = pictureControlSetWrapperPtr;

        // Set the Encoder mode
        pictureControlSetPtr->encMode = sequenceControlSetPtr->staticConfig.encMode; 

		// Keep track of the previous input for the ZZ SADs computation
		pictureControlSetPtr->previousPictureControlSetWrapperPtr = (contextPtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->initialPicture) ?
			pictureControlSetWrapperPtr :
			sequenceControlSetPtr->encodeContextPtr->previousPictureControlSetWrapperPtr;

		sequenceControlSetPtr->encodeContextPtr->previousPictureControlSetWrapperPtr = pictureControlSetWrapperPtr;

        // Copy data from the buffer to the input frame
        // *Note - Assumes 4:2:0 planar
        inputPictureWrapperPtr  = NULL;
        inputPicturePtr         = NULL;

        // assign the input picture
        pictureControlSetPtr->enhancedPicturePtr = (EbPictureBufferDesc_t*)ebInputPtr->pBuffer;
        // start latency measure as soon as we copy input picture from buffer
        pictureControlSetPtr->startTimeSeconds = 0;
        pictureControlSetPtr->startTimeuSeconds = 0;

        EbStartTime((uint64_t*)&pictureControlSetPtr->startTimeSeconds, (uint64_t*)&pictureControlSetPtr->startTimeuSeconds);

        inputPicturePtr = pictureControlSetPtr->enhancedPicturePtr;

        // Setup new input picture buffer
        inputPicturePtr->bitDepth           = sequenceControlSetPtr->inputBitdepth;
		inputPicturePtr->originX			= sequenceControlSetPtr->leftPadding;

        inputPicturePtr->originY            = sequenceControlSetPtr->topPadding;

        inputPicturePtr->maxWidth           = sequenceControlSetPtr->maxInputLumaWidth;
        inputPicturePtr->maxHeight          = sequenceControlSetPtr->maxInputLumaHeight;
        inputPicturePtr->lumaSize           =
			((sequenceControlSetPtr->maxInputLumaWidth - sequenceControlSetPtr->maxInputPadRight) + sequenceControlSetPtr->leftPadding + sequenceControlSetPtr->rightPadding) *
			((sequenceControlSetPtr->maxInputLumaHeight - sequenceControlSetPtr->maxInputPadBottom) + sequenceControlSetPtr->topPadding + sequenceControlSetPtr->botPadding);
        inputPicturePtr->chromaSize         = inputPicturePtr->lumaSize >> (3 - chromaFormat);

        inputPicturePtr->width              = sequenceControlSetPtr->lumaWidth;
        inputPicturePtr->height             = sequenceControlSetPtr->lumaHeight;
        inputPicturePtr->strideY            = sequenceControlSetPtr->lumaWidth + sequenceControlSetPtr->leftPadding + sequenceControlSetPtr->rightPadding;
        inputPicturePtr->strideCb           = inputPicturePtr->strideCr = inputPicturePtr->strideY >> subWidthCMinus1;

		inputPicturePtr->strideBitIncY      = inputPicturePtr->strideY;
		inputPicturePtr->strideBitIncCb     = inputPicturePtr->strideCb;
		inputPicturePtr->strideBitIncCr     = inputPicturePtr->strideCr;
        
        pictureControlSetPtr->ebInputPtr    = ebInputPtr;
        pictureControlSetPtr->ebInputWrapperPtr = ebInputWrapperPtr;

        endOfSequenceFlag = (ebInputPtr->nFlags & EB_BUFFERFLAG_EOS) ? EB_TRUE : EB_FALSE;

        pictureControlSetPtr->sequenceControlSetWrapperPtr    = contextPtr->sequenceControlSetActiveArray[instanceIndex];
        pictureControlSetPtr->inputPictureWrapperPtr          = inputPictureWrapperPtr;

        // Set Picture Control Flags
        pictureControlSetPtr->idrFlag                         = sequenceControlSetPtr->encodeContextPtr->initialPicture || (ebInputPtr->sliceType == EB_IDR_PICTURE);
        pictureControlSetPtr->craFlag                         = (ebInputPtr->sliceType == EB_I_PICTURE) ? EB_TRUE : EB_FALSE;
        pictureControlSetPtr->sceneChangeFlag                 = EB_FALSE;

        pictureControlSetPtr->qpOnTheFly                      = EB_FALSE;
           
		pictureControlSetPtr->lcuTotalCount					  = sequenceControlSetPtr->lcuTotalCount;

		if (sequenceControlSetPtr->staticConfig.speedControlFlag) {
			SpeedBufferControl(
				contextPtr,
				pictureControlSetPtr,
				sequenceControlSetPtr);
		}
		else {
			pictureControlSetPtr->encMode = (EB_ENC_MODE)sequenceControlSetPtr->staticConfig.encMode;
		}

		// Set the SCD Mode
		sequenceControlSetPtr->scdMode = sequenceControlSetPtr->staticConfig.sceneChangeDetection == 0 ?
			SCD_MODE_0 :
			SCD_MODE_1 ;
     

        // Pre-Analysis Signal(s) derivation
        if (sequenceControlSetPtr->staticConfig.tune == TUNE_SQ) {
            SignalDerivationPreAnalysisSq(
                sequenceControlSetPtr,
                pictureControlSetPtr);
        }
        else if (sequenceControlSetPtr->staticConfig.tune == TUNE_VMAF) {
            SignalDerivationPreAnalysisVmaf(
                sequenceControlSetPtr,
                pictureControlSetPtr);
		}
        else {
            SignalDerivationPreAnalysisOq(
                sequenceControlSetPtr,
                pictureControlSetPtr);
        }

	    // Rate Control                                            
		// Set the ME Distortion and OIS Historgrams to zero
        if (sequenceControlSetPtr->staticConfig.rateControlMode){
	            EB_MEMSET(pictureControlSetPtr->meDistortionHistogram, 0, NUMBER_OF_SAD_INTERVALS*sizeof(EB_U16));
	            EB_MEMSET(pictureControlSetPtr->oisDistortionHistogram, 0, NUMBER_OF_INTRA_SAD_INTERVALS*sizeof(EB_U16));
        }
	    pictureControlSetPtr->fullLcuCount                    = 0;

        if (sequenceControlSetPtr->staticConfig.useQpFile == 1){
            pictureControlSetPtr->qpOnTheFly = EB_TRUE;
            if (ebInputPtr->qpValue > 51)
                CHECK_REPORT_ERROR_NC(contextPtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->appCallbackPtr, EB_ENC_RES_COORD_InvalidQP);
            pictureControlSetPtr->pictureQp = (EB_U8)ebInputPtr->qpValue;
        }
        else {
            pictureControlSetPtr->qpOnTheFly = EB_FALSE;
            pictureControlSetPtr->pictureQp = (EB_U8)sequenceControlSetPtr->qp;
        }

        // Picture Stats
        pictureControlSetPtr->pictureNumber                   = contextPtr->pictureNumberArray[instanceIndex]++;

#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld RESCOOR IN \n", pictureControlSetPtr->pictureNumber);
#endif    
        // Set the picture structure: 0: progressive, 1: top, 2: bottom
        pictureControlSetPtr->pictStruct = sequenceControlSetPtr->interlacedVideo == EB_FALSE ? 
            PROGRESSIVE_PICT_STRUCT : 
            pictureControlSetPtr->pictureNumber % 2 == 0 ?
                TOP_FIELD_PICT_STRUCT :
                BOTTOM_FIELD_PICT_STRUCT ;

#if TILES        
        sequenceControlSetPtr->tileUniformSpacing = 1;
        sequenceControlSetPtr->tileColumnCount = sequenceControlSetPtr->staticConfig.tileColumnCount;
        sequenceControlSetPtr->tileRowCount    = sequenceControlSetPtr->staticConfig.tileRowCount;
#endif
        sequenceControlSetPtr->encodeContextPtr->initialPicture = EB_FALSE;

        // Get Empty Reference Picture Object
        EbGetEmptyObject(
            sequenceControlSetPtr->encodeContextPtr->paReferencePicturePoolFifoPtr,
            &referencePictureWrapperPtr);

        pictureControlSetPtr->paReferencePictureWrapperPtr = referencePictureWrapperPtr;

        // Give the new Reference a nominal liveCount of 1
        EbObjectIncLiveCount(
        	pictureControlSetPtr->paReferencePictureWrapperPtr,
            2);
   
        EbObjectIncLiveCount(
            pictureControlSetWrapperPtr,
            2);

        ((EbPaReferenceObject_t*)pictureControlSetPtr->paReferencePictureWrapperPtr->objectPtr)->inputPaddedPicturePtr->bufferY = inputPicturePtr->bufferY;

        // Get Empty Output Results Object
		if (pictureControlSetPtr->pictureNumber > 0 && prevPictureControlSetWrapperPtr != (EbObjectWrapper_t*)EB_NULL)
		{
			((PictureParentControlSet_t       *)prevPictureControlSetWrapperPtr->objectPtr)->endOfSequenceFlag = endOfSequenceFlag;

			EbGetEmptyObject(
				contextPtr->resourceCoordinationResultsOutputFifoPtr,
				&outputWrapperPtr);
			outputResultsPtr = (ResourceCoordinationResults_t*)outputWrapperPtr->objectPtr;
			outputResultsPtr->pictureControlSetWrapperPtr = prevPictureControlSetWrapperPtr;

			// Post the finished Results Object
			EbPostFullObject(outputWrapperPtr);

		}

		prevPictureControlSetWrapperPtr = pictureControlSetWrapperPtr;



#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld RESCOOR OUT \n", pictureControlSetPtr->pictureNumber);
#endif

    }

    return EB_NULL;
}
