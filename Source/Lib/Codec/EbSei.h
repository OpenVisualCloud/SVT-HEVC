/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EBSEI_h
#define EBSEI_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

    // These are temporary definitions, should be changed in the future according to user's requirements
#define MAX_CPB_COUNT    4
#define  MAX_TEMPORAL_LAYERS                        6
#define  MAX_DECODING_UNIT_COUNT                    64   // picture timing SEI


    // User defined structures for passing data from application to the library should be added here
    typedef struct UnregistedUserData_s {

        EB_U8   uuidIsoIec_11578[16];
        EB_U8   *userData;
        EB_U32  userDataSize;

    } UnregistedUserData_t;

    typedef struct RegistedUserData_s {

        EB_U8   *userData;    // First byte is itu_t_t35_country_code. 
                              // If itu_t_t35_country_code  ==  0xFF, second byte is itu_t_t35_country_code_extension_byte.
                              // the rest are the payloadByte
        EB_U32   userDataSize;

    } RegistedUserData_t;

    // SEI structures
    typedef struct AppHrdParameters_s {

        EB_BOOL                         nalHrdParametersPresentFlag;
        EB_BOOL                         vclHrdParametersPresentFlag;
        EB_BOOL                         subPicCpbParamsPresentFlag;

        EB_U32                          tickDivisorMinus2;
        EB_U32                          duCpbRemovalDelayLengthMinus1;

        EB_BOOL                         subPicCpbParamsPicTimingSeiFlag;

        EB_U32                          dpbOutputDelayDuLengthMinus1;

        EB_U32                          bitRateScale;
        EB_U32                          cpbSizeScale;
        EB_U32                          duCpbSizeScale;

        EB_U32                          initialCpbRemovalDelayLengthMinus1;
        EB_U32                          auCpbRemovalDelayLengthMinus1;
        EB_U32                          dpbOutputDelayLengthMinus1;

        EB_BOOL                         fixedPicRateGeneralFlag[MAX_TEMPORAL_LAYERS];
        EB_BOOL                         fixedPicRateWithinCvsFlag[MAX_TEMPORAL_LAYERS];

        EB_U32                          elementalDurationTcMinus1[MAX_TEMPORAL_LAYERS];

        EB_BOOL                         lowDelayHrdFlag[MAX_TEMPORAL_LAYERS];

        EB_U32                          cpbCountMinus1[MAX_TEMPORAL_LAYERS];

        EB_U32                          bitRateValueMinus1[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];
        EB_U32                          cpbSizeValueMinus1[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];
        EB_U32                          bitRateDuValueMinus1[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];
        EB_U32                          cpbSizeDuValueMinus1[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];

        EB_BOOL                         cbrFlag[MAX_TEMPORAL_LAYERS][2][MAX_CPB_COUNT];

        EB_BOOL                         cpbDpbDelaysPresentFlag;

    } AppHrdParameters_t;

    typedef struct AppVideoUsabilityInfo_s {

        EB_BOOL                 aspectRatioInfoPresentFlag;
        EB_U32                  aspectRatioIdc;
        EB_U32                  sarWidth;
        EB_U32                  sarHeight;

        EB_BOOL                 overscanInfoPresentFlag;
        EB_BOOL                 overscanApproriateFlag;
        EB_BOOL                 videoSignalTypePresentFlag;

        EB_U32                  videoFormat;
        EB_BOOL                 videoFullRangeFlag;
        EB_BOOL                 colorDescriptionPresentFlag;

        EB_U32                  colorPrimaries;
        EB_U32                  transferCharacteristics;
        EB_U32                  matrixCoeffs;

        EB_BOOL                 chromaLocInfoPresentFlag;
        EB_U32                  chromaSampleLocTypeTopField;
        EB_U32                  chromaSampleLocTypeBottomField;


        EB_BOOL                 neutralChromaIndicationFlag;
        EB_BOOL                 fieldSeqFlag;
        EB_BOOL                 frameFieldInfoPresentFlag;

        EB_BOOL                 defaultDisplayWindowFlag;
        EB_U32                  defaultDisplayWinLeftOffset;
        EB_U32                  defaultDisplayWinRightOffset;
        EB_U32                  defaultDisplayWinTopOffset;
        EB_U32                  defaultDisplayWinBottomOffset;

        EB_BOOL                 vuiTimingInfoPresentFlag;
        EB_U32                  vuiNumUnitsInTick;
        EB_U32                  vuiTimeScale;

        EB_BOOL                 vuiPocPropotionalTimingFlag;
        EB_U32                  vuiNumTicksPocDiffOneMinus1;

        EB_BOOL                 vuiHrdParametersPresentFlag;

        EB_BOOL                 bitstreamRestrictionFlag;

        EB_BOOL                 motionVectorsOverPicBoundariesFlag;
        EB_BOOL                 restrictedRefPicListsFlag;

        EB_U32                  minSpatialSegmentationIdc;
        EB_U32                  maxBytesPerPicDenom;
        EB_U32                  maxBitsPerMinCuDenom;
        EB_U32                  log2MaxMvLengthHorizontal;
        EB_U32                  log2MaxMvLengthVertical;

        AppHrdParameters_t     *hrdParametersPtr;

    } AppVideoUsabilityInfo_t;


    typedef struct AppPictureTimingSei_s {

        EB_U32   picStruct;
        EB_U32   sourceScanType;
        EB_BOOL  duplicateFlag;
        EB_U32   auCpbRemovalDelayMinus1;
        EB_U32   picDpbOutputDelay;
        EB_U32   picDpbOutputDuDelay;
        EB_U32   numDecodingUnitsMinus1;
        EB_BOOL  duCommonCpbRemovalDelayFlag;
        EB_U32   duCommonCpbRemovalDelayMinus1;
        EB_U32   numNalusInDuMinus1;
        EB_U32   duCpbRemovalDelayMinus1[MAX_DECODING_UNIT_COUNT];

    } AppPictureTimingSei_t;

    typedef struct AppBufferingPeriodSei_s {

        EB_U32  bpSeqParameterSetId;
        EB_BOOL rapCpbParamsPresentFlag;
        EB_BOOL concatenationFlag;
        EB_U32  auCpbRemovalDelayDeltaMinus1;
        EB_U32  cpbDelayOffset;
        EB_U32  dpbDelayOffset;
        EB_U32  initialCpbRemovalDelay[2][MAX_CPB_COUNT];
        EB_U32  initialCpbRemovalDelayOffset[2][MAX_CPB_COUNT];
        EB_U32  initialAltCpbRemovalDelay[2][MAX_CPB_COUNT];
        EB_U32  initialAltCpbRemovalDelayOffset[2][MAX_CPB_COUNT];

    } AppBufferingPeriodSei_t;

    typedef struct AppActiveParameterSetsSei_s {
        EB_U32 activeVideoParameterSetid;
        EB_BOOL selfContainedCvsFlag;
        EB_BOOL noParameterSetUpdateFlag;
        EB_U32 numSpsIdsMinus1;
        EB_U32 activeSeqParameterSetId;

    } AppActiveparameterSetSei_t;

    typedef struct AppRecoveryPoint_s {

        EB_U32  recoveryPocCnt;
        EB_BOOL exactMatchingFlag;
        EB_BOOL brokenLinkFlag;

    } AppRecoveryPoint_t;

    // Below is an example of PanScanRectangle SEI data structure 
    // Other SEI messages can have data structure in this format  
    typedef struct AppPanScanRectangleSei_s {

        EB_U32      panScanRectId;
        EB_BOOL     panScanRectCancelFlag;

        EB_U32      panScanCountMinus1;
        EB_U32      panScanRectLeftOffset[3];
        EB_U32      panScanRectRightOffset[3];
        EB_U32      panScanRectTopOffset[3];
        EB_U32      panScanRectBottomOffset[3];

        EB_BOOL     panScanRectPersistFlag;

    }AppPanScanRectangleSei_t;

    //

    typedef struct AppContentLightLevelSei_s {

        EB_U16 maxContentLightLevel;
        EB_U16 maxPicAverageLightLevel;

    }AppContentLightLevelSei_t;

    typedef struct AppMasteringDisplayColorVolumeSei_s {

        EB_U16 displayPrimaryX[3];
        EB_U16 displayPrimaryY[3];
        EB_U16 whitePointX, whitePointY;
        EB_U32 maxDisplayMasteringLuminance;
        EB_U32 minDisplayMasteringLuminance;

    }AppMasteringDisplayColorVolumeSei_t;


    typedef struct EB_FRAME_RATE_CFG
    {
        EB_U32 num;
        EB_U32 den;
    } EB_FRAME_RATE_CFG;

    typedef struct EB_LATENCY_CALC
    {
        EB_U64	startTimesSeconds;
        EB_U64	finishTimesSeconds;
        EB_U64	startTimesuSeconds;
        EB_U64	finishTimesuSeconds;
        EB_U64  poc;
    } EB_LATENCY_CALC;


    typedef struct EB_CU_QP_CFG_s
    {
        EB_U32      qpArrayCount;
        EB_S8      *qpArray;

        EB_U32      cuStride;
        EB_U32      lcuCount;
        EB_U32      lcuStride;

        EB_U32      numCUsPerLastRowLCU;
        EB_U32      numCUsPerTypicalLCU;


    } EB_CU_QP_CFG_t;



    // Signals that the default prediction structure and controls are to be 
    //   overwritten and manually controlled. Manual control should be active
    //   for an entire encode, from beginning to termination.  Mixing of default
    //   prediction structure control and override prediction structure control
    //   is not supported.
    //
    // The refList0Count and refList1Count variables control the picture/slice type.
    //   I_SLICE: refList0Count == 0 && refList1Count == 0
    //   P_SLICE: refList0Count  > 0 && refList1Count == 0
    //   B_SLICE: refList0Count  > 0 && refList1Count  > 0

    typedef struct EB_PRED_STRUCTURE_CFG
    {
        EB_U64              pictureNumber;      // Corresponds to the display order of the picture
        EB_U64              decodeOrderNumber;  // Corresponds to the decode order of the picture
        EB_U32              temporalLayerIndex; // Corresponds to the temporal layer index of the picture
        EB_U32              nalUnitType;        // Pictures NAL Unit Type
        EB_U32              refList0Count;      // A count of zero indicates the list is inactive
        EB_U32              refList1Count;      // A count of zero indicates the list is inactive
        EB_BOOL             isReferenced;       // Indicates whether or not the picture is used as
                                                //   future reference.           
        EB_U32              futureReferenceCount;
        EB_S32             *futureReferenceList;// Contains a list of delta POCs whose references shall
                                                //   be saved for future reference.  This signalling must
                                                //   be done with respect to decode picture order. Must 
                                                //   be conformant with the DPB rules.
    } EB_PRED_STRUCTURE_CFG;

    // EB_PICTURE_PLANE defines the data formatting of a singple plane of picture data.
    typedef struct EB_PICTURE_PLANE
    {
        // "start" is the starting position of the first 
        //   valid pixel in the picture plane.
        EB_U8* start;

        // "stride" is the number of bytes required to increment
        //   an address by one line without changing the horizontal
        //   positioning.
        EB_U32 stride;

    } EB_PICTURE_PLANE;

    typedef struct EB_EOS_DATA_DEF
    {
        EB_BOOL             dataValid;          // Indicates if the data attached with the last frame in the bitstream is valid or not.
                                                //   If the last frame is valid, the data will be included in the bistream
                                                //   If the last frame is NOT valid, the frame will be coded as IDR in the encoder, but
                                                //   not included in the bitstream.

    } EB_EOS_DATA_DEF;


