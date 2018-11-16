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

#include "EbAppContext.h"
#include "EbAppConfig.h"
#include "EbErrorCodes.h"

#include "EbTime.h"
/***************************************
 * Macros
 ***************************************/

#define CLIP3(MinVal, MaxVal, a)        (((a)<(MinVal)) ? (MinVal) : (((a)>(MaxVal)) ? (MaxVal) :(a)))
#define FUTURE_WINDOW_WIDTH                 4
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height,is16bit) ( ( ((width)*(height)*3)>>1 )<<is16bit)
extern volatile int keepRunning;

/***************************************
* Process Error Log
***************************************/
void LogErrorOutput(
    FILE                    *errorLogFile,
    EB_U32                  errorCode)
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
    EB_BUFFERHEADERTYPE     *headerPtr,
    FILE                     *inputFile,
    EB_U8                   *lumaInputPtr,
    EB_U8                   *cbInputPtr,
    EB_U8                   *crInputPtr,
    unsigned char             is16bit) {


    EB_S64  inputPaddedWidth  = config->inputPaddedWidth;
    EB_S64  inputPaddedHeight = config->inputPaddedHeight;

    EB_U64  sourceLumaRowSize = (EB_U64)(inputPaddedWidth << is16bit);

    EB_U64  sourceChromaRowSize = sourceLumaRowSize >> 1;
   
    EB_U8  *ebInputPtr;
    EB_U32  inputRowIndex;

    // Y
    ebInputPtr = lumaInputPtr;
    // Skip 1 luma row if bottom field (point to the bottom field)
    if (config->processedFrameCount % 2 != 0)
        fseeko64(inputFile, (long)sourceLumaRowSize, SEEK_CUR);

    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight; inputRowIndex++) {

        headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, sourceLumaRowSize, inputFile);
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

    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight >> 1; inputRowIndex++) {

        headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, sourceChromaRowSize, inputFile);
        // Skip 1 chroma row (only fields)
        fseeko64(inputFile, (long)sourceChromaRowSize, SEEK_CUR);
        ebInputPtr += sourceChromaRowSize;
    }

    // V
    ebInputPtr = crInputPtr;
    // Step back 1 chroma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field) 
    // => no action


    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight >> 1; inputRowIndex++) {

        headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, sourceChromaRowSize, inputFile);
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
static int qpReadFromFile = 0;

EB_S32 GetNextQpFromQpFile(
    EbConfig_t  *config
)
{
    EB_U8 *line;
    EB_S32 qp = 0;
    EB_U32 readsize = 0, eof = 0;
    EB_APP_MALLOC(EB_U8*, line, 8, EB_N_PTR, EB_ErrorInsufficientResources);
    memset(line,0,8);
    readsize = (EB_U32)fread(line, 1, 2, config->qpFile);

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
            readsize = (EB_U32)fread(line, 1, 1, config->qpFile);
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
            readsize = (EB_U32)fread(line, 1, 1, config->qpFile);
            if (readsize != 1)
                break;
        } while (line[0] != '\n');
    }

    if (qp > 0)
        qpReadFromFile |= 1;

    return qp;
}

