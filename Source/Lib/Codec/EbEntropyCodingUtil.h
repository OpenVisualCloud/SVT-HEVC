/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCodingUtil_h
#define EbEntropyCodingUtil_h

#include "EbDefinitions.h"
#include "EbEntropyCodingObject.h"
#include "EbCodingUnit.h"
#include "EbPredictionUnit.h"
#include "EbPictureBufferDesc.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"
#include "EbCabacContextModel.h"
#include "EbModeDecision.h"
#include "EbIntraPrediction.h"
#include "EbBitstreamUnit.h"
#include "EbPacketizationProcess.h"
//#include "EbHmCode.h"
/**************************************
* Defines
**************************************/
#ifdef __cplusplus
extern "C" {
#endif
#define SIZE_OF_SCAN_SET                                              16
#define LOG2_SCAN_SET_SIZE                                             4
#define NUMBER_OF_INTRA_MODES                                         36
#define NUMBER_OF_INTRA_MODES_MINUS_TWO                               34
#define RICE_UPDATE_LENGTH                                            24 //16
#define GREATER_THAN1_MAX_NUMBER                                       8 // maximum number for greater than 1 
#define GREATER_THAN2_MAX_NUMBER                                       1 // maximum number for greater than 2 
#define COEF_REMAIN_BIN_REDUCTION                                      3
#define CU_DELTA_QP_CMAX                                               5   
#define CU_DELTA_QP_EGK                                                0

// These are redefined here to avoid conflict with the existing scan indices
// The "old" ones should be removed as there is no zigzag scan in H.265
enum COEFF_SCAN_TYPE2
{
    SCAN_DIAG2 = 0,      // diagonal scan
    SCAN_HOR2,           // first scan is horizontal
    SCAN_VER2            // first scan is vertical
};

// texture component type
enum COMPONENT_TYPE
{
    COMPONENT_LUMA = 0,            // luma
    COMPONENT_CHROMA = 1,            // chroma (Cb+Cr)
    COMPONENT_CHROMA_CB = 2,            // chroma Cb
    COMPONENT_CHROMA_CR = 3,            // chroma Cr
    COMPONENT_CHROMA_CB2 = 4,   // chroma 2nd CB for 422
    COMPONENT_CHROMA_CR2 = 5,   // chroma 2nd CR for 422
    COMPONENT_ALL = 6,          // Y+Cb+Cr
    COMPONENT_NONE = 15
};

/**************************************
* Static Arrays
**************************************/

// 8x8 subblock scan
// low nibble is horizontal position
// high nibble is vertical position
static const EB_U8 sbScan8[8 * 8 / 16] =
{
    0x00,
    0x10, 0x01,
    0x11
};

// 16x16 subblock scan
// low nibble is horizontal position
// high nibble is vertical position
static const EB_U8 sbScan16[16 * 16 / 16] =
{
    0x00,
    0x10, 0x01,
    0x20, 0x11, 0x02,
    0x30, 0x21, 0x12, 0x03,
    0x31, 0x22, 0x13,
    0x32, 0x23,
    0x33
};

// 32x32 subblock scan
// low nibble is horizontal position
// high nibble is vertical position
static const EB_U8 sbScan32[32 * 32 / 16] =
{
    0x00,
    0x10, 0x01,
    0x20, 0x11, 0x02,
    0x30, 0x21, 0x12, 0x03,
    0x40, 0x31, 0x22, 0x13, 0x04,
    0x50, 0x41, 0x32, 0x23, 0x14, 0x05,
    0x60, 0x51, 0x42, 0x33, 0x24, 0x15, 0x06,
    0x70, 0x61, 0x52, 0x43, 0x34, 0x25, 0x16, 0x07,
    0x71, 0x62, 0x53, 0x44, 0x35, 0x26, 0x17,
    0x72, 0x63, 0x54, 0x45, 0x36, 0x27,
    0x73, 0x64, 0x55, 0x46, 0x37,
    0x74, 0x65, 0x56, 0x47,
    0x75, 0x66, 0x57,
    0x76, 0x67,
    0x77
};

// Table of significance map context indices (4x4 TU)
// 3 orderings correponding to 3 scan patterns
static const EB_U8 contextIndexMap4[3][16] =
{
    { 0, 2, 1, 6, 3, 4, 7, 6, 4, 5, 7, 8, 5, 8, 8, 8 },
    { 0, 1, 4, 5, 2, 3, 4, 5, 6, 6, 8, 8, 7, 7, 8, 8 },
    { 0, 2, 6, 7, 1, 3, 6, 7, 4, 4, 8, 8, 5, 5, 8, 8 }
};


// Table of significance map context indices (> 8x8 TU)
// 2 orderings correponding to 2 scan patterns: diagonal and horizontal (vertical also uses horizontal)
static const EB_U8 contextIndexMap8[2][4][16] =
{
    {
        { 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 2, 1, 2, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 0, 0, 0 },
        { 2, 2, 1, 2, 1, 0, 2, 1, 0, 0, 1, 0, 0, 0, 0, 0 },
        { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }
    },
    {
        { 2, 1, 1, 0,
        1, 1, 0, 0,
        1, 0, 0, 0,
        0, 0, 0, 0
        },
        { 2, 1, 0, 0,
        2, 1, 0, 0,
        2, 1, 0, 0,
        2, 1, 0, 0
        },
        { 2, 2, 2, 2,
        1, 1, 1, 1,
        0, 0, 0, 0,
        0, 0, 0, 0
        },
        { 2, 2, 2, 2,
        2, 2, 2, 2,
        2, 2, 2, 2,
        2, 2, 2, 2
        }
    }
};

// 4x4 subblock scans (diagonal and horizontal)
// Used to determine last X/Y position
static const EB_U8 scans4[2][16] =
{
    { 0, 4, 1, 8, 5, 2, 12, 9, 6, 3, 13, 10, 7, 14, 11, 15 },
    { 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 }
};

static const EB_U32 lastSigXYGroupIndex[] =
{
    0, 1, 2, 3, 4, 4, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9
};

static const EB_U32 minInSigXYGroup[] =
{
    0, 1, 2, 3, 4, 6, 8, 12, 16, 24
};

/**************************************
* Data Structures
**************************************/
typedef struct BacEncContext_s {
    OutputBitstreamUnit_t   *m_pcTComBitIf;
    EB_U32          intervalLowValue;            //32 bits
    EB_U32          intervalRangeValue;            //10 bits
    EB_U32          tempBufferedByte;
    EB_S32          tempBufferedBytesNum;
    EB_S32          bitsRemainingNum;
} BacEncContext_t;

typedef struct CabacEncodeContext_s {
    BacEncContext_t            bacEncContext;
    ContextModelEncContext_t   contextModelEncContext;
    EB_COLOR_FORMAT colorFormat;
} CabacEncodeContext_t;

/**************************************
* Static Functions
**************************************/
void EncodeOneBin(
    BacEncContext_t *bacEncContextPtr,
    const EB_U32     BinaryValue,
    EB_ContextModel *contextModelPtr);

void EncodeBypassOneBin(
    BacEncContext_t *bacEncContextPtr,
    const EB_U32     BinaryValue);

void EncodeBypassBins(
    BacEncContext_t *bacEncContextPtr,
    const EB_U32     BinaryValues,
    EB_U32           binsLength);

void BacEncContextTerminate(
    BacEncContext_t *bacEncContextPtr,
    const EB_U32     BinaryValue);

void RemainingCoeffExponentialGolombCode(
    CabacEncodeContext_t *cabacEncodeCtxPtr,
    EB_U32 symbolValue,
    EB_U32 golombParam);

EB_U32 EstimateRemainingCoeffExponentialGolombCode(
    EB_U32 symbolValue,
    EB_U32 golombParam);

void EncodeLastSignificantXY(
    CabacEncodeContext_t   *cabacEncodeCtxPtr,
    EB_U32                  lastSigXPos,
    EB_U32                  lastSigYPos,
    const EB_U32            size,
    const EB_U32            logBlockSize,
    const EB_U32            isChroma);

EB_U32 EstimateLastSignificantXY(
    CabacEncodeContext_t   *cabacEncodeCtxPtr,
    EB_U32                  lastSigXPos,
    EB_U32                  lastSigYPos,
    const EB_U32            size,
    const EB_U32            logBlockSize,
    const EB_U32            isChroma);

void EstimateOneBin(
    const EB_U32     BinaryValue,
    EB_ContextModel *contextModelPtr);

EB_ERRORTYPE EstimateMergeIndex(
    CodingUnit_t        *cuPtr,
    PredictionUnit_t    *puPtr,
    EB_ContextModel     *mergeIndexContextModel_p);

void EstimateSkipFlag(
    EB_BOOL                 skipFlag,
    CodingUnit_t           *cuPtr,
    EB_ContextModel        *skipFlagContextModel_p
);

EB_ERRORTYPE EstimateMergeFlag(
    CodingUnit_t           *cuPtr,
    PredictionUnit_t       *puPtr,
    EB_ContextModel        *mergeFlagContextModel_p);

EB_ERRORTYPE EstimateMvpIndex(
    CodingUnit_t        *cuPtr,
    PredictionUnit_t    *puPtr,
    EB_REFLIST           refList);

EB_ERRORTYPE EstimatePartitionSize(
	CodingUnit_t        *cuPtr);

EB_ERRORTYPE EstimatePredictionMode(
    CodingUnit_t        *cuPtr);

void EstimateIntraLumaMode(
    CodingUnit_t           *cuPtr,
    PredictionUnit_t       *puPtr,
    EB_U32                  intraLumaMode);

EB_ERRORTYPE EstimateIntraChromaMode(
    CodingUnit_t           *cuPtr);

EB_ERRORTYPE EstimateMvd(
    CodingUnit_t           *cuPtr,
    PredictionUnit_t       *puPtr,
    EB_REFLIST              refList);

EB_ERRORTYPE EstimatePredictionDirection(
    PredictionUnit_t    *puPtr,
    CodingUnit_t        *cuPtr);

EB_ERRORTYPE EstimateReferencePictureIndex(
    CodingUnit_t           *cuPtr,
    EB_REFLIST              refList,
    PictureControlSet_t    *pictureControlSetPtr);

EB_ERRORTYPE EstimateTuCoeff(
    CodingUnit_t            *cuPtr,
    const CodedUnitStats_t  *cuStatsPtr);

EB_ERRORTYPE EstimateIntra4x4EncodeCoeff(
    CodingUnit_t           *cuPtr);

EB_ERRORTYPE EstimateTuSplitCoeff(
    CodingUnit_t            *cuPtr,
    const CodedUnitStats_t  *cuStatsPtr);

void EstimateSplitFlag(
    EB_U32                  cuDepth,
    EB_U32                  maxCuDepth,
    EB_BOOL                 splitFlag,
    CodingUnit_t           *cuPtr,
    EB_ContextModel        *splitFlagContextModel_p);

#ifdef __cplusplus
}
#endif
#endif //EbEntropyCodingUtil_h