EB_ERRORTYPE EbVideoUsabilityInfoCtor(
    AppVideoUsabilityInfo_t *vuiPtr);



void EbVideoUsabilityInfoCopy(
    AppVideoUsabilityInfo_t *dstVuiPtr,
    AppVideoUsabilityInfo_t *srcVuiPtr);

extern void EbPictureTimeingSeiCtor(
    AppPictureTimingSei_t   *picTimingPtr);

extern void EbBufferingPeriodSeiCtor(
    AppBufferingPeriodSei_t   *bufferingPeriodPtr);

extern void EbActiveParameterSetSeiCtor(
    AppActiveparameterSetSei_t    *activeParameterPtr);

extern void EbRecoveryPointSeiCtor(
    AppRecoveryPoint_t   *recoveryPointSeiPtr);

extern void EbContentLightLevelCtor(
    AppContentLightLevelSei_t    *contentLightLevelPtr);

extern void EbMasteringDisplayColorVolumeCtor(
    AppMasteringDisplayColorVolumeSei_t    *masteringDisplayPtr);

extern void EbRegUserDataSEICtor(
    RegistedUserData_t    *regUserDataSeiPtr);

extern void EbUnRegUserDataSEICtor(
    UnregistedUserData_t    *UnRegUserDataPtr);

extern EB_U32 GetPictureTimingSEILength(
    AppPictureTimingSei_t      *picTimingSeiPtr,
    AppVideoUsabilityInfo_t    *vuiPtr);

extern EB_U32 GetBufPeriodSEILength(
    AppBufferingPeriodSei_t    *bufferingPeriodPtr,
    AppVideoUsabilityInfo_t    *vuiPtr);

extern EB_U32 GetActiveParameterSetSEILength(
	AppActiveparameterSetSei_t    *activeParameterSet);

extern EB_U32 GetRecoveryPointSEILength(
    AppRecoveryPoint_t    *recoveryPointSeiPtr);

extern EB_U32 GetContentLightLevelSEILength();

extern EB_U32 GetMasteringDisplayColorVolumeSEILength();
#ifdef __cplusplus
}
#endif
#endif // EBSEI_h
