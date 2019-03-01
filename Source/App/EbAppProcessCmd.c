/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
 * Includes
 ***************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "EbAppContext.h"
#include "EbAppConfig.h"
#include "EbErrorCodes.h"

#include "EbTime.h"
/***************************************
 * Macros
 ***************************************/

#define CLIP3(MinVal, MaxVal, a)        (((a)<(MinVal)) ? (MinVal) : (((a)>(MaxVal)) ? (MaxVal) :(a)))
#define FUTURE_WINDOW_WIDTH                 4
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height, csp, is16bit) \
    ( (((width)*(height)) + 2*(((width)*(height))>>(3-csp)) )<<is16bit)
extern volatile int32_t keepRunning;

/***************************************
* Process Error Log
***************************************/
void LogErrorOutput(
    FILE                     *errorLogFile,
    uint32_t                  errorCode)
{

    switch (errorCode) {

        // EB_ENC_AMVP_ERRORS:
    case EB_ENC_AMVP_ERROR1:
        fprintf(errorLogFile, "Error: The input PU to GetNonScalingSpatialAMVP() can not be I_MODE!\n");
        break;

    case EB_ENC_AMVP_ERROR2:
        fprintf(errorLogFile, "Error: The input PU to GetNonScalingSpatialAMVP() must be available!\n");
        break;

    case EB_ENC_AMVP_ERROR3:
        fprintf(errorLogFile, "Error: The input PU to GetNonScalingSpatialAMVP() can not be I_MODE!\n");
        break;

    case EB_ENC_AMVP_ERROR4:
        fprintf(errorLogFile, "Error: The availability parameter in GetSpatialMVPPosAx() function can not be > 3 !\n");
        break;

    case EB_ENC_AMVP_ERROR5:
        fprintf(errorLogFile, "Error: The availability parameter in GetSpatialMVPPosBx() function can not be > 7 !\n");
        break;

    case EB_ENC_AMVP_ERROR6:
        fprintf(errorLogFile, "Error: GetTemporalMVP: tmvpMapLcuIndex must be either 0 or 1!\n");
        break;

    case EB_ENC_AMVP_ERROR7:
        fprintf(errorLogFile, "Error: the input PU to GetNonScalingSpatialAMVP() must be available!");
        break;

    case EB_ENC_AMVP_ERROR8:
        fprintf(errorLogFile, "Error: GetTemporalMVP: tmvpMapLcuIndex must be either 0 or 1");
        break;

    case EB_ENC_AMVP_NULL_REF_ERROR:
        fprintf(errorLogFile, "Error: The referenceObject can not be NULL!\n");
        break;

    case EB_ENC_AMVP_SPATIAL_NA_ERROR:
        fprintf(errorLogFile, "Error: The input PU to GetNonScalingSpatialAMVP() must be available!\n");
        break;

        // EB_ENC_CL_ERRORS:
    case EB_ENC_CL_ERROR1:
        fprintf(errorLogFile, "Error: Unknown Inter Prediction Direction!\n");
        break;

    case EB_ENC_CL_ERROR2:
        fprintf(errorLogFile, "Error: Unknown coding mode!\n");
        break;

    case EB_ENC_CL_ERROR3:
        fprintf(errorLogFile, "Error: Mode Decision Candidate Buffer Overflow!\n");
        break;

    case EB_ENC_CL_ERROR4:
        fprintf(errorLogFile, "Error: Too many Mode Decision Fast Candidates!\n");
        break;

    case EB_ENC_CL_ERROR5:
        fprintf(errorLogFile, "Error: Too many buffers chosen for this level by PreModeDecision!\n");
        break;

    case EB_ENC_CL_ERROR6:
        fprintf(errorLogFile, "Error: Ping-Pong structure needs at least two buffers to work properly!\n");
        break;

    case EB_ENC_CL_ERROR7:
        fprintf(errorLogFile, "Error: Invalid Intra Partition\n");
        break;

    case EB_ENC_CL_ERROR8:
        fprintf(errorLogFile, "Error: Invalid TU Configuration\n");
        break;
    case EB_ENC_CL_ERROR9:
        fprintf(errorLogFile, "Error: Invalid Prediction Mode\n");
        break;


        // EB_ENC_DLF_ERRORS:
    case EB_ENC_DLF_ERROR1:
        fprintf(errorLogFile, "Error: While calculating bS for DLF!\n");
        break;

    case EB_ENC_DLF_ERROR2:
        fprintf(errorLogFile, "Error: Unknown Inter Prediction Direction Combination!\n");
        break;

    case EB_ENC_DLF_ERROR3:
        fprintf(errorLogFile, "Error: If any PU is in I_MODE, the bS will be 2!\n");
        break;

    case EB_ENC_DLF_ERROR4:
        fprintf(errorLogFile, "Error: The x/y location of the CU must be the multiple of minmum CU size!");
        break;

    case EB_ENC_DLF_ERROR5:
        fprintf(errorLogFile, "Error: Unknown Slice Type!");
        break;

    case EB_ENC_DLF_ERROR6:
        fprintf(errorLogFile, "Error: While calculating the bS for the PU bounday, the 4x4 block must be guaranteed at the PU boundary!");
        break;

    case EB_ENC_DLF_ERROR7:
        fprintf(errorLogFile, "Error: LCU size must be power of 2!");
        break;

    case EB_ENC_DLF_ERROR8:
        fprintf(errorLogFile, "Error: Deblocking filter can not support the picture whose width or height is not the multiple of 8!");
        break;

    case EB_ENC_DLF_ERROR9:
        fprintf(errorLogFile, "Error: Neighbor PU must be available!");
        break;

    case EB_ENC_DLF_ERROR10:
        fprintf(errorLogFile, "Error: Deblocking filter can not support the picture whose width or height is not the multiple of 8!");
        break;



        // EB_ENC_EC_ERRORS:
    case EB_ENC_EC_ERROR1:
        fprintf(errorLogFile, "Error: EncodeCodedBlockFlags: context value too large!\n");
        break;

    case EB_ENC_EC_ERROR10:
        fprintf(errorLogFile, "Error: EncodeTuSplitCoeff: context value too large!\n");
        break;

    case EB_ENC_EC_ERROR11:
        fprintf(errorLogFile, "Error: CodeSPS: Long term reference pictures are not currently handled!\n");
        break;

    case EB_ENC_EC_ERROR12:
        fprintf(errorLogFile, "Error: CodeProfileTierLevel: The maximum sublayers must be equal to 1!\n");
        break;

    case EB_ENC_EC_ERROR13:
        fprintf(errorLogFile, "Error: EncodeRootCodedBlockFlag: rootCbf too large!\n");
        break;

    case EB_ENC_EC_ERROR14:
        fprintf(errorLogFile, "Error: cpbCountMinus1 in HRD parameter exceeds the upper limit 4!\n");
        break;

    case EB_ENC_EC_ERROR15:
        fprintf(errorLogFile, "Error: numDecodingUnitsMinus1 in picture timeing SEI exceeds the upper limit 64!\n");
        break;

    case EB_ENC_EC_ERROR16:
        fprintf(errorLogFile, "Error: The size of the unregistered user data SEI payload is not allowed!\n");
        break;

    case EB_ENC_EC_ERROR2:
        fprintf(errorLogFile, "Error: CopyRbspBitstreamToPayload: output buffer too small!\n");
        break;

    case EB_ENC_EC_ERROR3:
        fprintf(errorLogFile, "Error: EncodeLcu: Unknown mode type!\n");
        break;

    case EB_ENC_EC_ERROR4:
        fprintf(errorLogFile, "Error: 8x4 & 4x8 PU should not have Bi-pred mode!\n");
        break;

    case EB_ENC_EC_ERROR5:
        fprintf(errorLogFile, "Error: EncodeMergeIndex: value too large!\n");
        break;

    case EB_ENC_EC_ERROR6:
        fprintf(errorLogFile, "Error: EncodeSkipFlag: context too large!\n");
        break;

    case EB_ENC_EC_ERROR7:
        fprintf(errorLogFile, "Error: EncodeBypassBins: binsLength must be less than 32!\n");
        break;

    case EB_ENC_EC_ERROR8:
        fprintf(errorLogFile, "Error: EncodeQuantizedCoefficients: Invalid block size!\n");
        break;

    case EB_ENC_EC_ERROR9:
        fprintf(errorLogFile, "Error: EncodeSplitFlag: context too large!\n");
        break;

    case EB_ENC_EC_ERROR26:
        fprintf(errorLogFile, "Error: Level not recognized!\n");
        break;

    case EB_ENC_EC_ERROR27:
        fprintf(errorLogFile, "Error: EncodeOneBin:  BinaryValue must be less than 2\n");
        break;

    case EB_ENC_EC_ERROR28:
        fprintf(errorLogFile, "Error: No more than 6 SAO types\n");
        break;

    case EB_ENC_EC_ERROR29:
        fprintf(errorLogFile, "Error: No more than 6 SAO types\n");
        break;

        // EB_ENC_FL_ERRORS:
    case EB_ENC_FL_ERROR1:
        fprintf(errorLogFile, "Error: Uncovered area inside Cu!\n");
        break;

    case EB_ENC_FL_ERROR2:
        fprintf(errorLogFile, "Error: Depth 2 is not allowed for 8x8 CU!\n");
        break;

    case EB_ENC_FL_ERROR3:
        fprintf(errorLogFile, "Error: Depth 0 is not allowed for 64x64 CU!\n");
        break;

    case EB_ENC_FL_ERROR4:
        fprintf(errorLogFile, "Error: Max CU Depth Exceeded!\n");
        break;

        // EB_ENC_HANDLE_ERRORS:
    case EB_ENC_HANDLE_ERROR1:
        fprintf(errorLogFile, "Error: Only one Resource Coordination Process allowed!\n");
        break;

    case EB_ENC_HANDLE_ERROR10:
        fprintf(errorLogFile, "Error: Need at least one Entropy Coding Process!\n");
        break;

    case EB_ENC_HANDLE_ERROR11:
        fprintf(errorLogFile, "Error: Only one Packetization Process allowed!\n");
        break;

    case EB_ENC_HANDLE_ERROR12:
        fprintf(errorLogFile, "Error: RC Results Fifo Size should be greater than RC Tasks Fifo Size in order to avoid deadlock!\n");
        break;

    case EB_ENC_HANDLE_ERROR13:
        fprintf(errorLogFile, "Error: RC Tasks Fifo Size should be greater than EC results Fifo Size in order to avoid deadlock!\n");
        break;

    case EB_ENC_HANDLE_ERROR14:
        fprintf(errorLogFile, "Error: RC Tasks Fifo Size should be greater than Picture Manager results Fifo Size in order to avoid deadlock!\n");
        break;

    case EB_ENC_HANDLE_ERROR18:
        fprintf(errorLogFile, "Error: Intra period setting breaks mini-gop!\n");
        break;

    case EB_ENC_HANDLE_ERROR2:
        fprintf(errorLogFile, "Error: Only one Picture Enhancement Process allowed!\n");
        break;

    case EB_ENC_HANDLE_ERROR3:
        fprintf(errorLogFile, "Error: Only one Picture Manager Process allowed!\n");
        break;

    case EB_ENC_HANDLE_ERROR4:
        fprintf(errorLogFile, "Error: Need at least one ME Process!\n");
        break;

    case EB_ENC_HANDLE_ERROR5:
        fprintf(errorLogFile, "Error: Only one Rate-Control Process allowed!\n");
        break;

    case EB_ENC_HANDLE_ERROR6:
        fprintf(errorLogFile, "Error: Need at least one Mode Decision Configuration Process!\n");
        break;

    case EB_ENC_HANDLE_ERROR7:
        fprintf(errorLogFile, "Error: Need at least one Coding Loop Process!\n");
        break;

    case EB_ENC_HANDLE_ERROR8:
        fprintf(errorLogFile, "Error: Only one Second Pass Deblocking Process allowed!\n");
        break;

    case EB_ENC_HANDLE_ERROR9:
        fprintf(errorLogFile, "Error: Only one ALF Process allowed!\n");
        break;

        // EB_ENC_INTER_ERRORS:
    case EB_ENC_INTER_INVLD_MCP_ERROR:
        fprintf(errorLogFile, "Error: Motion compensation prediction is out of the picture boundary!\n");
        break;

    case EB_ENC_INTER_PRED_ERROR0:
        fprintf(errorLogFile, "Error: Unkown Inter Prediction Direction!\n");
        break;

    case EB_ENC_INTER_PRED_ERROR1:
        fprintf(errorLogFile, "Error: Inter prediction can not support more than 2 MVPs!\n");
        break;

        // EB_ENC_INTRA_ERRORS:
    case EB_ENC_INTRA_PRED_ERROR1:
        fprintf(errorLogFile, "Error: IntraPrediction does not support 2Nx2N partition size!\n");
        break;

    case EB_ENC_INTRA_PRED_ERROR2:
        fprintf(errorLogFile, "Error: IntraPrediction: intra prediction only supports square PU!\n");
        break;

    case EB_ENC_INTRA_PRED_ERROR3:
        fprintf(errorLogFile, "Error: IntraPredictionChroma: Only Planar!\n");
        break;

    case EB_ENC_INVLD_PART_SIZE_ERROR:
        fprintf(errorLogFile, "Error: IntraPrediction: only PU sizes of 8 or largers are currently supported!\n");
        break;


        // EB_ENC_MD_ERRORS:
    case EB_ENC_MD_ERROR1:
        fprintf(errorLogFile, "Error: Unknown AMVP Mode Decision Candidate Type!\n");
        break;

    case EB_ENC_MD_ERROR2:
        fprintf(errorLogFile, "Error: PreModeDecision: need at least one buffer!\n");
        break;

    case EB_ENC_MD_ERROR3:
        fprintf(errorLogFile, "Error: Unknow Inter Prediction Direction!\n");
        break;

    case EB_ENC_MD_ERROR4:
        fprintf(errorLogFile, "Error: Unknown ME SAD Level!\n");
        break;

    case EB_ENC_MD_ERROR5:
        fprintf(errorLogFile, "Error: Invalid encoder mode. The encoder mode should be 0, 1 or 2!\n");
        break;

    case EB_ENC_MD_ERROR6:
        fprintf(errorLogFile, "Error: Invalid TU size!\n");
        break;

    case EB_ENC_MD_ERROR7:
        fprintf(errorLogFile, "Error: Unknown depth!\n");
        break;

    case EB_ENC_MD_ERROR8:
        fprintf(errorLogFile, "Error: Depth not supported!\n");
        break;

    case EB_ENC_MD_ERROR9:
        fprintf(errorLogFile, "Error: Ping-Pong structure needs at least two buffers to work properly\n");
        break;

    case EB_ENC_MD_ERROR10:
        fprintf(errorLogFile, "Error: Ping-Pong structure needs at least two buffers to work properly\n");
        break;

        // EB_ENC_ME_ERRORS:
    case EB_ENC_ME_ERROR1:
        fprintf(errorLogFile, "Error: Motion Estimation: non valid value of the subPelDirection !\n");
        break;

    case EB_ENC_ME_ERROR2:
        fprintf(errorLogFile, "Error: FillMvMergeCandidate() method only supports P or B slices!\n");
        break;

        // EB_ENC_ERRORS:
    case EB_ENC_ROB_OF_ERROR:
        fprintf(errorLogFile, "Error: Recon Output Buffer Overflow!\n");
        break;

        // EB_ENC_PACKETIZATION_ERRORS:
    case EB_ENC_PACKETIZATION_ERROR1:
        fprintf(errorLogFile, "Error: PacketizationProcess: Picture Number does not match entry. PacketizationReorderQueue overflow!\n");
        break;

    case EB_ENC_PACKETIZATION_ERROR2:
        fprintf(errorLogFile, "Error: Entropy Coding Result can not be outputed by processes other than entropy coder and ALF!\n");
        break;

    case EB_ENC_PACKETIZATION_ERROR3:
        fprintf(errorLogFile, "Error: The encoder can not support the SliceMode other than 0 and 1!\n");
        break;

    case EB_ENC_PACKETIZATION_ERROR4:
        fprintf(errorLogFile, "Error: Statistics Output Buffer Overflow!\n");
        break;
    case EB_ENC_PACKETIZATION_ERROR5:
        fprintf(errorLogFile, "Error: Stream Fifo is starving..deadlock, increase EB_outputStreamBufferFifoInitCount APP_ENCODERSTREAMBUFFERCOUNT \n");
        break;

        // EB_ENC_PM_ERRORS:
    case EB_ENC_PM_ERROR0:
        fprintf(errorLogFile, "Error: PictureManagerProcess: Unknown Slice Type!\n");
        break;

    case EB_ENC_PM_ERROR1:
        fprintf(errorLogFile, "Error: EbPictureManager: dependentCount underflow!\n");
        break;

    case EB_ENC_PM_ERROR10:
        fprintf(errorLogFile, "Error: PictureManagerKernel: referenceEntryPtr should never be null!\n");
        break;

    case EB_ENC_PM_ERROR2:
        fprintf(errorLogFile, "Error: PictureManagerProcess: The Reference Structure period must be less than the MAX_ELAPSED_IDR_COUNT or false-IDR boundary logic will be activated!\n");
        break;

    case EB_ENC_PM_ERROR3:
        fprintf(errorLogFile, "Error: PictureManagerProcess: The dependentCount underflow detected!\n");
        break;

    case EB_ENC_PM_ERROR4:
        fprintf(errorLogFile, "Error: PictureManagerProcess: Empty input queue!\n");
        break;

    case EB_ENC_PM_ERROR5:
        fprintf(errorLogFile, "Error: PictureManagerProcess: Empty reference queue!\n");
        break;

    case EB_ENC_PM_ERROR6:
        fprintf(errorLogFile, "Error: PictureManagerProcess: The capped elaspedNonIdrCount must be larger than the maximum supported delta ref poc!\n");
        break;

    case EB_ENC_PM_ERROR7:
        fprintf(errorLogFile, "Error: PictureManagerProcess: Reference Picture Queue Full!\n");
        break;

    case EB_ENC_PM_ERROR8:
        fprintf(errorLogFile, "Error: PictureManagerProcess: No reference match found - this will lead to a memory leak!\n");
        break;

    case EB_ENC_PM_ERROR9:
        fprintf(errorLogFile, "Error: PictureManagerProcess: Unknown picture type!\n");
        break;

    case EB_ENC_PM_ERROR12:
        fprintf(errorLogFile, "Error: PictureManagerProcess: prediction structure configuration API has too many reference pictures\n");
        break;

    case EB_ENC_PM_ERROR13:
        fprintf(errorLogFile, "Error: PictureManagerProcess: The maximum allowed frame rate is 60 fps\n");
        break;

    case EB_ENC_PM_ERROR14:
        fprintf(errorLogFile, "Error: PictureManagerProcess: The minimum allowed frame rate is 1 fps\n");
        break;

        // EB_ENC_PRED_STRC_ERRORS:
    case EB_ENC_PRED_STRC_ERROR1:
        fprintf(errorLogFile, "Error: PredictionStructureCtor: DecodeOrder LUT too small!\n");
        break;

    case EB_ENC_PRED_STRC_ERROR2:
        fprintf(errorLogFile, "Error: PredictionStructureCtor: prediction structure improperly configured!\n");
        break;

        // EB_ENC_PU_ERRORS:
    case EB_ENC_PU_ERROR1:
        fprintf(errorLogFile, "Error: Unknown partition size!\n");
        break;

    case EB_ENC_PU_ERROR2:
        fprintf(errorLogFile, "Error: The target area is not inside the CU!\n");
        break;

        // EB_ENC_RC_ERRORS:
    case EB_ENC_RC_ERROR1:
        fprintf(errorLogFile, "Error: RateControlProcess: Unknown input tasktype!\n");
        break;

    case EB_ENC_RC_ERROR2:
        fprintf(errorLogFile, "Error: RateControlProcess: No RC interval found!\n");
        break;

    case EB_ENC_RC_ERROR3:
        fprintf(errorLogFile, "Error: RateControlProcess: RC input Picture Queue Full!\n");
        break;

    case EB_ENC_RC_ERROR4:
        fprintf(errorLogFile, "Error: RateControlProcess: RC feedback Picture Queue Full!\n");
        break;

    case EB_ENC_RC_ERROR5:
        fprintf(errorLogFile, "Error: RateControlProcess: RC feedback Picture Queue Full!\n");
        break;

    case EB_ENC_RC_ERROR6:
        fprintf(errorLogFile, "Error: RateControlProcess: No feedback frame match found - this will lead to a memory leak!\n");
        break;

    case EB_ENC_RC_ERROR7:
        fprintf(errorLogFile, "Error: remainingBytes has to be multiple of 2 for 16 bit input\n");
        break;

    case EB_ENC_RC_ERROR8:
        fprintf(errorLogFile, "Error: hlRateControlHistorgramQueue Overflow\n");
        break;

        // EB_ENC_RD_COST_ERRORS:
    case EB_ENC_RD_COST_ERROR1:
        fprintf(errorLogFile, "Error: Skip mode only exists in 2Nx2N partition type!\n");
        break;

    case EB_ENC_RD_COST_ERROR2:
        fprintf(errorLogFile, "Error: IntraChromaCost: Unknown slice type!\n");
        break;

    case EB_ENC_RD_COST_ERROR3:
        fprintf(errorLogFile, "Error: Intra2Nx2NFastCostIslice can only support 2Nx2N partition type!\n");
        break;

        // EB_ENC_SAO_ERRORS:
    case EB_ENC_SAO_ERROR1:
        fprintf(errorLogFile, "Error: No more than 6 SAO types!\n");
        break;

    case EB_ENC_SAO_ERROR2:
        fprintf(errorLogFile, "Error: No more than 5 EO SAO categories!\n");
        break;
        // EB_ENC_SCS_ERRORS:
    case EB_ENC_SCS_ERROR1:
        fprintf(errorLogFile, "Error: SequenceControlSetCopy: Not all SequenceControlSet_t members are being copied!\n");
        break;

        // EB_ENC_BITSTREAM_ERRORS:
    case EB_ENC_BITSTREAM_ERROR1:
        fprintf(errorLogFile, "Error: OutputBitstreamRBSPToPayload: Bitstream payload buffer empty!\n");
        break;

    case EB_ENC_BITSTREAM_ERROR2:
        fprintf(errorLogFile, "Error: OutputBitstreamWrite: Empty bitstream!\n");
        break;

    case EB_ENC_BITSTREAM_ERROR3:
        fprintf(errorLogFile, "Error: OutputBitstreamRBSPToPayload: Buffer index more than buffer size!\n");
        break;

    case EB_ENC_BITSTREAM_ERROR4:
        fprintf(errorLogFile, "Error: OutputBitstreamRBSPToPayload: Start Location in not inside the buffer!\n");
        break;

    case EB_ENC_BITSTREAM_ERROR5:
        fprintf(errorLogFile, "Error: OutputBitstreamWrite: Trying to write more than one word!\n");
        break;

    case EB_ENC_BITSTREAM_ERROR6:
        fprintf(errorLogFile, "Error: OutputBitstreamRBSPToPayload: Expecting Start code!\n");
        break;

    case EB_ENC_BITSTREAM_ERROR7:
        fprintf(errorLogFile, "Error: OutputBitstreamRBSPToPayload: Bitstream not flushed (i.e. byte-aligned)!\n");
        break;

    case EB_ENC_RESS_COOR_ERRORS1:
        fprintf(errorLogFile, "Error: ResourceCoordinationProcess: The received input data should be equal to the buffer size - only complete frame transfer is supported\n");
        break;

    case EB_ENC_RES_COORD_InvalidQP:
        fprintf(errorLogFile, "Error: ResourceCoordinationProcess: The QP value in the QP file is invalid\n");
        break;

    case EB_ENC_RES_COORD_InvalidSliceType:
        fprintf(errorLogFile, "Error: ResourceCoordinationProcess: Slice Type Invalid\n");
        break;

        // picture decision Errors
    case EB_ENC_PD_ERROR8:
        fprintf(errorLogFile, "Error: PictureDecisionProcess: Picture Decision Reorder Queue overflow\n");
        break;

    default:
        fprintf(errorLogFile, "Error: Others!\n");
        break;
    }

    return;
}

