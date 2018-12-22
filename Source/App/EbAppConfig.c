/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EbAppConfig.h"
#include "EbApi.h"

#ifdef _WIN32
#else
#include <unistd.h>
#endif

/**********************************
 * Defines
 **********************************/
#define HELP_TOKEN                      "-help"
#define CHANNEL_NUMBER_TOKEN            "-nch"
#define COMMAND_LINE_MAX_SIZE           2048
#define CONFIG_FILE_TOKEN               "-c"
#define INPUT_FILE_TOKEN                "-i"
#define OUTPUT_BITSTREAM_TOKEN          "-b"
#define OUTPUT_RECON_TOKEN              "-o"
#define ERROR_FILE_TOKEN                "-errlog"
#define STAT_FILE_TOKEN                 "-stat-file"
#define QP_FILE_TOKEN                   "-qp-file"
#define WIDTH_TOKEN                     "-w"
#define HEIGHT_TOKEN                    "-h"
#define NUMBER_OF_PICTURES_TOKEN        "-n"
#define BUFFERED_INPUT_TOKEN            "-nb"
#define BASE_LAYER_SWITCH_MODE_TOKEN    "-base-layer-switch-mode" // no Eval
#define QP_TOKEN                        "-q"
#define USE_QP_FILE_TOKEN               "-use-q-file"
#define TUNE_TOKEN                      "-tune"
#define FRAME_RATE_TOKEN                "-fps"
#define FRAME_RATE_NUMERATOR_TOKEN      "-fps-num"
#define FRAME_RATE_DENOMINATOR_TOKEN    "-fps-denom"
#define ENCODER_BIT_DEPTH               "-bit-depth"
#define INPUT_COMPRESSED_TEN_BIT_FORMAT "-compressed-ten-bit-format"
#define ENCMODE_TOKEN                   "-encMode"
#define HIERARCHICAL_LEVELS_TOKEN       "-hierarchical-levels" // no Eval
#define PRED_STRUCT_TOKEN               "-pred-struct"
#define INTRA_PERIOD_TOKEN              "-intra-period"
#define PROFILE_TOKEN                   "-profile"
#define TIER_TOKEN                      "-tier"
#define LEVEL_TOKEN                     "-level" 
#define LATENCY_MODE                    "-latency-mode" // no Eval
#define INTERLACED_VIDEO_TOKEN          "-interlaced-video" 
#define SEPERATE_FILDS_TOKEN            "-separate-fields"
#define INTRA_REFRESH_TYPE_TOKEN        "-irefresh-type" // no Eval
#define LOOP_FILTER_DISABLE_TOKEN       "-dlf"
#define SAO_ENABLE_TOKEN                "-sao"
#define USE_DEFAULT_ME_HME_TOKEN        "-use-default-me-hme"
#define HME_ENABLE_TOKEN                "-hme"      // no Eval
#define HME_L0_ENABLE_TOKEN             "-hme-l0"   // no Eval
#define HME_L1_ENABLE_TOKEN             "-hme-l1"   // no Eval
#define HME_L2_ENABLE_TOKEN             "-hme-l2"   // no Eval
#define SEARCH_AREA_WIDTH_TOKEN         "-search-w" // no Eval
#define SEARCH_AREA_HEIGHT_TOKEN        "-search-h" // no Eval
#define NUM_HME_SEARCH_WIDTH_TOKEN      "-num-hme-w"// no Eval
#define NUM_HME_SEARCH_HEIGHT_TOKEN     "-num-hme-h"// no Eval
#define HME_SRCH_T_L0_WIDTH_TOKEN       "-hme-tot-l0-w"// no Eval
#define HME_SRCH_T_L0_HEIGHT_TOKEN      "-hme-tot-l0-h"// no Eval
#define HME_LEVEL0_WIDTH			    "-hme-l0-w" // no Eval
#define HME_LEVEL0_HEIGHT			    "-hme-l0-h" // no Eval
#define HME_LEVEL1_WIDTH			    "-hme-l1-w" // no Eval
#define HME_LEVEL1_HEIGHT			    "-hme-l1-h" // no Eval
#define HME_LEVEL2_WIDTH                "-hme-l2-w" // no Eval
#define HME_LEVEL2_HEIGHT               "-hme-l2-h" // no Eval
#define CONSTRAINED_INTRA_ENABLE_TOKEN  "-constrd-intra"
#define IMPROVE_SHARPNESS_TOKEN         "-sharp"
#define BITRATE_REDUCTION_TOKEN         "-brr"
#define VIDEO_USE_INFO_TOKEN            "-vid-info"
#define HDR_INPUT_TOKEN                 "-hdr"
#define ACCESS_UNIT_DELM_TOKEN          "-ua-delm"   // no Eval
#define BUFF_PERIOD_TOKEN               "-pbuff"     // no Eval
#define PIC_TIMING_TOKEN                "-tpic"      // no Eval
#define REG_USER_DATA_TOKEN             "-reg-user-data"  // no Eval
#define UNREG_USER_DATA_TOKEN           "-unreg-user-data"  // no Eval
#define RECOVERY_POINT_TOKEN            "-recovery-point" // no Eval
#define RATE_CONTROL_ENABLE_TOKEN       "-rc"
#define TARGET_BIT_RATE_TOKEN           "-tbr"
#define MAX_QP_TOKEN                    "-max-qp"
#define MIN_QP_TOKEN                    "-min-qp"
#define TEMPORAL_ID					    "-temporal-id" // no Eval
#define LOOK_AHEAD_DIST_TOKEN           "-lad"
#define SCENE_CHANGE_DETECTION_TOKEN    "-scd"
#define INJECTOR_TOKEN                  "-inj"  // no Eval
#define INJECTOR_FRAMERATE_TOKEN        "-inj-frm-rt" // no Eval
#define SPEED_CONTROL_TOKEN             "-speed-ctrl"
#define ASM_TYPE_TOKEN				    "-asm" // no Eval
#define RR_THREAD_MGMNT		            "-rr"
#define TARGET_SOCKET		            "-ss"
#define CONFIG_FILE_COMMENT_CHAR        '#'
#define CONFIG_FILE_NEWLINE_CHAR        '\n'
#define CONFIG_FILE_RETURN_CHAR         '\r'
#define CONFIG_FILE_VALUE_SPLIT         ':'
#define CONFIG_FILE_SPACE_CHAR          ' '
#define CONFIG_FILE_ARRAY_SEP_CHAR      CONFIG_FILE_SPACE_CHAR
#define CONFIG_FILE_TAB_CHAR            '\t'
#define CONFIG_FILE_NULL_CHAR           '\0'
#define CONFIG_FILE_MAX_ARG_COUNT       256
#define CONFIG_FILE_MAX_VAR_LEN         128
#define EVENT_FILE_MAX_ARG_COUNT        20
#define EVENT_FILE_MAX_VAR_LEN          256
#define BUFFER_FILE_MAX_ARG_COUNT       320
#define BUFFER_FILE_MAX_VAR_LEN         128

/**********************************
 * Set Cfg Functions
 **********************************/