void ReadInputFrames(
    EbConfig_t                  *config,
    unsigned char                is16bit,
    EB_BUFFERHEADERTYPE         *headerPtr)
{

    EB_U64  readSize;
    EB_U32  inputPaddedWidth = config->inputPaddedWidth;
    EB_U32  inputPaddedHeight = config->inputPaddedHeight;
    FILE   *inputFile = config->inputFile;
    EB_U8  *ebInputPtr;
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;

    EB_U64 frameSize = (EB_U64)((inputPaddedWidth*inputPaddedHeight * 3) / 2 + (inputPaddedWidth / 4 * inputPaddedHeight * 3) / 2);
    inputPtr->yStride  = inputPaddedWidth;
    inputPtr->crStride = inputPaddedWidth >> 1;
    inputPtr->cbStride = inputPaddedWidth >> 1;

    if (config->bufferedInput == -1) {

        if (is16bit == 0 || (is16bit == 1 && config->compressedTenBitFormat == 0)) {

            readSize = (EB_U64)SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, is16bit);

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
            }
            else {
                EB_U64 lumaReadSize = (EB_U64)inputPaddedWidth*inputPaddedHeight << is16bit;
                ebInputPtr = inputPtr->luma;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
                ebInputPtr = inputPtr->cb;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                ebInputPtr = inputPtr->cr;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING) << is16bit);
                inputPtr->cb   = inputPtr->cb + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)) << is16bit);
                inputPtr->cr   = inputPtr->cr + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)) << is16bit);


                if (readSize != headerPtr->nFilledLen) {

                    fseek(inputFile, 0, SEEK_SET);
                    ebInputPtr = inputPtr->luma;
                    headerPtr->nFilledLen = (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
                    ebInputPtr = inputPtr->cb;
                    headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                    ebInputPtr = inputPtr->cr;
                    headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);

                    inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING));
                    inputPtr->cb = inputPtr->cb + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));
                    inputPtr->cr = inputPtr->cr + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));

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

                fseek(inputFile, 0, SEEK_SET);
                ebInputPtr = inputPtr->luma;
                headerPtr->nFilledLen = (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
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

            }

        }

        // 10-bit Unpacked Mode
        else {

            readSize = (EB_U64)SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, 1);

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
                    0);

                ProcessInputFieldStandardMode(
                    config,
                    headerPtr,
                    inputFile,
                    inputPtr->lumaExt,
                    inputPtr->cbExt,
                    inputPtr->crExt,
                    0);

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
                        0);

                    ProcessInputFieldStandardMode(
                        config,
                        headerPtr,
                        inputFile,
                        inputPtr->lumaExt,
                        inputPtr->cbExt,
                        inputPtr->crExt,
                        0);
                }

                // Reset the pointer position after a top field
                if (config->processedFrameCount % 2 == 0) {
                    fseek(inputFile, -(long)(readSize << 1), SEEK_CUR);
                }

            }
            else {


                EB_U64 lumaReadSize = (EB_U64)inputPaddedWidth*inputPaddedHeight;

                ebInputPtr = inputPtr->luma;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
                ebInputPtr = inputPtr->cb;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                ebInputPtr = inputPtr->cr;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);

                inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING));
                inputPtr->cb = inputPtr->cb + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));
                inputPtr->cr = inputPtr->cr + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));

                ebInputPtr = inputPtr->lumaExt;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
                ebInputPtr = inputPtr->cbExt;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                ebInputPtr = inputPtr->crExt;
                headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);

                inputPtr->lumaExt = inputPtr->lumaExt + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING));
                inputPtr->cbExt = inputPtr->cbExt + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));
                inputPtr->crExt = inputPtr->crExt + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));

                if (readSize != headerPtr->nFilledLen) {

                    fseek(inputFile, 0, SEEK_SET);
                    ebInputPtr = inputPtr->luma;
                    headerPtr->nFilledLen = (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
                    ebInputPtr = inputPtr->cb;
                    headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                    ebInputPtr = inputPtr->cr;
                    headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);

                    inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING));
                    inputPtr->cb = inputPtr->cb + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));
                    inputPtr->cr = inputPtr->cr + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));

                    ebInputPtr = inputPtr->lumaExt;
                    headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize, inputFile);
                    ebInputPtr = inputPtr->cbExt;
                    headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);
                    ebInputPtr = inputPtr->crExt;
                    headerPtr->nFilledLen += (EB_U32)fread(ebInputPtr, 1, lumaReadSize >> 2, inputFile);

                    inputPtr->lumaExt = inputPtr->lumaExt + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING));
                    inputPtr->cbExt = inputPtr->cbExt + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));
                    inputPtr->crExt = inputPtr->crExt + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));

                }

            }




        }

    }
    else {
        if (config->encoderBitDepth == 10 && config->compressedTenBitFormat == 1)
        {
            // Determine size of each plane

            const size_t luma8bitSize = config->inputPaddedWidth * config->inputPaddedHeight;
            const size_t chroma8bitSize = luma8bitSize >> 2;

            const size_t luma2bitSize = luma8bitSize / 4; //4-2bit pixels into 1 byte
            const size_t chroma2bitSize = luma2bitSize >> 2;

            EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;
            inputPtr->yStride = config->inputPaddedWidth;
            inputPtr->crStride = config->inputPaddedWidth >> 1;
            inputPtr->cbStride = config->inputPaddedWidth >> 1;

            inputPtr->luma = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput];
            inputPtr->cb = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize;
            inputPtr->cr = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + chroma8bitSize;

            inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING));
            inputPtr->cb = inputPtr->cb + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));
            inputPtr->cr = inputPtr->cr + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)));

            if (is16bit) {
                inputPtr->lumaExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize;
                inputPtr->cbExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize + luma2bitSize;
                inputPtr->crExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize + luma2bitSize + chroma2bitSize;

                inputPtr->lumaExt = inputPtr->lumaExt + config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING;
                inputPtr->cbExt = inputPtr->cbExt + (config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);
                inputPtr->crExt = inputPtr->crExt + (config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);

            }

            headerPtr->nFilledLen = (EB_U32)frameSize;
        }
        else
        {
            const int tenBitPackedMode = (config->encoderBitDepth > 8) && (config->compressedTenBitFormat == 0) ? 1 : 0;

            // Determine size of each plane
            const size_t luma8bitSize =
                (config->inputPaddedWidth) *
                (config->inputPaddedHeight) *
                (1 << tenBitPackedMode);

            const size_t chroma8bitSize = luma8bitSize >> 2;

            const size_t luma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? luma8bitSize : 0;
            const size_t chroma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? chroma8bitSize : 0;

            EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;

            inputPtr->yStride = config->inputPaddedWidth;
            inputPtr->crStride = config->inputPaddedWidth >> 1;
            inputPtr->cbStride = config->inputPaddedWidth >> 1;

            inputPtr->luma = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput];
            inputPtr->cb = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize;
            inputPtr->cr = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + chroma8bitSize;
            inputPtr->luma = inputPtr->luma + ((config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING) << tenBitPackedMode);
            inputPtr->cb = inputPtr->cb + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)) << tenBitPackedMode);
            inputPtr->cr = inputPtr->cr + (((config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1)) << tenBitPackedMode);


            if (is16bit) {
                inputPtr->lumaExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize;
                inputPtr->cbExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize + luma10bitSize;
                inputPtr->crExt = config->sequenceBuffer[config->processedFrameCount % config->bufferedInput] + luma8bitSize + 2 * chroma8bitSize + luma10bitSize + chroma10bitSize;
                inputPtr->lumaExt = inputPtr->lumaExt + config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING;
                inputPtr->cbExt = inputPtr->cbExt + (config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);
                inputPtr->crExt = inputPtr->crExt + (config->inputPaddedWidth >> 1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);

            }

            headerPtr->nFilledLen = (EB_U32)(EB_U64)SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, is16bit);

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
        EB_U32           qpPtr;
        EB_S32           tmpQp = 0;

        do {
            // get next qp
            tmpQp = GetNextQpFromQpFile(config);

            if (tmpQp == (EB_S32)EB_ErrorInsufficientResources) {
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
            printf("\nWarning: QP File did not contain any valid QPs");
        }

        qpPtr = CLIP3(0, 51, tmpQp);

        headerPtr->qpValue = qpPtr;
    }
    return;
}