/******************************************************
* Copy fields from the stream to the input buffer
    Input   : stream
    Output  : valid input buffer
******************************************************/
void ProcessInputFieldStandardMode(

    EbConfig_t               *config,
    EB_BUFFERHEADERTYPE      *headerPtr,
    FILE                     *inputFile,
    uint8_t                    *lumaInputPtr,
    uint8_t                    *cbInputPtr,
    uint8_t                    *crInputPtr,
    uint8_t                   is16bit)
{
    const int64_t inputPaddedWidth  = config->inputPaddedWidth;
    const int64_t inputPaddedHeight = config->inputPaddedHeight;
    const EB_COLOR_FORMAT colorFormat = (EB_COLOR_FORMAT)config->encoderColorFormat;
    const uint8_t subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const uint8_t subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    //const uint8_t is16bit = (config->encoderBitDepth > 8) ? 1 : 0;
    uint64_t sourceLumaRowSize = (uint64_t)(inputPaddedWidth << is16bit);
    uint64_t sourceChromaRowSize = sourceLumaRowSize >> subWidthCMinus1;
    uint8_t  *ebInputPtr;
    uint32_t  inputRowIndex;

    // Y
    ebInputPtr = lumaInputPtr;
    // Skip 1 luma row if bottom field (point to the bottom field)
    if (config->processedFrameCount % 2 != 0)
        fseeko64(inputFile, (long)sourceLumaRowSize, SEEK_CUR);

    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight; inputRowIndex++) {

        headerPtr->nFilledLen += (uint32_t)fread(ebInputPtr, 1, sourceLumaRowSize, inputFile);
        // Skip 1 luma row (only fields)
        fseeko64(inputFile, (long)sourceLumaRowSize, SEEK_CUR);
        ebInputPtr += sourceLumaRowSize;
    }

    // U
    ebInputPtr = cbInputPtr;
    // Step back 1 luma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field)
    if (config->processedFrameCount % 2 != 0) {
        fseeko64(inputFile, -(long)sourceLumaRowSize, SEEK_CUR);
        fseeko64(inputFile, (long)sourceChromaRowSize, SEEK_CUR);
    }

    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight >> subHeightCMinus1; inputRowIndex++) {

        headerPtr->nFilledLen += (uint32_t)fread(ebInputPtr, 1, sourceChromaRowSize, inputFile);
        // Skip 1 chroma row (only fields)
        fseeko64(inputFile, (long)sourceChromaRowSize, SEEK_CUR);
        ebInputPtr += sourceChromaRowSize;
    }

    // V
    ebInputPtr = crInputPtr;
    // Step back 1 chroma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field)
    // => no action


    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight >> subHeightCMinus1; inputRowIndex++) {

        headerPtr->nFilledLen += (uint32_t)fread(ebInputPtr, 1, sourceChromaRowSize, inputFile);
        // Skip 1 chroma row (only fields)
        fseeko64(inputFile, (long)sourceChromaRowSize, SEEK_CUR);
        ebInputPtr += sourceChromaRowSize;
    }

    // Step back 1 chroma row if bottom field (undo the previous jump)
    if (config->processedFrameCount % 2 != 0) {
        fseeko64(inputFile, -(long)sourceChromaRowSize, SEEK_CUR);
    }
}


