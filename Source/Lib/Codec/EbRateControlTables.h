/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbRateControlTables_h
#define EbRateControlTables_h

#include "EbDefinitions.h"
#include "EbUtility.h"
#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Rate Control Defines
 **************************************/
#define SAD_PRECISION_INTERVAL                              4

#define VAR_ROUND_INTERVAL                                  20
#define NUMBER_OF_SAD_INTERVALS                             128      // number of intervals in SAD tables

#define NUMBER_OF_INTRA_SAD_INTERVALS                       NUMBER_OF_SAD_INTERVALS      // number of intervals in intra Sad tables

#define TOTAL_NUMBER_OF_INTERVALS                           (NUMBER_OF_SAD_INTERVALS + \
                                                             NUMBER_OF_INTRA_SAD_INTERVALS )

#define TOTAL_NUMBER_OF_REF_QP_VALUES                       1      

#define TOTAL_NUMBER_OF_INITIAL_RC_TABLES_ENTRY             (TOTAL_NUMBER_OF_REF_QP_VALUES * TOTAL_NUMBER_OF_INTERVALS)

/**************************************
 * The EB_BitFraction is used to define the bit fraction numbers
 **************************************/
typedef EB_U16 EB_Bit_Number;

/**************************************
 * Initial Rate Control Structure
 **************************************/
typedef struct InitialRateControlTables_s {
    EB_Bit_Number  sadBitsArray      [MAX_TEMPORAL_LAYERS][NUMBER_OF_SAD_INTERVALS];
    EB_Bit_Number  intraSadBitsArray [MAX_TEMPORAL_LAYERS][NUMBER_OF_INTRA_SAD_INTERVALS];


} RateControlTables_t;
static const EB_U8 refQpListTable [] = 

    { 0,    1,    2,    3,    4,    5,    6,    7, 
      8,    9,   10,   11,   12,   13,   14,   15,   
     16,   17,   18,   19,   20,   21,   22,   23,
     24,   25,   26,   27,   28,   29,   30,   31,
     32,   33,   34,   35,   36,   37,   38,   39,
     40,   41,   42,   43,   44,   45,   46,   47,
     48,   49,   50,   51};

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE RateControlTablesCtor(
    RateControlTables_t *rateControlTablesArray
);

#ifdef __cplusplus
}
#endif
#endif //EbRateControlTables_h