//************************************/
// ProcessInputBuffer
// Reads yuv frames from file and copy
// them into the input buffer
/************************************/
APPEXITCONDITIONTYPE ProcessInputBuffer(
    EbConfig_t             *config,
    EbAppContext_t         *appCallBack)
{
    unsigned char            is16bit = (unsigned char)(config->encoderBitDepth > 8);
    EB_BUFFERHEADERTYPE     *headerPtr = appCallBack->inputBufferPool[0]; // needs to change for buffered input
    EB_COMPONENTTYPE        *componentHandle = (EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle;
    
    APPEXITCONDITIONTYPE    return_value = APP_ExitConditionNone;

    EB_S64                  inputPaddedWidth           = config->inputPaddedWidth;
    EB_S64                  inputPaddedHeight          = config->inputPaddedHeight; 
    EB_S64                  framesToBeEncoded          = config->framesToBeEncoded;
	EB_U64                  frameSize                  = (EB_U64)((inputPaddedWidth*inputPaddedHeight * 3) / 2 + (inputPaddedWidth / 4 * inputPaddedHeight * 3) / 2);
    EB_S64                  totalBytesToProcessCount;
    EB_S64                  remainingByteCount;

    if (config->injector && config->processedFrameCount)
    {
        EbInjector(config->processedFrameCount, config->injectorFrameRate);
    }

	totalBytesToProcessCount = (framesToBeEncoded < 0) ? -1 : (config->encoderBitDepth == 10 && config->compressedTenBitFormat == 1) ?
		framesToBeEncoded * (EB_S64)frameSize : 
        framesToBeEncoded * SIZE_OF_ONE_FRAME_IN_BYTES(inputPaddedWidth, inputPaddedHeight, is16bit);


    remainingByteCount       = (totalBytesToProcessCount < 0) ?   -1 :  totalBytesToProcessCount - (EB_S64)config->processedByteCount;

    // If there are bytes left to encode, configure the header
    if (remainingByteCount != 0 && config->stopEncoder == EB_FALSE) {
        ReadInputFrames(
            config,
            is16bit,
            headerPtr);

        // Update the context parameters
        config->processedByteCount += headerPtr->nFilledLen;
        headerPtr->pAppPrivate          = (EB_PTR)EB_NULL;
        config->framesEncoded           = (EB_S32)(++config->processedFrameCount);

        // Configuration parameters changed on the fly
        if (config->useQpFile && config->qpFile)
            SendQpOnTheFly(
                config,
                headerPtr);        

        if (keepRunning == 0 && !config->stopEncoder) {
            config->stopEncoder = EB_TRUE;
        }

        // Fill in Buffers Header control data
        headerPtr->nOffset      = 0;
        headerPtr->pts          = config->processedFrameCount-1;
        headerPtr->sliceType    = INVALID_SLICE;

#if CHKN_EOS
        headerPtr->nFlags = 0; 
#else
        headerPtr->nFlags |= (contextPtr->processedFrameCount == (EB_U64)config->framesToBeEncoded) || config->stopEncoder ? EB_BUFFERFLAG_EOS : 0;
#endif

        // Send the picture
        EbH265EncSendPicture(componentHandle, headerPtr);
        
        if ((config->processedFrameCount == (EB_U64)config->framesToBeEncoded) || config->stopEncoder) {

            headerPtr->nAllocLen    = 0;
            headerPtr->nFilledLen   = 0;
            headerPtr->nTickCount   = 0;
            headerPtr->pAppPrivate  = NULL;
            headerPtr->nOffset      = 0;
            headerPtr->nFlags       = EB_BUFFERFLAG_EOS;
            headerPtr->pBuffer      = NULL;
            headerPtr->sliceType    = INVALID_SLICE;

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
    unsigned char           picSendDone)
{
    APPPORTACTIVETYPE       *portState = &appCallBack->outputStreamPortActive;
    EB_BUFFERHEADERTYPE     *headerPtr = appCallBack->streamBufferPool[0]; // needs to change for buffered input
    EB_COMPONENTTYPE        *componentHandle = (EB_COMPONENTTYPE*)appCallBack->svtEncoderHandle;
    APPEXITCONDITIONTYPE    return_value = APP_ExitConditionNone;
    EB_ERRORTYPE            stream_status = EB_ErrorNone;
    // Per channel variables
    FILE                 *streamFile      = config->bitstreamFile;
    EB_U32               startFrame      = (config->framesToBeEncoded == 0 || config->framesToBeEncoded < LONG_ENCODE_FRAME_ENCODE || (SPEED_MEASUREMENT_INTERVAL < START_STEADY_STATE)) ? 0 : START_STEADY_STATE;
	EB_U64              *startsTime      = &config->performanceContext.startsTime ; 
    EB_U64              *startuTime      = &config->performanceContext.startuTime ;
                         
    EB_U64              *totalLatency    = &config->performanceContext.totalLatency;
    EB_U32              *maxLatency      = &config->performanceContext.maxLatency;
    
    // System performance variables
    static EB_U64        allChannelsStartsTime     = 0;
    static EB_U64        allChannelsStartuTime     = 0;
    static int           frameCount                = 0;

    // for a long encode, ignore startup time to get the steady state speed
    static EB_U32        allChannelsStartFrame     = START_STEADY_STATE;
    
    // Local variables
    EB_U64                finishsTime     = 0;
    EB_U64                finishuTime     = 0;
    double                duration        = 0.0;

    // non-blocking call until all input frames are sent
    stream_status = EbH265GetPacket(componentHandle, headerPtr, picSendDone);

    if (stream_status == EB_ErrorMax) {
        printf("\n");
        LogErrorOutput(
            config->errorLogFile,
            headerPtr->nFlags);
        return APP_ExitConditionError;
    }
    else if (stream_status != EB_NoErrorEmptyQueue) {
        ++(config->performanceContext.frameCount);
        *totalLatency += (EB_U64)headerPtr->nTickCount;
        *maxLatency = (headerPtr->nTickCount > *maxLatency) ? headerPtr->nTickCount : *maxLatency;

        // Reset counters for long encodes
        if (config->performanceContext.frameCount - 1 == startFrame) {
            StartTime((unsigned long long*)startsTime, (unsigned long long*)startuTime);
            *maxLatency = 0;
            *totalLatency = 0;
        }
        if ((EB_U32)frameCount == allChannelsStartFrame && allChannelsStartFrame <= SPEED_MEASUREMENT_INTERVAL) {
            StartTime((unsigned long long*)&allChannelsStartsTime, (unsigned long long*)&allChannelsStartuTime);
        }

        FinishTime((unsigned long long*)&finishsTime, (unsigned long long*)&finishuTime);

        ComputeOverallElapsedTime(
            *startsTime,
            *startuTime,
            finishsTime,
            finishuTime,
            &duration);

        // Write Stream Data to file
        if (streamFile) {
            fwrite(headerPtr->pBuffer + headerPtr->nOffset, 1, headerPtr->nFilledLen, streamFile);
        }
        config->performanceContext.byteCount += headerPtr->nFilledLen;

        // Update Output Port Activity State
        *portState = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? APP_PortInactive : *portState;
        return_value = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? APP_ExitConditionFinished : APP_ExitConditionNone;
#if DEADLOCK_DEBUG
        ++frameCount;
#else
        //++frameCount;
        printf("\b\b\b\b\b\b\b\b\b%9d", ++frameCount);
#endif

        //++frameCount;
        fflush(stdout);

        // Queue the buffer again if the port is still active
        if (*portState == APP_PortActive) {
#if ! CHKN_OMX
            EbH265EncFillPacket((EB_HANDLETYPE)componentHandle, headerPtr);
#endif 
        }
        else {
            if ((config->framesToBeEncoded < SPEED_MEASUREMENT_INTERVAL) || (config->framesToBeEncoded - startFrame) < SPEED_MEASUREMENT_INTERVAL) {
                config->performanceContext.averageSpeed = (config->performanceContext.frameCount - startFrame) / duration;
                config->performanceContext.averageLatency = config->performanceContext.totalLatency / (double)(config->performanceContext.frameCount - startFrame);
            }
        }


        if ((((config->framesToBeEncoded != 0) && (config->framesToBeEncoded > SPEED_MEASUREMENT_INTERVAL))
            && ((EB_S64)config->performanceContext.frameCount == (config->framesToBeEncoded - (EB_S32)startFrame)))) {
            config->performanceContext.averageSpeed = (config->performanceContext.frameCount - startFrame) / duration;
            config->performanceContext.averageLatency = config->performanceContext.totalLatency / (double)(config->performanceContext.frameCount - startFrame);
        }

        if (!(frameCount % SPEED_MEASUREMENT_INTERVAL)) {
            if (frameCount < (EB_S32)allChannelsStartFrame && (frameCount >= (EB_S32)LONG_ENCODE_FRAME_ENCODE) && (allChannelsStartFrame <= SPEED_MEASUREMENT_INTERVAL)) {
                ComputeOverallElapsedTime(
                    allChannelsStartsTime,
                    allChannelsStartuTime,
                    finishsTime,
                    finishuTime,
                    &duration);

                printf("\n");
                printf("Average System Encoding Speed:        %.2f\n", (double)(frameCount - allChannelsStartFrame) / duration);
            }
            else {
                ComputeOverallElapsedTime(
                    *startsTime,
                    *startuTime,
                    finishsTime,
                    finishuTime,
                    &duration);

                printf("\n");
                printf("Average System Encoding Speed:        %.2f\n", (double)(frameCount - (EB_S32)startFrame) / duration);
            }
        }
    }
	return return_value;
}