//************************************/
// GetNextQpFromQpFile
// Reads and extracts one qp from the qpfile
// Input  : QP file
// Output : QP value
/************************************/
static int32_t qpReadFromFile = 0;

int32_t GetNextQpFromQpFile(
    EbConfig_t  *config
)
{
    uint8_t *line;
    int32_t qp = 0;
    uint32_t readsize = 0, eof = 0;
    EB_APP_MALLOC(uint8_t*, line, 8, EB_N_PTR, EB_ErrorInsufficientResources);
    memset(line,0,8);
    readsize = (uint32_t)fread(line, 1, 2, config->qpFile);

    if (readsize == 0) {
        // end of file
        return -1;
    }
    else if (readsize == 1) {
        qp = strtol((const char*)line, NULL, 0);
        if (qp == 0) // eof
            qp = -1;
    }
    else if (readsize == 2 && (line[0] == '\n')) {
        // new line
        fseek(config->qpFile, -1, SEEK_CUR);
        qp = 0;
    }
    else if (readsize == 2 && (line[1] == '\n')) {
        // new line
        qp = strtol((const char*)line, NULL, 0);
    }
    else if (readsize == 2 && (line[0] == '#' || line[0] == '/' || line[0] == '-' || line[0] == ' ')) {
        // Backup one step to not miss the new line char
        fseek(config->qpFile, -1, SEEK_CUR);
        do {
            readsize = (uint32_t)fread(line, 1, 1, config->qpFile);
            if (readsize != 1)
                break;
        } while (line[0] != '\n');

        if (eof != 0)
            // end of file
            qp = -1;
        else
            // skip line
            qp = 0;
    }
    else if (readsize == 2) {
        qp = strtol((const char*)line, NULL, 0);
        do {
            readsize = (uint32_t)fread(line, 1, 1, config->qpFile);
            if (readsize != 1)
                break;
        } while (line[0] != '\n');
    }

    if (qp > 0)
        qpReadFromFile |= 1;

    return qp;
}