static void SetCfgInputFile                     (const char *value, EbConfig_t *cfg) 
{
    if (cfg->inputFile && cfg->inputFile != stdin) {
        fclose(cfg->inputFile);
    }
    if (!strcmp(value, "stdin")) {
        cfg->inputFile = stdin;
    }
    else {
        FOPEN(cfg->inputFile, value, "rb"); 
    }
};
static void SetCfgStreamFile                    (const char *value, EbConfig_t *cfg) 
{
    if (cfg->bitstreamFile) { fclose(cfg->bitstreamFile); } 
    FOPEN(cfg->bitstreamFile,value, "wb");
};
static void SetCfgErrorFile                     (const char *value, EbConfig_t *cfg) 
{
    if (cfg->errorLogFile) { fclose(cfg->errorLogFile); } 
    FOPEN(cfg->errorLogFile,value, "w+");
};
static void SetCfgReconFile(const char *value, EbConfig_t *cfg)
{
    if (cfg->reconFile) { fclose(cfg->reconFile); }  
    FOPEN(cfg->reconFile,value, "wb");
};
static void SetCfgQpFile                        (const char *value, EbConfig_t *cfg) 
{
    if (cfg->qpFile) { fclose(cfg->qpFile); }  
    FOPEN(cfg->qpFile,value, "r");
};
static void SetCfgSourceWidth                   (const char *value, EbConfig_t *cfg) {cfg->sourceWidth = strtoul(value, NULL, 0);};
static void SetInterlacedVideo                  (const char *value, EbConfig_t *cfg) {cfg->interlacedVideo  = (EB_BOOL) strtoul(value, NULL, 0);};
static void SetSeperateFields                   (const char *value, EbConfig_t *cfg) {cfg->separateFields = (EB_BOOL) strtoul(value, NULL, 0);};
static void SetCfgSourceHeight                  (const char *value, EbConfig_t *cfg) {cfg->sourceHeight = strtoul(value, NULL, 0) >> cfg->separateFields;};
static void SetCfgFramesToBeEncoded             (const char *value, EbConfig_t *cfg) {cfg->framesToBeEncoded = strtol(value,  NULL, 0) << cfg->separateFields;};
static void SetBufferedInput                    (const char *value, EbConfig_t *cfg) {cfg->bufferedInput = (strtol(value, NULL, 0) != -1 && cfg->separateFields) ? strtol(value, NULL, 0) << cfg->separateFields : strtol(value, NULL, 0);};
static void SetFrameRate                        (const char *value, EbConfig_t *cfg) {
    cfg->frameRate = strtoul(value, NULL, 0);
    if (cfg->frameRate > 1000 ){
        cfg->frameRate = cfg->frameRate;
    }
    else{
        cfg->frameRate = cfg->frameRate << 16;
    }   
}
static void SetFrameRateNumerator               (const char *value, EbConfig_t *cfg) { cfg->frameRateNumerator = strtoul(value, NULL, 0);};
static void SetFrameRateDenominator             (const char *value, EbConfig_t *cfg) { cfg->frameRateDenominator = strtoul(value, NULL, 0);};
static void SetEncoderBitDepth                  (const char *value, EbConfig_t *cfg) {cfg->encoderBitDepth = strtoul(value, NULL, 0);}
static void SetcompressedTenBitFormat			(const char *value, EbConfig_t *cfg) {cfg->compressedTenBitFormat = strtoul(value, NULL, 0);}
static void SetBaseLayerSwitchMode              (const char *value, EbConfig_t *cfg) {cfg->baseLayerSwitchMode = (EB_BOOL) strtoul(value, NULL, 0);};
static void SetencMode                          (const char *value, EbConfig_t *cfg) {cfg->encMode = (EB_U8)strtoul(value, NULL, 0);};
static void SetCfgIntraPeriod                   (const char *value, EbConfig_t *cfg) {cfg->intraPeriod = strtol(value,  NULL, 0);};
static void SetCfgIntraRefreshType              (const char *value, EbConfig_t *cfg) {cfg->intraRefreshType = strtol(value,  NULL, 0);};
static void SetHierarchicalLevels				(const char *value, EbConfig_t *cfg) { cfg->hierarchicalLevels = strtol(value, NULL, 0); };
static void SetCfgPredStructure					(const char *value, EbConfig_t *cfg) { cfg->predStructure = strtol(value, NULL, 0); };
static void SetCfgQp                            (const char *value, EbConfig_t *cfg) {cfg->qp = strtoul(value, NULL, 0);};
static void SetCfgUseQpFile                     (const char *value, EbConfig_t *cfg) {cfg->useQpFile = (EB_BOOL)strtol(value, NULL, 0); };
static void SetDisableDlfFlag                   (const char *value, EbConfig_t *cfg) {cfg->disableDlfFlag = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetEnableSaoFlag                    (const char *value, EbConfig_t *cfg) {cfg->enableSaoFlag = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetEnableHmeFlag                    (const char *value, EbConfig_t *cfg) {cfg->enableHmeFlag = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetEnableHmeLevel0Flag              (const char *value, EbConfig_t *cfg) {cfg->enableHmeLevel0Flag = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetSceneChangeDetection             (const char *value, EbConfig_t *cfg) {cfg->sceneChangeDetection = strtoul(value, NULL, 0);};
static void SetLookAheadDistance                (const char *value, EbConfig_t *cfg) {cfg->lookAheadDistance = strtoul(value, NULL, 0);};
static void SetRateControlMode                  (const char *value, EbConfig_t *cfg) {cfg->rateControlMode = strtoul(value, NULL, 0);};
static void SetTargetBitRate                    (const char *value, EbConfig_t *cfg) {cfg->targetBitRate = strtoul(value, NULL, 0);};
static void SetMaxQpAllowed                     (const char *value, EbConfig_t *cfg) {cfg->maxQpAllowed = strtoul(value, NULL, 0);};
static void SetMinQpAllowed                     (const char *value, EbConfig_t *cfg) {cfg->minQpAllowed = strtoul(value, NULL, 0);};
static void SetEnableHmeLevel1Flag              (const char *value, EbConfig_t *cfg) {cfg->enableHmeLevel1Flag  = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetEnableHmeLevel2Flag              (const char *value, EbConfig_t *cfg) {cfg->enableHmeLevel2Flag  = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetCfgSearchAreaWidth               (const char *value, EbConfig_t *cfg) {cfg->searchAreaWidth = strtoul(value, NULL, 0);};
static void SetCfgSearchAreaHeight              (const char *value, EbConfig_t *cfg) {cfg->searchAreaHeight = strtoul(value, NULL, 0);};
static void SetCfgNumberHmeSearchRegionInWidth  (const char *value, EbConfig_t *cfg) {cfg->numberHmeSearchRegionInWidth = strtoul(value, NULL, 0);};
static void SetCfgNumberHmeSearchRegionInHeight (const char *value, EbConfig_t *cfg) {cfg->numberHmeSearchRegionInHeight = strtoul(value, NULL, 0);};
static void SetCfgHmeLevel0TotalSearchAreaWidth (const char *value, EbConfig_t *cfg) {cfg->hmeLevel0TotalSearchAreaWidth = strtoul(value, NULL, 0);};
static void SetCfgHmeLevel0TotalSearchAreaHeight(const char *value, EbConfig_t *cfg) {cfg->hmeLevel0TotalSearchAreaHeight = strtoul(value, NULL, 0);};
static void SetCfgUseDefaultMeHme               (const char *value, EbConfig_t *cfg) {cfg->useDefaultMeHme = (EB_BOOL)strtol(value, NULL, 0); };
static void SetHmeLevel0SearchAreaInWidthArray  (const char *value, EbConfig_t *cfg) {cfg->hmeLevel0SearchAreaInWidthArray[cfg->hmeLevel0ColumnIndex++] = strtoul(value, NULL, 0);};
static void SetHmeLevel0SearchAreaInHeightArray (const char *value, EbConfig_t *cfg) {cfg->hmeLevel0SearchAreaInHeightArray[cfg->hmeLevel0RowIndex++] = strtoul(value, NULL, 0);};
static void SetHmeLevel1SearchAreaInWidthArray  (const char *value, EbConfig_t *cfg) {cfg->hmeLevel1SearchAreaInWidthArray[cfg->hmeLevel1ColumnIndex++] = strtoul(value, NULL, 0);};
static void SetHmeLevel1SearchAreaInHeightArray (const char *value, EbConfig_t *cfg) {cfg->hmeLevel1SearchAreaInHeightArray[cfg->hmeLevel1RowIndex++] = strtoul(value, NULL, 0);};
static void SetHmeLevel2SearchAreaInWidthArray  (const char *value, EbConfig_t *cfg) {cfg->hmeLevel2SearchAreaInWidthArray[cfg->hmeLevel2ColumnIndex++] = strtoul(value, NULL, 0);};
static void SetHmeLevel2SearchAreaInHeightArray (const char *value, EbConfig_t *cfg) {cfg->hmeLevel2SearchAreaInHeightArray[cfg->hmeLevel2RowIndex++] = strtoul(value, NULL, 0);};
static void SetEnableConstrainedIntra           (const char *value, EbConfig_t *cfg) {cfg->constrainedIntra 											= (EB_BOOL)strtoul(value, NULL, 0);};
static void SetCfgTune                          (const char *value, EbConfig_t *cfg) { cfg->tune = (EB_U8)strtoul(value, NULL, 0); };
static void SetBitRateReduction                 (const char *value, EbConfig_t *cfg) { cfg->bitRateReduction = (EB_BOOL)strtol(value, NULL, 0); };
static void SetImproveSharpness                 (const char *value, EbConfig_t *cfg) {cfg->improveSharpness               = (EB_BOOL)strtol(value,  NULL, 0);};
static void SetVideoUsabilityInfo               (const char *value, EbConfig_t *cfg) {cfg->videoUsabilityInfo               = strtol(value,  NULL, 0);};
static void SetHighDynamicRangeInput            (const char *value, EbConfig_t *cfg) {cfg->highDynamicRangeInput            = strtol(value,  NULL, 0);};
static void SetAccessUnitDelimiter              (const char *value, EbConfig_t *cfg) {cfg->accessUnitDelimiter              = strtol(value,  NULL, 0);};
static void SetBufferingPeriodSEI               (const char *value, EbConfig_t *cfg) {cfg->bufferingPeriodSEI               = strtol(value,  NULL, 0);};
static void SetPictureTimingSEI                 (const char *value, EbConfig_t *cfg) {cfg->pictureTimingSEI                 = strtol(value,  NULL, 0);};
static void SetRegisteredUserDataSEI            (const char *value, EbConfig_t *cfg) {cfg->registeredUserDataSeiFlag        = (EB_BOOL)strtol(value,  NULL, 0);};
static void SetUnRegisteredUserDataSEI          (const char *value, EbConfig_t *cfg) {cfg->unregisteredUserDataSeiFlag      = (EB_BOOL)strtol(value,  NULL, 0);};
static void SetRecoveryPointSEI                 (const char *value, EbConfig_t *cfg) {cfg->recoveryPointSeiFlag             = (EB_BOOL)strtol(value,  NULL, 0);};
static void SetEnableTemporalId                 (const char *value, EbConfig_t *cfg) {cfg->enableTemporalId                 = strtol(value,  NULL, 0);};
static void SetProfile                          (const char *value, EbConfig_t *cfg) {cfg->profile                          = strtol(value,  NULL, 0);};
static void SetTier                             (const char *value, EbConfig_t *cfg) {cfg->tier                             = strtol(value,  NULL, 0);};
static void SetLevel                            (const char *value, EbConfig_t *cfg) {	
	if (strtoul( value, NULL,0) != 0 || EB_STRCMP(value, "0") == 0 )
		cfg->level = (EB_U32)(10*strtod(value,  NULL));
	else 
		cfg->level = 9999999;
};
static void SetInjector                         (const char *value, EbConfig_t *cfg) {cfg->injector                         = strtol(value,  NULL, 0);};
static void SpeedControlFlag                    (const char *value, EbConfig_t *cfg) { cfg->speedControlFlag = strtol(value, NULL, 0); };
static void SetInjectorFrameRate                (const char *value, EbConfig_t *cfg) {
    cfg->injectorFrameRate = strtoul(value, NULL, 0);
    if (cfg->injectorFrameRate > 1000 ){
        cfg->injectorFrameRate = cfg->injectorFrameRate;
    }
    else{
        cfg->injectorFrameRate = cfg->injectorFrameRate << 16;
    }   
}
static void	SetLatencyMode                      (const char *value, EbConfig_t *cfg)  {cfg->latencyMode               = (EB_U8)strtol(value, NULL, 0);};
static void SetAsmType                          (const char *value, EbConfig_t *cfg)  { cfg->asmType = (EB_ASM)strtoul(value, NULL, 0); };
static void SetUseRoundRobinThreadAssignment    (const char *value, EbConfig_t *cfg) {cfg->useRoundRobinThreadAssignment   = (EB_BOOL)strtol(value, NULL, 0);}; 
static void SetTargetSocket                     (const char *value, EbConfig_t *cfg)  { cfg->targetSocket = (EB_U8)strtoul(value, NULL, 0); };

enum cfg_type{
    SINGLE_INPUT,   // Configuration parameters that have only 1 value input
    ARRAY_INPUT     // Configuration parameters that have multiple values as input
};

/**********************************
 * Config Entry Struct
 **********************************/
typedef struct config_entry_s {
    enum  cfg_type type;
    const char *token;
    const char *name;
    void (*scf)(const char *, EbConfig_t *);
} config_entry_t;

/**********************************
 * Config Entry Array
 **********************************/
config_entry_t config_entry[] = {

    // File I/O
    { SINGLE_INPUT, INPUT_FILE_TOKEN, "InputFile", SetCfgInputFile },
    { SINGLE_INPUT, OUTPUT_BITSTREAM_TOKEN,   "StreamFile",       SetCfgStreamFile },
    { SINGLE_INPUT, ERROR_FILE_TOKEN, "ErrorFile", SetCfgErrorFile },
    { SINGLE_INPUT, OUTPUT_RECON_TOKEN, "ReconFile", SetCfgReconFile },
    { SINGLE_INPUT, QP_FILE_TOKEN, "QpFile", SetCfgQpFile },

    // Interlaced Video 
    { SINGLE_INPUT, INTERLACED_VIDEO_TOKEN , "InterlacedVideo" , SetInterlacedVideo },
    { SINGLE_INPUT, SEPERATE_FILDS_TOKEN, "SeperateFields", SetSeperateFields },

    // Picture Dimensions                             
    { SINGLE_INPUT, WIDTH_TOKEN, "SourceWidth", SetCfgSourceWidth },
    { SINGLE_INPUT, HEIGHT_TOKEN, "SourceHeight", SetCfgSourceHeight },
                                                          
    // Prediction Structure                           
    { SINGLE_INPUT, NUMBER_OF_PICTURES_TOKEN, "FrameToBeEncoded", SetCfgFramesToBeEncoded },
    { SINGLE_INPUT, BUFFERED_INPUT_TOKEN, "BufferedInput", SetBufferedInput },
    { SINGLE_INPUT, BASE_LAYER_SWITCH_MODE_TOKEN, "BaseLayerSwitchMode", SetBaseLayerSwitchMode },
    { SINGLE_INPUT, ENCMODE_TOKEN, "EncoderMode", SetencMode},
    { SINGLE_INPUT, INTRA_PERIOD_TOKEN, "IntraPeriod", SetCfgIntraPeriod },
    { SINGLE_INPUT, INTRA_REFRESH_TYPE_TOKEN, "IntraRefreshType", SetCfgIntraRefreshType },
    { SINGLE_INPUT, FRAME_RATE_TOKEN, "FrameRate", SetFrameRate },
    { SINGLE_INPUT, FRAME_RATE_NUMERATOR_TOKEN, "FrameRateNumerator", SetFrameRateNumerator },
    { SINGLE_INPUT, FRAME_RATE_DENOMINATOR_TOKEN, "FrameRateDenominator", SetFrameRateDenominator },
    { SINGLE_INPUT, ENCODER_BIT_DEPTH, "EncoderBitDepth", SetEncoderBitDepth },
	{ SINGLE_INPUT, INPUT_COMPRESSED_TEN_BIT_FORMAT, "CompressedTenBitFormat", SetcompressedTenBitFormat },
	{ SINGLE_INPUT, HIERARCHICAL_LEVELS_TOKEN, "HierarchicalLevels", SetHierarchicalLevels },

	{ SINGLE_INPUT, PRED_STRUCT_TOKEN, "PredStructure", SetCfgPredStructure },

                                                      
    // Rate Control                                   
    { SINGLE_INPUT, SCENE_CHANGE_DETECTION_TOKEN, "SceneChangeDetection", SetSceneChangeDetection}, 
    { SINGLE_INPUT, QP_TOKEN, "QP", SetCfgQp },

    { SINGLE_INPUT, USE_QP_FILE_TOKEN, "UseQpFile", SetCfgUseQpFile },

    { SINGLE_INPUT, RATE_CONTROL_ENABLE_TOKEN, "RateControlMode", SetRateControlMode },
    { SINGLE_INPUT, LOOK_AHEAD_DIST_TOKEN, "LookAheadDistance",                             SetLookAheadDistance},
    { SINGLE_INPUT, TARGET_BIT_RATE_TOKEN, "TargetBitRate", SetTargetBitRate },
    { SINGLE_INPUT, MAX_QP_TOKEN, "MaxQpAllowed", SetMaxQpAllowed },
    { SINGLE_INPUT, MIN_QP_TOKEN, "MinQpAllowed", SetMinQpAllowed },
                                                      
    // DLF                                            
    { SINGLE_INPUT, LOOP_FILTER_DISABLE_TOKEN, "LoopFilterDisable", SetDisableDlfFlag },
                                                      
    // SAO                                            
    { SINGLE_INPUT, SAO_ENABLE_TOKEN, "SAO", SetEnableSaoFlag },

    // ME Tools                                       
    { SINGLE_INPUT, USE_DEFAULT_ME_HME_TOKEN, "UseDefaultMeHme", SetCfgUseDefaultMeHme },
    { SINGLE_INPUT, HME_ENABLE_TOKEN, "HME", SetEnableHmeFlag },
    { SINGLE_INPUT, HME_L0_ENABLE_TOKEN, "HMELevel0", SetEnableHmeLevel0Flag },
    { SINGLE_INPUT, HME_L1_ENABLE_TOKEN, "HMELevel1", SetEnableHmeLevel1Flag },
    { SINGLE_INPUT, HME_L2_ENABLE_TOKEN, "HMELevel2", SetEnableHmeLevel2Flag },
                                                
    // ME Parameters                                  
    { SINGLE_INPUT, SEARCH_AREA_WIDTH_TOKEN, "SearchAreaWidth", SetCfgSearchAreaWidth },
    { SINGLE_INPUT, SEARCH_AREA_HEIGHT_TOKEN, "SearchAreaHeight", SetCfgSearchAreaHeight },
                                                      
    // HME Parameters                                 
    { SINGLE_INPUT, NUM_HME_SEARCH_WIDTH_TOKEN, "NumberHmeSearchRegionInWidth", SetCfgNumberHmeSearchRegionInWidth },
    { SINGLE_INPUT, NUM_HME_SEARCH_HEIGHT_TOKEN, "NumberHmeSearchRegionInHeight", SetCfgNumberHmeSearchRegionInHeight },
    { SINGLE_INPUT, HME_SRCH_T_L0_WIDTH_TOKEN, "HmeLevel0TotalSearchAreaWidth", SetCfgHmeLevel0TotalSearchAreaWidth },
    { SINGLE_INPUT, HME_SRCH_T_L0_HEIGHT_TOKEN, "HmeLevel0TotalSearchAreaHeight", SetCfgHmeLevel0TotalSearchAreaHeight },
                                                      
    // MD Parameters         
    { SINGLE_INPUT, CONSTRAINED_INTRA_ENABLE_TOKEN, "ConstrainedIntra", SetEnableConstrainedIntra},


    // Tune
    { SINGLE_INPUT, TUNE_TOKEN, "Tune", SetCfgTune },

    // Windows Manual Thread Management
    { SINGLE_INPUT, RR_THREAD_MGMNT, "UseRoundRobinThreadAssignment", SetUseRoundRobinThreadAssignment },
    { SINGLE_INPUT, TARGET_SOCKET, "TargetSocket", SetTargetSocket },

    // Optional Features 

	{ SINGLE_INPUT, BITRATE_REDUCTION_TOKEN, "BitRateReduction", SetBitRateReduction },
    { SINGLE_INPUT, IMPROVE_SHARPNESS_TOKEN,"ImproveSharpness", SetImproveSharpness},
    { SINGLE_INPUT, VIDEO_USE_INFO_TOKEN, "VideoUsabilityInfo", SetVideoUsabilityInfo },
    { SINGLE_INPUT, HDR_INPUT_TOKEN, "HighDynamicRangeInput", SetHighDynamicRangeInput },
    { SINGLE_INPUT, ACCESS_UNIT_DELM_TOKEN, "AccessUnitDelimiter", SetAccessUnitDelimiter },
    { SINGLE_INPUT, BUFF_PERIOD_TOKEN, "BufferingPeriod", SetBufferingPeriodSEI },
    { SINGLE_INPUT, PIC_TIMING_TOKEN, "PictureTiming", SetPictureTimingSEI },
    { SINGLE_INPUT, REG_USER_DATA_TOKEN, "RegisteredUserData", SetRegisteredUserDataSEI },
    { SINGLE_INPUT, UNREG_USER_DATA_TOKEN, "UnregisteredUserData", SetUnRegisteredUserDataSEI },
    { SINGLE_INPUT, RECOVERY_POINT_TOKEN, "RecoveryPoint", SetRecoveryPointSEI },
    { SINGLE_INPUT, TEMPORAL_ID, "TemporalId", SetEnableTemporalId },
    // Latency 
    { SINGLE_INPUT, INJECTOR_TOKEN, "Injector", SetInjector },
    { SINGLE_INPUT, INJECTOR_FRAMERATE_TOKEN, "InjectorFrameRate", SetInjectorFrameRate },

    { SINGLE_INPUT, SPEED_CONTROL_TOKEN, "SpeedControlFlag", SpeedControlFlag },

    // Annex A parameters                             
    { SINGLE_INPUT, PROFILE_TOKEN, "Profile", SetProfile },
    { SINGLE_INPUT, TIER_TOKEN, "Tier", SetTier },
    { SINGLE_INPUT, LEVEL_TOKEN, "Level", SetLevel },
    { SINGLE_INPUT, LATENCY_MODE, "LatencyMode", SetLatencyMode },

    // Asm Type
    { SINGLE_INPUT, ASM_TYPE_TOKEN, "AsmType", SetAsmType },
    // HME 
    { ARRAY_INPUT,HME_LEVEL0_WIDTH, "HmeLevel0SearchAreaInWidth", SetHmeLevel0SearchAreaInWidthArray },
    { ARRAY_INPUT,HME_LEVEL0_HEIGHT, "HmeLevel0SearchAreaInHeight", SetHmeLevel0SearchAreaInHeightArray },
    { ARRAY_INPUT,HME_LEVEL1_WIDTH, "HmeLevel1SearchAreaInWidth", SetHmeLevel1SearchAreaInWidthArray },
    { ARRAY_INPUT,HME_LEVEL1_HEIGHT, "HmeLevel1SearchAreaInHeight", SetHmeLevel1SearchAreaInHeightArray },
    { ARRAY_INPUT,HME_LEVEL2_WIDTH, "HmeLevel2SearchAreaInWidth", SetHmeLevel2SearchAreaInWidthArray },
    { ARRAY_INPUT,HME_LEVEL2_HEIGHT, "HmeLevel2SearchAreaInHeight", SetHmeLevel2SearchAreaInHeightArray },
    // Termination
    {SINGLE_INPUT,NULL,  NULL,                                NULL}
};

/**********************************
 * Constructor
 **********************************/
void EbConfigCtor(EbConfig_t *configPtr)
{
    configPtr->configFile                           = NULL;
    configPtr->inputFile                            = NULL;
    configPtr->bitstreamFile                        = NULL;
    configPtr->reconFile                            = NULL;
    configPtr->errorLogFile                         = stderr;
    configPtr->qpFile                               = NULL;
    
    
    configPtr->frameRate                            = 60;
    configPtr->frameRateNumerator                   = 0;
    configPtr->frameRateDenominator                 = 0;
    configPtr->encoderBitDepth                      = 8;    
	configPtr->compressedTenBitFormat			    = 0;
    configPtr->sourceWidth                          = 0;
    configPtr->sourceHeight                         = 0;
    configPtr->inputPaddedWidth                     = 0;
    configPtr->inputPaddedHeight                    = 0;
    configPtr->framesToBeEncoded                    = 0;
    configPtr->bufferedInput                        = -1;
    configPtr->sequenceBuffer                       = 0;
    configPtr->latencyMode                          = 0;
    
    // Interlaced Video 
    configPtr->interlacedVideo                      = EB_FALSE;
    configPtr->separateFields                       = EB_FALSE;
    configPtr->qp                                   = 32;
    configPtr->useQpFile                            = EB_FALSE;

    configPtr->sceneChangeDetection                 = 1;
    configPtr->rateControlMode                      = 0;
    configPtr->lookAheadDistance                    = (EB_U32)~0;
    configPtr->targetBitRate                        = 7000000;
    configPtr->maxQpAllowed                         = 48;
    configPtr->minQpAllowed                         = 10;
    configPtr->baseLayerSwitchMode                  = 0;
	configPtr->encMode								= 9;
    configPtr->intraPeriod                          = -2;
    configPtr->intraRefreshType                     = 1;
	configPtr->hierarchicalLevels					= 3;
	configPtr->predStructure						= 2;
    configPtr->disableDlfFlag                       = EB_FALSE;
    configPtr->enableSaoFlag                        = EB_TRUE;
    configPtr->useDefaultMeHme                      = EB_TRUE;
    configPtr->enableHmeFlag                        = EB_TRUE;
    configPtr->enableHmeLevel0Flag                  = EB_TRUE;
    configPtr->enableHmeLevel1Flag                  = EB_FALSE;
    configPtr->enableHmeLevel2Flag                  = EB_FALSE;
    configPtr->searchAreaWidth                      = 16;
    configPtr->searchAreaHeight                     = 7;
    configPtr->numberHmeSearchRegionInWidth         = 2;
    configPtr->numberHmeSearchRegionInHeight        = 2;
    configPtr->hmeLevel0TotalSearchAreaWidth        = 64;
    configPtr->hmeLevel0TotalSearchAreaHeight       = 25;
    configPtr->hmeLevel0ColumnIndex                 = 0;
    configPtr->hmeLevel0RowIndex                    = 0;
    configPtr->hmeLevel1ColumnIndex                 = 0;
    configPtr->hmeLevel1RowIndex                    = 0;
    configPtr->hmeLevel2ColumnIndex                 = 0;
    configPtr->hmeLevel2RowIndex                    = 0;
    configPtr->hmeLevel0SearchAreaInWidthArray[0]   = 32;
    configPtr->hmeLevel0SearchAreaInWidthArray[1]   = 32;
    configPtr->hmeLevel0SearchAreaInHeightArray[0]  = 12;
    configPtr->hmeLevel0SearchAreaInHeightArray[1]  = 13;
    configPtr->hmeLevel1SearchAreaInWidthArray[0]   = 1;
    configPtr->hmeLevel1SearchAreaInWidthArray[1]   = 1;
    configPtr->hmeLevel1SearchAreaInHeightArray[0]  = 1;
    configPtr->hmeLevel1SearchAreaInHeightArray[1]  = 1;
    configPtr->hmeLevel2SearchAreaInWidthArray[0]   = 1;
    configPtr->hmeLevel2SearchAreaInWidthArray[1]   = 1;
    configPtr->hmeLevel2SearchAreaInHeightArray[0]  = 1;
    configPtr->hmeLevel2SearchAreaInHeightArray[1]  = 1;
    configPtr->constrainedIntra                     = EB_FALSE;
    configPtr->tune                                 = 1; // OQ By Default
    // Thresholds
    configPtr->videoUsabilityInfo                   = 0;
    configPtr->highDynamicRangeInput                = 0;
    configPtr->accessUnitDelimiter                  = 0;
    configPtr->bufferingPeriodSEI                   = 0;
    configPtr->pictureTimingSEI                     = 0;

	configPtr->bitRateReduction					    = EB_TRUE;
    configPtr->improveSharpness                     = EB_TRUE;
    configPtr->registeredUserDataSeiFlag            = EB_FALSE;
    configPtr->unregisteredUserDataSeiFlag          = EB_FALSE;
    configPtr->recoveryPointSeiFlag                 = EB_FALSE;
    configPtr->enableTemporalId                     = 1;

    // Annex A parameters
    configPtr->profile                              = 2;
    configPtr->tier                                 = 0;
    configPtr->level                                = 0;

    // Latency
    configPtr->injector                                     = 0;
    configPtr->injectorFrameRate                            = 60 << 16;
    configPtr->speedControlFlag                             = 0;


    // Testing
    configPtr->testUserData                                 = 0;
	configPtr->eosFlag										= EB_FALSE;

// Computational Performance Parameters
    configPtr->performanceContext.frameCount                = 0;
    configPtr->performanceContext.averageSpeed              = 0;
    configPtr->performanceContext.startsTime                = 0;
    configPtr->performanceContext.startuTime                = 0;
    configPtr->performanceContext.maxLatency                = 0;
    configPtr->performanceContext.totalLatency              = 0;    
    configPtr->performanceContext.byteCount                 = 0;
    
    // ASM Type
    configPtr->asmType                                      = EB_ASM_AVX2;

    configPtr->stopEncoder                                  = EB_FALSE;
    configPtr->useRoundRobinThreadAssignment                = EB_FALSE;
	configPtr->targetSocket                                 = 1;
    configPtr->processedFrameCount                          = 0;
    configPtr->processedByteCount                           = 0;
    

    return;
}

/**********************************
 * Destructor
 **********************************/
void EbConfigDtor(EbConfig_t *configPtr)
{

    // Close any files that are open
    if (configPtr->configFile) {
        fclose(configPtr->configFile);
        configPtr->configFile = (FILE *) NULL;
    }

    if (configPtr->inputFile) {
        if (configPtr->inputFile != stdin) fclose(configPtr->inputFile);
        configPtr->inputFile = (FILE *) NULL;
    }

    if (configPtr->bitstreamFile) {
        fclose(configPtr->bitstreamFile);
        configPtr->bitstreamFile = (FILE *) NULL;
    }

    if (configPtr->reconFile) {
        fclose(configPtr->reconFile);
        configPtr->reconFile = (FILE *)NULL;
    }

    if (configPtr->errorLogFile) {
        fclose(configPtr->errorLogFile);
        configPtr->errorLogFile = (FILE *) NULL;
    }

    if (configPtr->qpFile) {
        fclose(configPtr->qpFile);
        configPtr->qpFile = (FILE *)NULL;
    }

    return;
}

/**********************************
 * File Size
 **********************************/
static int findFileSize(
    FILE * const pFile)
{
    int fileSize;

    fseek(pFile, 0, SEEK_END);
    fileSize = ftell(pFile);
    rewind(pFile);

    return fileSize;
}

/**********************************
 * Line Split
 **********************************/
static void lineSplit(
    unsigned int   *argc,
    char           *argv  [CONFIG_FILE_MAX_ARG_COUNT],
    unsigned int    argLen[CONFIG_FILE_MAX_ARG_COUNT],
    char           *linePtr)
{
    unsigned int i=0;
    *argc = 0;

    while((*linePtr != CONFIG_FILE_NEWLINE_CHAR) &&
            (*linePtr != CONFIG_FILE_RETURN_CHAR) &&
            (*linePtr != CONFIG_FILE_COMMENT_CHAR) &&
            (*argc < CONFIG_FILE_MAX_ARG_COUNT)) {
        // Increment past whitespace
        while((*linePtr == CONFIG_FILE_SPACE_CHAR || *linePtr == CONFIG_FILE_TAB_CHAR) && (*linePtr != CONFIG_FILE_NEWLINE_CHAR)) {
            ++linePtr;
        }

        // Set arg
        if ((*linePtr != CONFIG_FILE_NEWLINE_CHAR) &&
                (*linePtr != CONFIG_FILE_RETURN_CHAR) &&
                (*linePtr != CONFIG_FILE_COMMENT_CHAR) &&
                (*argc < CONFIG_FILE_MAX_ARG_COUNT)) {
            argv[*argc] = linePtr;

            // Increment to next whitespace
            while(*linePtr != CONFIG_FILE_SPACE_CHAR &&
                    *linePtr != CONFIG_FILE_TAB_CHAR &&
                    *linePtr != CONFIG_FILE_NEWLINE_CHAR &&
                    *linePtr != CONFIG_FILE_RETURN_CHAR) {
                ++linePtr;
                ++i;
            }

            // Set arg length
            argLen[(*argc)++] = i;

            i=0;
        }
    }

    return;
}


/**********************************
* Set Config Value
**********************************/
static void SetConfigValue(
	EbConfig_t *config,
	char *name,
	char *value)
{
	int i=0;

	while(config_entry[i].name != NULL) {
		if(EB_STRCMP(config_entry[i].name, name) == 0)  {
			(*config_entry[i].scf)((const char *) value, config);
		}
		++i;
	}

	return;
}

/**********************************
* Parse Config File
**********************************/
static void ParseConfigFile(
	EbConfig_t *config,
	char       *buffer,
	int         size)
{
	unsigned int argc;
	char *argv[CONFIG_FILE_MAX_ARG_COUNT];
	unsigned int argLen[CONFIG_FILE_MAX_ARG_COUNT];

	char varName[CONFIG_FILE_MAX_VAR_LEN];
	char varValue[CONFIG_FILE_MAX_ARG_COUNT][CONFIG_FILE_MAX_VAR_LEN];

	unsigned int valueIndex;

	unsigned int commentSectionFlag = 0;
	unsigned int newLineFlag = 0;

	// Keep looping until we process the entire file
	while(size--) {
		commentSectionFlag = ((*buffer == CONFIG_FILE_COMMENT_CHAR) || (commentSectionFlag != 0)) ? 1 : commentSectionFlag;

		// At the beginning of each line
		if ((newLineFlag == 1) && (commentSectionFlag == 0)) {
			// Do an argc/argv split for the line
			lineSplit(&argc, argv, argLen, buffer);

			if ((argc > 2) && (*argv[1] == CONFIG_FILE_VALUE_SPLIT)) {
				// ***NOTE - We're assuming that the variable name is the first arg and
				// the variable value is the third arg.

				// Cap the length of the variable name
				argLen[0] = (argLen[0] > CONFIG_FILE_MAX_VAR_LEN - 1) ? CONFIG_FILE_MAX_VAR_LEN - 1 : argLen[0];
				// Copy the variable name
				EB_STRNCPY(varName, argv[0], argLen[0]);
				// Null terminate the variable name
				varName[argLen[0]] = CONFIG_FILE_NULL_CHAR;

				for(valueIndex=0; (valueIndex < CONFIG_FILE_MAX_ARG_COUNT - 2) && (valueIndex < (argc - 2)); ++valueIndex) {

					// Cap the length of the variable
					argLen[valueIndex+2] = (argLen[valueIndex+2] > CONFIG_FILE_MAX_VAR_LEN - 1) ? CONFIG_FILE_MAX_VAR_LEN - 1 : argLen[valueIndex+2];
					// Copy the variable name
					EB_STRNCPY(varValue[valueIndex], argv[valueIndex+2], argLen[valueIndex+2]);
					// Null terminate the variable name
					varValue[valueIndex][argLen[valueIndex+2]] = CONFIG_FILE_NULL_CHAR;

					SetConfigValue(config, varName, varValue[valueIndex]);
				}
			}
		}

		commentSectionFlag = (*buffer == CONFIG_FILE_NEWLINE_CHAR) ? 0 : commentSectionFlag;
		newLineFlag = (*buffer == CONFIG_FILE_NEWLINE_CHAR) ? 1 : 0;
		++buffer;
	}

	return;
}

/******************************************
* Find Token
******************************************/
static int FindToken(
	int             argc,
	char * const    argv[],
	char const *    token,
	char*           configStr)
{
	int return_error = -1;

	while((argc > 0) && (return_error != 0)) {
		return_error = EB_STRCMP(argv[--argc], token);
		if (return_error == 0) {
			EB_STRCPY(configStr, COMMAND_LINE_MAX_SIZE, argv[argc + 1]);
		}
	}

	return return_error;
}

/**********************************
* Read Config File
**********************************/
static int ReadConfigFile(
	EbConfig_t  *config,
	char		*configPath,
	EB_U32		 instanceIdx)
{
	int return_error = 0;

	// Open the config file
	FOPEN(config->configFile, configPath, "rb");

	if (config->configFile != (FILE*) NULL) {
		int configFileSize = findFileSize(config->configFile);
		char *configFileBuffer = (char*) malloc(configFileSize);

		if (configFileBuffer != (char *) NULL) {
			int resultSize = (int) fread(configFileBuffer, 1, configFileSize, config->configFile);

			if (resultSize == configFileSize) {
				ParseConfigFile(config, configFileBuffer, configFileSize);
			} else {
				printf("Error channel %u: File Read Failed\n",instanceIdx+1);
				return_error = -1;
			}
		} else {
			printf("Error channel %u: Memory Allocation Failed\n",instanceIdx+1);
			return_error = -1;
		}

		free(configFileBuffer);
		fclose(config->configFile);
		config->configFile = (FILE*) NULL;
	} else {
		printf("Error channel %u: Couldn't open Config File: %s\n", instanceIdx+1,configPath);
		return_error = -1;
	}

	return return_error;
}

/******************************************
* Verify Settings
******************************************/
static EB_ERRORTYPE VerifySettings(EbConfig_t *config, unsigned int channelNumber)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	// Check Input File
	if(config->inputFile == (FILE*) NULL) {
		fprintf(config->errorLogFile, "Error instance %u: Invalid Input File\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	} 

    if (config->framesToBeEncoded <= -1) {
		fprintf(config->errorLogFile, "Error instance %u: FrameToBeEncoded must be greater than 0\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	} 

    if (config->bufferedInput < -1) {
        fprintf(config->errorLogFile, "Error instance %u: Invalid BufferedInput. BufferedInput must greater or equal to -1\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

	if (config->bufferedInput > config->framesToBeEncoded) {
		fprintf(config->errorLogFile, "Error instance %u: Invalid BufferedInput. BufferedInput must be less or equal to the number of frames to be encoded\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	} 

    if (config->useQpFile == EB_TRUE && config->qpFile == NULL) {
        fprintf(config->errorLogFile, "Error instance %u: Could not find QP file, UseQpFile is set to 1\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->separateFields > 1) {
        fprintf(config->errorLogFile, "Error Instance %u: Invalid SeperateFields Input\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->encoderBitDepth == 10 && config->separateFields == 1)
    {
        fprintf(config->errorLogFile, "Error instance %u: Separate fields is not supported for 10 bit input \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->injector > 1 ){
        fprintf(config->errorLogFile, "Error Instance %u: Invalid injector [0 - 1]\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    } 

    if(config->injectorFrameRate > (240<<16) && config->injector){
        fprintf(config->errorLogFile, "Error Instance %u: The maximum allowed injectorFrameRate is 240 fps\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }
    // Check that the injector frameRate is non-zero
    if(config->injectorFrameRate <= 0 && config->injector) {
        fprintf(config->errorLogFile, "Error Instance %u: The injector frame rate should be greater than 0 fps \n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

	// UseRoundRobinThreadAssignment
	if (config->useRoundRobinThreadAssignment != 0 && config->useRoundRobinThreadAssignment != 1) {
		fprintf(config->errorLogFile, "Error instance %u: Invalid UseRoundRobinThreadAssignment [0 - 1]\n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}

    // TargetSocket
    if (config->targetSocket != 0 && config->targetSocket != 1 && config->targetSocket != 2) {
        fprintf(config->errorLogFile, "Error instance %u: Invalid TargetSocket [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    return return_error;
}

/******************************************
 * Find Token for multiple inputs
 ******************************************/
int FindTokenMultipleInputs(
    int             argc,
    char* const     argv[],
    const char*     token,
	char**          configStr)
{
	int return_error = -1;
	int done = 0;
	while((argc > 0) && (return_error != 0)) {
		return_error = EB_STRCMP(argv[--argc], token);
		if (return_error == 0) {
			int count;
			for (count=0; count < MAX_CHANNEL_NUMBER  ; ++count){
				if (done ==0){
					if (argv[argc + count + 1] ){
						if (strtoul(argv[argc + count + 1], NULL,0) != 0 || EB_STRCMP(argv[argc + count + 1], "0") == 0 ){
							EB_STRCPY(configStr[count], COMMAND_LINE_MAX_SIZE, argv[argc + count + 1]);
						}else if (argv[argc + count + 1][0] != '-'){
							EB_STRCPY(configStr[count], COMMAND_LINE_MAX_SIZE, argv[argc + count + 1]);
						}else {
							EB_STRCPY(configStr[count], COMMAND_LINE_MAX_SIZE," ");
							done = 1;
						}
					}else{
						EB_STRCPY(configStr[count], COMMAND_LINE_MAX_SIZE, " ");
						done =1;
						//return return_error;
					}
				}else
					EB_STRCPY(configStr[count], COMMAND_LINE_MAX_SIZE, " ");
			}
		}
	}

	return return_error;
}

unsigned int GetHelp(int argc, char *const argv[])
{
	char config_string[COMMAND_LINE_MAX_SIZE];
    if (FindToken(argc, argv, HELP_TOKEN, config_string) == 0) {
        int token_index = -1;

        printf("\n%-25s\t%-25s\t%-25s\t\n\n" ,"TOKEN", "DESCRIPTION", "INPUT TYPE");
        printf("%-25s\t%-25s\t%-25s\t\n" ,"-nch", "NumberOfChannels", "Single input");
        while (config_entry[++token_index].token != NULL) {
            printf("%-25s\t%-25s\t%-25s\t\n", config_entry[token_index].token, config_entry[token_index].name, config_entry[token_index].type ? "Array input": "Single input");
        }
        return 1;

    }
    else {
        return 0;
    }
}

/******************************************************
* Get the number of channels and validate it with input 
******************************************************/
unsigned int GetNumberOfChannels(int argc, char *const argv[])
{
	char config_string[COMMAND_LINE_MAX_SIZE];
	unsigned int channelNumber;
	if (FindToken(argc, argv, CHANNEL_NUMBER_TOKEN, config_string) == 0) {

		// Set the input file    
		channelNumber = strtol(config_string,  NULL, 0);
		if ((channelNumber > (EB_U32) MAX_CHANNEL_NUMBER) || channelNumber == 0){
			printf("Error: The number of channels has to be within the range [1,%u]\n",(unsigned int) MAX_CHANNEL_NUMBER);
			return 0;		
		}else{
			return channelNumber;
		}
	}
	
	return 1;
}

#ifdef _MSC_VER
//  VS
#define cpuid(info,x)    __cpuidex(info,x,0)
#else
//  GCC Inline Assembly
void cpuid(int CPUInfo[4], int InfoType){
	__asm__ __volatile__(
		"cpuid":
	"=a" (CPUInfo[0]),
		"=b" (CPUInfo[1]),
		"=c" (CPUInfo[2]),
		"=d" (CPUInfo[3]) :
		"a" (InfoType), "c" (0)
		);
}
#endif

void mark_token_as_read(
    const char * token,
    char * cmd_copy[],
    int * cmd_token_cnt
    )
{
    int cmd_copy_index;
    for (cmd_copy_index = 0; cmd_copy_index < *(cmd_token_cnt); ++cmd_copy_index) {
        if (!EB_STRCMP(cmd_copy[cmd_copy_index], token)) {
            cmd_copy[cmd_copy_index] = cmd_copy[--(*cmd_token_cnt)];
        }
    }
}

EB_BOOL is_negative_number(const char* string) {
    int length = (int)strlen(string);
    int index = 0;
    if (string[0] != '-') return EB_FALSE;
    for (index = 1; index < length; index++)
    {
        if (string[index] < '0' || string[index] > '9')
            return EB_FALSE;
    }
    return EB_TRUE;
}

#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height,is16bit) ( ( ((width)*(height)*3)>>1 )<<is16bit)
// Computes the number of frames in the input file
EB_S32 ComputeFramesToBeEncoded(
    EbConfig_t   *config)
{
    EB_U64 fileSize = 0;
    EB_S32 frameCount = 0;
    EB_U32 frameSize;
    if (config->inputFile) {
        fseeko64(config->inputFile, 0L, SEEK_END);
        fileSize = ftello64(config->inputFile);
    }
    
    frameSize = SIZE_OF_ONE_FRAME_IN_BYTES(config->inputPaddedWidth, config->inputPaddedHeight, (EB_U8)((config->encoderBitDepth == 10) ? 1 : 0));

    if (frameSize == 0)
        return -1;

    if (config->encoderBitDepth == 10 && config->compressedTenBitFormat == 1)
        frameCount = (EB_S32)(2 * ((double)fileSize / frameSize) / 1.25);
    else
        frameCount = (EB_S32)(fileSize / frameSize);

    if (frameCount == 0)
        return -1;
    
    return frameCount;

}

/******************************************
* Read Command Line
******************************************/
EB_ERRORTYPE ReadCommandLine(
	int            argc,
	char *const    argv[],
	EbConfig_t   **configs,
	unsigned int   numChannels,
	EB_ERRORTYPE *return_errors)
{

    EB_ERRORTYPE return_error = EB_ErrorBadParameter;
    char		    config_string[COMMAND_LINE_MAX_SIZE];		// for one input options
    char		   *config_strings[MAX_CHANNEL_NUMBER]; // for multiple input options
    char           *cmd_copy[MAX_NUM_TOKENS];                 // keep track of extra tokens
    unsigned int    index           = 0;
    int             cmd_token_cnt   = 0;                        // total number of tokens
    int             token_index     = -1;

    for (index = 0; index < MAX_CHANNEL_NUMBER; ++index){
        config_strings[index] = (char*)malloc(sizeof(char)*COMMAND_LINE_MAX_SIZE);
    }

    // Copy tokens (except for CHANNEL_NUMBER_TOKEN ) into a temp token buffer hosting all tokens that are passed through the command line
	size_t len = EB_STRLEN(CHANNEL_NUMBER_TOKEN, COMMAND_LINE_MAX_SIZE);
    for (token_index = 0; token_index < argc; ++token_index) {
        if ((argv[token_index][0] == '-') && strncmp(argv[token_index], CHANNEL_NUMBER_TOKEN, len) && !is_negative_number(argv[token_index])) {
                cmd_copy[cmd_token_cnt++] = argv[token_index];
        }
    }

    /***************************************************************************************************/
    /****************  Find configuration files tokens and call respective functions  ******************/
    /***************************************************************************************************/

    // Find the Config File Path in the command line
    if (FindTokenMultipleInputs(argc, argv, CONFIG_FILE_TOKEN, config_strings) == 0) {

        mark_token_as_read(CONFIG_FILE_TOKEN, cmd_copy, &cmd_token_cnt);
        // Parse the config file
        for (index = 0; index < numChannels; ++index){
            return_errors[index] = (EB_ERRORTYPE)ReadConfigFile(configs[index], config_strings[index], index);
            return_error = (EB_ERRORTYPE)(return_error &  return_errors[index]);
        }
    }
    else {
        if (FindToken(argc, argv, CONFIG_FILE_TOKEN, config_string) == 0) {
            printf("Error: Config File Token Not Found\n");
            return EB_ErrorBadParameter;
        }
        else {
            return_error = EB_ErrorNone;
        }
    }


    /***************************************************************************************************/
    /***********   Find SINGLE_INPUT configuration parameter tokens and call respective functions  **********/
    /***************************************************************************************************/
    token_index = -1;
    // Parse command line for tokens
    while (config_entry[++token_index].name != NULL){
        if (config_entry[token_index].type == SINGLE_INPUT){

            if (FindTokenMultipleInputs(argc, argv, config_entry[token_index].token, config_strings) == 0) {

                // When a token is found mark it as found in the temp token buffer
                mark_token_as_read(config_entry[token_index].token, cmd_copy, &cmd_token_cnt);

                // Fill up the values corresponding to each channel
                for (index = 0; index < numChannels; ++index) {
                    if (EB_STRCMP(config_strings[index], " ")) {
                        (*config_entry[token_index].scf)(config_strings[index], configs[index]);
                    }
                    else {
                        break;
                    }
                }
            }
        }
    }

    /***************************************************************************************************/
    /***********   Find SPECIAL configuration parameter tokens and call respective functions  **********/
    /***************************************************************************************************/
    // Parse command line for search region at level 0 width token
    if (FindTokenMultipleInputs(argc, argv, HME_LEVEL0_WIDTH, config_strings) == 0) {
        unsigned int inputIndex = 0, lastIndex = 0;
        unsigned int done = 1;

        mark_token_as_read(HME_LEVEL0_WIDTH, cmd_copy, &cmd_token_cnt);

        for (index = 0; done && (index < numChannels); ++index){
            configs[index]->hmeLevel0ColumnIndex = 0;
            for (inputIndex = lastIndex; inputIndex < configs[index]->numberHmeSearchRegionInWidth + lastIndex; ++inputIndex){
                if (EB_STRCMP(config_strings[inputIndex], " "))
                    SetHmeLevel0SearchAreaInWidthArray(config_strings[inputIndex], configs[index]);
                else{
                    done = 0;
                    break;
                }
            }
            lastIndex += configs[index]->numberHmeSearchRegionInWidth;
        }
    }


    //// Parse command line for search region at level 0 height token
    if (FindTokenMultipleInputs(argc, argv, HME_LEVEL0_HEIGHT, config_strings) == 0) {

        unsigned int inputIndex = 0, lastIndex = 0;
        unsigned int done = 1;

        mark_token_as_read(HME_LEVEL0_HEIGHT, cmd_copy, &cmd_token_cnt);

        for (index = 0; done && (index < numChannels); ++index){
            configs[index]->hmeLevel0RowIndex = 0;
            for (inputIndex = lastIndex; inputIndex < configs[index]->numberHmeSearchRegionInHeight + lastIndex; ++inputIndex){
                if (EB_STRCMP(config_strings[inputIndex], " "))
                    SetHmeLevel0SearchAreaInHeightArray(config_strings[inputIndex], configs[index]);
                else{
                    done = 0;
                    break;
                }
            }
            lastIndex += configs[index]->numberHmeSearchRegionInHeight;
        }
    }

    // Parse command line for search region at level 1 Height token
    if (FindTokenMultipleInputs(argc, argv, HME_LEVEL1_HEIGHT, config_strings) == 0) {

        unsigned int inputIndex = 0, lastIndex = 0;
        unsigned int done = 1;

        mark_token_as_read(HME_LEVEL1_HEIGHT, cmd_copy, &cmd_token_cnt);

        for (index = 0; done && (index < numChannels); ++index){
            configs[index]->hmeLevel1RowIndex = 0;
            for (inputIndex = lastIndex; inputIndex < configs[index]->numberHmeSearchRegionInHeight + lastIndex; ++inputIndex){
                if (EB_STRCMP(config_strings[inputIndex], " "))
                    SetHmeLevel1SearchAreaInHeightArray(config_strings[inputIndex], configs[index]);
                else{
                    done = 0;
                    break;
                }
            }
            lastIndex += configs[index]->numberHmeSearchRegionInHeight;
        }
    }

    // Parse command line for search region at level 1 width token
    if (FindTokenMultipleInputs(argc, argv, HME_LEVEL1_WIDTH, config_strings) == 0) {

        unsigned int inputIndex = 0, lastIndex = 0;
        unsigned int done = 1;

        mark_token_as_read(HME_LEVEL1_WIDTH, cmd_copy, &cmd_token_cnt);

        for (index = 0; done && (index < numChannels); ++index){
            configs[index]->hmeLevel1ColumnIndex = 0;
            for (inputIndex = lastIndex; inputIndex < configs[index]->numberHmeSearchRegionInWidth + lastIndex; ++inputIndex){
                if (EB_STRCMP(config_strings[inputIndex], " "))
                    SetHmeLevel1SearchAreaInWidthArray(config_strings[inputIndex], configs[index]);
                else{
                    done = 0;
                    break;
                }
            }
            lastIndex += configs[index]->numberHmeSearchRegionInWidth;
        }
    }

    // Parse command line for search region at level 2 width token
    if (FindTokenMultipleInputs(argc, argv, HME_LEVEL2_WIDTH, config_strings) == 0) {

        unsigned int inputIndex = 0, lastIndex = 0;
        unsigned int done = 1;

        mark_token_as_read(HME_LEVEL2_WIDTH, cmd_copy, &cmd_token_cnt);

        for (index = 0; done && (index < numChannels); ++index){
            configs[index]->hmeLevel2ColumnIndex = 0;
            for (inputIndex = lastIndex; inputIndex < configs[index]->numberHmeSearchRegionInWidth + lastIndex; ++inputIndex){
                if (EB_STRCMP(config_strings[inputIndex], " "))
                    SetHmeLevel2SearchAreaInWidthArray(config_strings[inputIndex], configs[index]);
                else{
                    done = 0;
                    break;
                }
            }
            lastIndex += configs[index]->numberHmeSearchRegionInWidth;
        }
    }

    // Parse command line for search region at level 2 height token
    if (FindTokenMultipleInputs(argc, argv, HME_LEVEL2_HEIGHT, config_strings) == 0) {

        unsigned int inputIndex = 0, lastIndex = 0;
        unsigned int done = 1;

        mark_token_as_read(HME_LEVEL2_HEIGHT, cmd_copy, &cmd_token_cnt);

        for (index = 0; done && (index < numChannels); ++index){
            configs[index]->hmeLevel2RowIndex = 0;
            for (inputIndex = lastIndex; inputIndex < configs[index]->numberHmeSearchRegionInHeight + lastIndex; ++inputIndex){
                if (EB_STRCMP(config_strings[inputIndex], " "))
                    SetHmeLevel2SearchAreaInHeightArray(config_strings[inputIndex], configs[index]);
                else{
                    done = 0;
                    break;
                }
            }
            lastIndex += configs[index]->numberHmeSearchRegionInHeight;
        }
    }
    
    /***************************************************************************************************/
    /**************************************   Verify configuration parameters   ************************/
    /***************************************************************************************************/
    // Verify the config values
    if (return_error == 0) {
        return_error = EB_ErrorBadParameter;
        for (index = 0; index < numChannels; ++index){
            if (return_errors[index] == EB_ErrorNone){
                return_errors[index] = VerifySettings(configs[index], index);

                // Assuming no errors, add padding to width and height
                if (return_errors[index] == EB_ErrorNone) {
                    configs[index]->inputPaddedWidth  = configs[index]->sourceWidth + LEFT_INPUT_PADDING + RIGHT_INPUT_PADDING;
                    configs[index]->inputPaddedHeight = configs[index]->sourceHeight + TOP_INPUT_PADDING + BOTTOM_INPUT_PADDING;
                }


                // Assuming no errors, set the frames to be encoded to the number of frames in the input yuv
                if (return_errors[index] == EB_ErrorNone && configs[index]->framesToBeEncoded == 0)
                    configs[index]->framesToBeEncoded = ComputeFramesToBeEncoded(configs[index]);

                if (configs[index]->framesToBeEncoded == -1) {
                    fprintf(configs[index]->errorLogFile, "Error instance %u: Input yuv does not contain enough frames \n", index + 1);
                    return_errors[index] = EB_ErrorBadParameter;                
                }

                // Force the injector latency mode, and injector frame rate when speed control is on
                if (return_errors[index] == EB_ErrorNone && configs[index]->speedControlFlag == 1) {
                    configs[index]->injector    = 1;
                }

            }
            return_error = (EB_ERRORTYPE)(return_error & return_errors[index]);
        }
    }

    // Print message for unprocessed tokens
    if (cmd_token_cnt > 0) {
        EB_S32 cmd_copy_index;
        printf("Unprocessed tokens: ");
        for (cmd_copy_index = 0; cmd_copy_index < cmd_token_cnt; ++cmd_copy_index) {
            printf(" %s ", cmd_copy[cmd_copy_index]);
        }
        printf("\n\n");
        return_error = EB_ErrorBadParameter;
    }

    for (index = 0; index < MAX_CHANNEL_NUMBER; ++index){
        free(config_strings[index]);
    }

    return return_error;
}