static void ReadInputFrames(
    EbConfig_t                  *config,
    uint8_t                      is16bit,
    EB_BUFFERHEADERTYPE         *headerPtr)
{
    const uint32_t  inputPaddedWidth = config->inputPaddedWidth;
    const uint32_t  inputPaddedHeight = config->inputPaddedHeight;
    FILE   *inputFile = config->inputFile;
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;
    const EB_COLOR_FORMAT colorFormat = (EB_COLOR_FORMAT)config->encoderColorFormat;
    const uint8_t subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const uint8_t subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    //const uint8_t is16bit = (config->encoderBitDepth > 8) ? 1 : 0;

    inputPtr->yStride  = inputPaddedWidth;
    inputPtr->crStride = inputPaddedWidth >> subWidthCMinus1;
    inputPtr->cbStride = inputPaddedWidth >> subWidthCMinus1;
    inputPtr->dolbyVisionRpu.payloadSize = 0;

    if (config->bufferedInput == -1) {
        if (is16bit == 0 || (is16bit == 1 && config->compressedTenBitFormat == 0)) {

            uint32_t readSize = SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, colorFormat, is16bit);

            headerPtr->nFilledLen = 0;

            // Interlaced Video
            if (config->separateFields) {

                ProcessInputFieldStandardMode(
                    config,
                    headerPtr,
                    inputFile,
                    inputPtr->luma,
                    inputPtr->cb,
                    inputPtr->cr,
                    is16bit);

                if (readSize != headerPtr->nFilledLen) {

                    fseek(inputFile, 0, SEEK_SET);
                    headerPtr->nFilledLen = 0;

                    ProcessInputFieldStandardMode(
                        config,
                        headerPtr,
                        inputFile,
                        inputPtr->luma,
                        inputPtr->cb,
                        inputPtr->cr,
                        is16bit);
                }

                // Reset the pointer position after a top field
                if (config->processedFrameCount % 2 == 0) {
                    fseek(inputFile, -(long)(readSize << 1), SEEK_CUR);
                }
            } else {
                const uint32_t lumaReadSize = inputPaddedWidth * inputPaddedHeight << is16bit;
                const uint32_t chromaReadSize = lumaReadSize >> (3 - colorFormat);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->luma, 1, lumaReadSize, inputFile);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cb, 1, chromaReadSize, inputFile);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cr, 1, chromaReadSize, inputFile);


                if (readSize != headerPtr->nFilledLen) {

                    fseek(inputFile, 0, SEEK_SET);
                    headerPtr->nFilledLen += (uint32_t)fread(inputPtr->luma, 1, lumaReadSize, inputFile);
                    headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cb, 1, chromaReadSize, inputFile);
                    headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cr, 1, chromaReadSize, inputFile);

                    inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING));
                    inputPtr->cb = inputPtr->cb + (((config->inputPaddedWidth >> subWidthCMinus1)*(TOP_INPUT_PADDING >> subHeightCMinus1) + (LEFT_INPUT_PADDING >> subWidthCMinus1)));
                    inputPtr->cr = inputPtr->cr + (((config->inputPaddedWidth >> subWidthCMinus1)*(TOP_INPUT_PADDING >> subHeightCMinus1) + (LEFT_INPUT_PADDING >> subWidthCMinus1)));

                }
            }
        } else {
            // 10-bit Compressed Unpacked Mode
            const uint32_t lumaReadSize = inputPaddedWidth * inputPaddedHeight;
            const uint32_t chromaReadSize = lumaReadSize >> (3 - colorFormat);
            const uint32_t nbitLumaReadSize = (inputPaddedWidth / 4) * inputPaddedHeight;
            const uint32_t nbitChromaReadSize = nbitLumaReadSize >> (3 - colorFormat);

            // Fill the buffer with a complete frame
            headerPtr->nFilledLen = 0;
            headerPtr->nFilledLen += (uint32_t)fread(inputPtr->luma, 1, lumaReadSize, inputFile);
            headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cb, 1, chromaReadSize, inputFile);
            headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cr, 1, chromaReadSize, inputFile);

            headerPtr->nFilledLen += (uint32_t)fread(inputPtr->lumaExt, 1, nbitLumaReadSize, inputFile);
            headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cbExt, 1, nbitChromaReadSize, inputFile);
            headerPtr->nFilledLen += (uint32_t)fread(inputPtr->crExt, 1, nbitChromaReadSize, inputFile);

            if (headerPtr->nFilledLen != (lumaReadSize + nbitLumaReadSize + 2 * (chromaReadSize + nbitChromaReadSize))) {
                fseek(inputFile, 0, SEEK_SET);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->luma, 1, lumaReadSize, inputFile);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cb, 1, chromaReadSize, inputFile);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cr, 1, chromaReadSize, inputFile);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->lumaExt, 1, nbitLumaReadSize, inputFile);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->cbExt, 1, nbitChromaReadSize, inputFile);
                headerPtr->nFilledLen += (uint32_t)fread(inputPtr->crExt, 1, nbitChromaReadSize, inputFile);
            }
        }
    } else {
        if (is16bit && config->compressedTenBitFormat == 1) {
            // Determine size of each plane

            const size_t luma8bitSize = config->inputPaddedWidth * config->inputPaddedHeight;
            const size_t chroma8bitSize = luma8bitSize >> (3 - colorFormat);

            const size_t luma2bitSize = luma8bitSize / 4; //4-2bit pixels into 1 byte
            const size_t chroma2bitSize = luma2bitSize >> (3 - colorFormat);

            EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;
            inputPtr->yStride = config->inputPaddedWidth;
            inputPtr->crStride = config->inputPaddedWidth >> subWidthCMinus1;
            inputPtr->cbStride = config->inputPaddedWidth >> subWidthCMinus1;

            inputPtr->luma = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput];
            inputPtr->cb = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize;
            inputPtr->cr = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + chroma8bitSize;

            inputPtr->lumaExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize;
            inputPtr->cbExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize + luma2bitSize;
            inputPtr->crExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize + luma2bitSize + chroma2bitSize;

            headerPtr->nFilledLen = luma8bitSize + luma2bitSize + 2 * (chroma8bitSize + chroma2bitSize);
        } else {
            //Normal unpacked mode:yuv420p10le yuv422p10le yuv444p10le

            // Determine size of each plane
            const size_t lumaSize = (config->inputPaddedWidth * config->inputPaddedHeight) << is16bit;
            const size_t chromaSize = lumaSize >> (3 - colorFormat);

            EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;

            inputPtr->yStride = config->inputPaddedWidth;
            inputPtr->crStride = config->inputPaddedWidth >> subWidthCMinus1;
            inputPtr->cbStride = config->inputPaddedWidth >> subWidthCMinus1;

            inputPtr->luma = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput];
            inputPtr->cb = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + lumaSize;
            inputPtr->cr = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + lumaSize + chromaSize;

            headerPtr->nFilledLen = (uint32_t)(uint64_t)SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, colorFormat, is16bit);

        }
    }

    // If we reached the end of file, loop over again
    if (feof(inputFile) != 0) {
        fseek(inputFile, 0, SEEK_SET);
    }

    return;
}

void SendQpOnTheFly(
    EbConfig_t                  *config,
    EB_BUFFERHEADERTYPE        *headerPtr)
{
    {
        uint32_t           qpPtr;
        int32_t           tmpQp = 0;

        do {
            // get next qp
            tmpQp = GetNextQpFromQpFile(config);

            if (tmpQp == (int32_t)EB_ErrorInsufficientResources) {
                printf("Malloc has failed due to insuffucient resources");
                return;
            }

            // check if eof
            if ((tmpQp == -1) && (qpReadFromFile != 0))
                fseek(config->qpFile, 0, SEEK_SET);

            // check if the qp read is valid
            else if (tmpQp > 0)
                break;

        } while (tmpQp == 0 || ((tmpQp == -1) && (qpReadFromFile != 0)));

        if (tmpQp == -1) {
            config->useQpFile = EB_FALSE;
            printf("\nSVT [Warning]: QP File did not contain any valid QPs");
        }

        qpPtr = CLIP3(0, 51, tmpQp);

        headerPtr->qpValue = qpPtr;
    }
    return;
}

void SendNaluOnTheFly(
    EbConfig_t                  *config,
    EB_BUFFERHEADERTYPE        *headerPtr)
{
    {
        char line[1024];
        EB_ERRORTYPE return_error = EB_ErrorNone;
        uint32_t poc = 0;
        uint8_t *prefix = NULL;
        uint32_t nalType = 0;
        uint32_t payloadType = 0;
        uint8_t *base64Encode = NULL;
        uint8_t *context = NULL;

        if (fgets(line, sizeof(line), config->naluFile) != NULL) {
            poc = strtol(EB_STRTOK(line, " ", &context), NULL, 0);
            if (return_error == EB_ErrorNone && *context != 0) {
                prefix = (uint8_t*)EB_STRTOK(NULL, " ", &context);
            }
            else {
                return_error = EB_ErrorBadParameter;
            }
            if (return_error == EB_ErrorNone && *context != 0) {
                nalType = strtol(EB_STRTOK(NULL, "/", &context), NULL, 0);
            }
            else {
                return_error = EB_ErrorBadParameter;
            }
            if (return_error == EB_ErrorNone && *context != 0) {
                payloadType = strtol(EB_STRTOK(NULL, " ", &context), NULL, 0);
            }
            else {
                return_error = EB_ErrorBadParameter;
            }
            if (return_error == EB_ErrorNone && *context != 0) {
                base64Encode = (uint8_t*)EB_STRTOK(NULL, "\n", &context);
            }
            else {
                return_error = EB_ErrorBadParameter;
            }

            headerPtr->naluPOC = poc;
            if (prefix != NULL && !EB_STRCMP((char*)prefix, "PREFIX"))
                headerPtr->naluPrefix = 0;
            headerPtr->naluNalType = nalType;
            headerPtr->naluPayloadType = payloadType;
            headerPtr->naluBase64Encode = base64Encode;
            headerPtr->naluFound = EB_TRUE;
        }
        else {
            return_error = EB_ErrorBadParameter;
        }
        if (return_error != EB_ErrorNone) {
            printf("\nSVT [Warning]: Nalu file cannot be parsed correctly \n ");
            headerPtr->naluFound = EB_FALSE;
            //config->useNaluFile = EB_FALSE;
            return;
        }
    }
    return;
}

#define START_CODE 0x00000001
#define START_CODE_BYTES 4

int ParseDolbyVisionRPUMetadata(
    EbConfig_t               *config,
    EB_BUFFERHEADERTYPE      *headerPtr)
{
    uint8_t byteVal = 0;
    uint32_t code = 0;
    uint32_t bytesRead = 0;
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;
    FILE* ptr = config->dolbyVisionRpuFile;

    if (!headerPtr->pts) {
        while (bytesRead++ < 4 && fread(&byteVal, sizeof(uint8_t), 1, ptr))
            code = (code << 8) | byteVal;

        if (code != START_CODE) {
            printf("Warning : Invalid Dolby Vision RPU startcode in POC  %" PRId64 "\n", headerPtr->pts);
            return 1;
        }
    }

    bytesRead = 0;
    while (fread(&byteVal, sizeof(uint8_t), 1, ptr))
    {
        code = (code << 8) | byteVal;
        if (bytesRead++ < 3)
            continue;
        if (bytesRead >= 1024) {
            printf("Warning : Invalid Dolby Vision RPU size in POC  %" PRId64 "\n", headerPtr->pts);
            return 1;
        }

        if (code != START_CODE)
            inputPtr->dolbyVisionRpu.payload[inputPtr->dolbyVisionRpu.payloadSize++] = (code >> (3 * 8)) & 0xFF;
        else
            return 0;

    }

    int ShiftBytes = START_CODE_BYTES - (bytesRead - inputPtr->dolbyVisionRpu.payloadSize);
    int bytesLeft = bytesRead - inputPtr->dolbyVisionRpu.payloadSize;
    code = (code << ShiftBytes * 8);
    for (int i = 0; i < bytesLeft; i++) {
        inputPtr->dolbyVisionRpu.payload[inputPtr->dolbyVisionRpu.payloadSize++] = (code >> (3 * 8)) & 0xFF;
        code = (code << 8);
    }
    if (!inputPtr->dolbyVisionRpu.payloadSize) {
        printf("Warning : Dolby Vision RPU not found for POC  %" PRId64 "\n", headerPtr->pts);
        return 1;
    }
    return 0;

}

//************************************/
// ProcessInputBuffer
// Reads yuv frames from file and copy
// them into the input buffer
/************************************/
APPEXITCONDITIONTYPE ProcessInputBuffer(EbConfig_t *config, EbAppContext_t *appCallBack)
{
    uint8_t            is16bit = (uint8_t)(config->encoderBitDepth > 8);
    EB_BUFFERHEADERTYPE     *headerPtr = appCallBack->inputBufferPool;
    EB_COMPONENTTYPE        *componentHandle = (EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle;

    APPEXITCONDITIONTYPE    return_value = APP_ExitConditionNone;

    int64_t                  inputPaddedWidth           = config->inputPaddedWidth;
    int64_t                  inputPaddedHeight          = config->inputPaddedHeight;
    int64_t                  framesToBeEncoded          = config->framesToBeEncoded;
    int64_t                  totalBytesToProcessCount;
    int64_t                  remainingByteCount;
    const EB_COLOR_FORMAT colorFormat = (EB_COLOR_FORMAT)config->encoderColorFormat;
    int ret;
    uint32_t compressed10bitFrameSize = (inputPaddedWidth*inputPaddedHeight) + 2 * ((inputPaddedWidth*inputPaddedWidth) >> (3 - colorFormat));
    compressed10bitFrameSize += compressed10bitFrameSize / 4;

    if (config->injector && config->processedFrameCount)
    {
        EbInjector(config->processedFrameCount, config->injectorFrameRate);
    }

	totalBytesToProcessCount = (framesToBeEncoded < 0) ? -1 : (config->encoderBitDepth == 10 && config->compressedTenBitFormat == 1) ?
		framesToBeEncoded * compressed10bitFrameSize:
        framesToBeEncoded * SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, colorFormat, is16bit);


    remainingByteCount       = (totalBytesToProcessCount < 0) ?   -1 :  totalBytesToProcessCount - (int64_t)config->processedByteCount;

    // If there are bytes left to encode, configure the header
    if (remainingByteCount != 0 && config->stopEncoder == EB_FALSE) {
        ReadInputFrames(
            config,
            is16bit,
            headerPtr);

        // Update the context parameters
        config->processedByteCount += headerPtr->nFilledLen;
        headerPtr->pAppPrivate          = (EB_PTR)EB_NULL;
        config->framesEncoded           = (int32_t)(++config->processedFrameCount);

        // Configuration parameters changed on the fly
        if (config->useQpFile && config->qpFile)
            SendQpOnTheFly(
                config,
                headerPtr);

        // Configuration parameters changed on the fly
        if (config->useNaluFile && config->naluFile)
            SendNaluOnTheFly(
                config,
                headerPtr);

        if (keepRunning == 0 && !config->stopEncoder) {
            config->stopEncoder = EB_TRUE;
        }

        // Fill in Buffers Header control data
        headerPtr->pts          = config->processedFrameCount-1;
        headerPtr->sliceType    = EB_INVALID_PICTURE;

        headerPtr->nFlags = 0;

        if (config->dolbyVisionProfile == 81 && config->dolbyVisionRpuFile) {
            ret = ParseDolbyVisionRPUMetadata(
                  config,
                  headerPtr);
            if (ret) {
                printf("\n Warning : Dolby vision RPU not parsed for POC %" PRId64 "\t", headerPtr->pts);
            }
        }

        // Send the picture
        EbH265EncSendPicture(componentHandle, headerPtr);

        if ((config->processedFrameCount == (uint64_t)config->framesToBeEncoded) || config->stopEncoder) {

            headerPtr->nAllocLen    = 0;
            headerPtr->nFilledLen   = 0;
            headerPtr->nTickCount   = 0;
            headerPtr->pAppPrivate  = NULL;
            headerPtr->nFlags       = EB_BUFFERFLAG_EOS;
            headerPtr->pBuffer      = NULL;
            headerPtr->sliceType    = EB_INVALID_PICTURE;

            EbH265EncSendPicture(componentHandle, headerPtr);

        }

        return_value = (headerPtr->nFlags == EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : return_value;

    }

    return return_value;
}

#define LONG_ENCODE_FRAME_ENCODE    4000
#define SPEED_MEASUREMENT_INTERVAL  2000
#define START_STEADY_STATE          1000

APPEXITCONDITIONTYPE ProcessOutputStreamBuffer(
    EbConfig_t             *config,
    EbAppContext_t         *appCallBack,
    uint8_t                 picSendDone)
{
    APPPORTACTIVETYPE      *portState       = &appCallBack->outputStreamPortActive;
    EB_BUFFERHEADERTYPE    *headerPtr;
    EB_COMPONENTTYPE       *componentHandle = (EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle;
    APPEXITCONDITIONTYPE    return_value    = APP_ExitConditionNone;
    EB_ERRORTYPE            stream_status   = EB_ErrorNone;
    // Per channel variables
    FILE                  *streamFile       = config->bitstreamFile;

    uint64_t              *totalLatency     = &config->performanceContext.totalLatency;
    uint32_t              *maxLatency       = &config->performanceContext.maxLatency;

    // System performance variables
    static int32_t         frameCount       = 0;

    // Local variables
    uint64_t               finishsTime      = 0;
    uint64_t               finishuTime      = 0;

    // non-blocking call until all input frames are sent
    stream_status = EbH265GetPacket(componentHandle, &headerPtr, picSendDone);

    if (stream_status == EB_ErrorMax) {
        printf("\n");
        LogErrorOutput(
            config->errorLogFile,
            headerPtr->nFlags);
        return APP_ExitConditionError;
    }
    else if (stream_status != EB_NoErrorEmptyQueue) {
        ++(config->performanceContext.frameCount);
        *totalLatency += (uint64_t)headerPtr->nTickCount;
        *maxLatency = (headerPtr->nTickCount > *maxLatency) ? headerPtr->nTickCount : *maxLatency;
        
        EbFinishTime((uint64_t*)&finishsTime, (uint64_t*)&finishuTime);
        // total execution time, inc init time
        EbComputeOverallElapsedTime(
            config->performanceContext.libStartTime[0],
            config->performanceContext.libStartTime[1],
            finishsTime,
            finishuTime,
            &config->performanceContext.totalExecutionTime);

        // total encode time
        EbComputeOverallElapsedTime(
            config->performanceContext.encodeStartTime[0],
            config->performanceContext.encodeStartTime[1],
            finishsTime,
            finishuTime,
            &config->performanceContext.totalEncodeTime);

        // Write Stream Data to file
        if (streamFile) {
            fwrite(headerPtr->pBuffer, 1, headerPtr->nFilledLen, streamFile);
        }
        config->performanceContext.byteCount += headerPtr->nFilledLen;

        if ((headerPtr->nFlags & EB_BUFFERFLAG_EOS) && appCallBack->ebEncParameters.codeEosNal == 0) {
            EB_BUFFERHEADERTYPE *outputStreamBuffer;
            stream_status = EbH265EncEosNal(componentHandle, &outputStreamBuffer);
            if (stream_status == EB_ErrorMax) {
                printf("\n");
                LogErrorOutput(
                    config->errorLogFile,
                    stream_status);
                return APP_ExitConditionError;
            }
            else if (stream_status != EB_NoErrorEmptyQueue && streamFile) {
                fwrite(outputStreamBuffer->pBuffer, 1, outputStreamBuffer->nFilledLen, streamFile);
            }
            config->performanceContext.byteCount += outputStreamBuffer->nFilledLen;
        }
        // Update Output Port Activity State
        *portState = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? APP_PortInactive : *portState;
        return_value = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : APP_ExitConditionNone;

        // Release the output buffer
        if (stream_status != EB_NoErrorEmptyQueue)
            EbH265ReleaseOutBuffer(&headerPtr);

#if DEADLOCK_DEBUG
        ++frameCount;
#else
        //++frameCount;
        printf("\b\b\b\b\b\b\b\b\b%9d", ++frameCount);
#endif

        //++frameCount;
        fflush(stdout);

        // Queue the buffer again if the port is still active
        {
            config->performanceContext.averageSpeed = (config->performanceContext.frameCount) / config->performanceContext.totalEncodeTime;
            config->performanceContext.averageLatency = config->performanceContext.totalLatency / (double)(config->performanceContext.frameCount);
        }

        if (!(frameCount % SPEED_MEASUREMENT_INTERVAL)) {
            {
                printf("\n");
                printf("Average System Encoding Speed:        %.2f\n", (double)(frameCount) / config->performanceContext.totalEncodeTime);
            }
        }
    }
	return return_value;
}
APPEXITCONDITIONTYPE ProcessOutputReconBuffer(
    EbConfig_t             *config,
    EbAppContext_t         *appCallBack)
{
    EB_BUFFERHEADERTYPE    *headerPtr = appCallBack->reconBuffer; // needs to change for buffered input
    EB_COMPONENTTYPE       *componentHandle = (EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle;
    APPEXITCONDITIONTYPE    return_value = APP_ExitConditionNone;
    EB_ERRORTYPE            recon_status = EB_ErrorNone;
    int32_t fseekReturnVal;
    // non-blocking call until all input frames are sent
    recon_status = EbH265GetRecon(componentHandle, headerPtr);

    if (recon_status == EB_ErrorMax) {
        printf("\n");
        LogErrorOutput(
            config->errorLogFile,
            headerPtr->nFlags);
        return APP_ExitConditionError;
    }
    else if (recon_status != EB_NoErrorEmptyQueue) {
        //Sets the File position to the beginning of the file.
        rewind(config->reconFile);
        uint64_t frameNum = headerPtr->pts;
        while (frameNum>0) {
            fseekReturnVal = fseeko64(config->reconFile, headerPtr->nFilledLen, SEEK_CUR);

            if (fseekReturnVal != 0) {
                printf("Error in fseeko64  returnVal %i\n", fseekReturnVal);
                return APP_ExitConditionError;
            }
            frameNum = frameNum - 1;
        }

        fwrite(headerPtr->pBuffer, 1, headerPtr->nFilledLen, config->reconFile);

        // Update Output Port Activity State
        return_value = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : APP_ExitConditionNone;
    }
    return return_value;
}

