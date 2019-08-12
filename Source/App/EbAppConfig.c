/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "EbAppConfig.h"
#include "EbApi.h"
#include "EbAppInputy4m.h"

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
#define TILE_ROW_COUNT_TOKEN            "-tile_row_cnt"
#define TILE_COL_COUNT_TOKEN            "-tile_col_cnt"
#define TILE_SLICE_MODE_TOKEN           "-tile_slice_mode"
#define TUNE_TOKEN                      "-tune"
#define FRAME_RATE_TOKEN                "-fps"
#define FRAME_RATE_NUMERATOR_TOKEN      "-fps-num"
#define FRAME_RATE_DENOMINATOR_TOKEN    "-fps-denom"
#define ENCODER_BIT_DEPTH               "-bit-depth"
#define ENCODER_COLOR_FORMAT            "-color-format"
#define INPUT_COMPRESSED_TEN_BIT_FORMAT "-compressed-ten-bit-format"
#define ENCMODE_TOKEN                   "-encMode"
#define HIERARCHICAL_LEVELS_TOKEN       "-hierarchical-levels" // no Eval
#define PRED_STRUCT_TOKEN               "-pred-struct"
#define INTRA_PERIOD_TOKEN              "-intra-period"
#define PROFILE_TOKEN                   "-profile"
#define TIER_TOKEN                      "-tier"
#define LEVEL_TOKEN                     "-level"
//#define LATENCY_MODE                    "-latency-mode" // no Eval
#define INTERLACED_VIDEO_TOKEN          "-interlaced-video"
#define SEPERATE_FILDS_TOKEN            "-separate-fields"
#define INTRA_REFRESH_TYPE_TOKEN        "-irefresh-type" // no Eval
#define LOOP_FILTER_DISABLE_TOKEN       "-dlf"
#define SAO_ENABLE_TOKEN                "-sao"
#define USE_DEFAULT_ME_HME_TOKEN        "-use-default-me-hme"
#define HME_ENABLE_TOKEN                "-hme"      // no Eval
#define SEARCH_AREA_WIDTH_TOKEN         "-search-w" // no Eval
#define SEARCH_AREA_HEIGHT_TOKEN        "-search-h" // no Eval
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
#define MAXCLL_TOKEN                    "-max-cll"
#define MAXFALL_TOKEN                   "-max-fall"
#define USE_MASTER_DISPLAY_TOKEN        "-use-master-display"
#define MASTER_DISPLAY_TOKEN            "-master-display"
#define DOLBY_VISION_PROFILE_TOKEN      "-dolby-vision-profile"
#define DOLBY_VISION_RPU_FILE_TOKEN     "-dolby-vision-rpu"
#define NALU_FILE_TOKEN                 "-nalu-file"
#define RATE_CONTROL_ENABLE_TOKEN       "-rc"
#define TARGET_BIT_RATE_TOKEN           "-tbr"
#define VBV_MAX_RATE_TOKEN              "-vbv-maxrate"
#define VBV_BUFFER_SIZE_TOKEN           "-vbv-bufsize"
#define VBV_BUFFER_INIT_TOKEN           "-vbv-init"
#define HRD_TOKEN                       "-hrd"
#define MAX_QP_TOKEN                    "-max-qp"
#define MIN_QP_TOKEN                    "-min-qp"
#define TEMPORAL_ID					    "-temporal-id" // no Eval
#define LOOK_AHEAD_DIST_TOKEN           "-lad"
#define SCENE_CHANGE_DETECTION_TOKEN    "-scd"
#define INJECTOR_TOKEN                  "-inj"  // no Eval
#define INJECTOR_FRAMERATE_TOKEN        "-inj-frm-rt" // no Eval
#define SPEED_CONTROL_TOKEN             "-speed-ctrl"
#define ASM_TYPE_TOKEN				    "-asm" // no Eval
#define THREAD_MGMNT                    "-lp"
#define TARGET_SOCKET                   "-ss"
#define SWITCHTHREADSTOREALTIME_TOKEN   "-rt"
#define FPSINVPS_TOKEN                  "-fpsinvps"
#define UNRESTRICTED_MOTION_VECTOR      "-umv"
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
        /* if input is a YUV4MPEG2 (y4m) file, read header and parse parameters */
        if (cfg->inputFile != NULL) {
            if ((EB_BOOL)(check_if_y4m(cfg)) == EB_TRUE)
                cfg->y4m_input = EB_TRUE;
            else
                cfg->y4m_input = EB_FALSE;
        }
        else {
            cfg->y4m_input = EB_FALSE;
        }
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
static void SetCfgReconFile                     (const char *value, EbConfig_t *cfg)
{
    if (cfg->reconFile) { fclose(cfg->reconFile); }
    FOPEN(cfg->reconFile,value, "wb");
};
static void SetCfgQpFile                        (const char *value, EbConfig_t *cfg)
{
    if (cfg->qpFile) { fclose(cfg->qpFile); }
    FOPEN(cfg->qpFile,value, "r");
};
static void SetCfgDolbyVisionRpuFile			(const char *value, EbConfig_t *cfg)
{
    if (cfg->dolbyVisionRpuFile) { fclose(cfg->dolbyVisionRpuFile); }
    FOPEN(cfg->dolbyVisionRpuFile, value, "rb");
};
static void SetNaluFile(const char *value, EbConfig_t *cfg)
{
    if (cfg->naluFile) { fclose(cfg->naluFile); }
    FOPEN(cfg->naluFile, value, "rb");
    if (cfg->naluFile)
        cfg->useNaluFile = EB_TRUE;
    else
        printf("Error: Nalu file: %s does not exist, won't use\n", value);
};
static void SetCfgSourceWidth                   (const char *value, EbConfig_t *cfg) {cfg->sourceWidth                      = strtoul(value, NULL, 0);};
static void SetInterlacedVideo                  (const char *value, EbConfig_t *cfg) {cfg->interlacedVideo                  = (EB_BOOL) strtoul(value, NULL, 0);};
static void SetSeperateFields                   (const char *value, EbConfig_t *cfg) {cfg->separateFields                   = (EB_BOOL) strtoul(value, NULL, 0);};
static void SetCfgSourceHeight                  (const char *value, EbConfig_t *cfg) {cfg->sourceHeight                     = strtoul(value, NULL, 0) >> cfg->separateFields;};
static void SetCfgFramesToBeEncoded             (const char *value, EbConfig_t *cfg) {cfg->framesToBeEncoded                = strtoll(value,  NULL, 0) << cfg->separateFields;};
static void SetBufferedInput                    (const char *value, EbConfig_t *cfg) {cfg->bufferedInput                    = (strtol(value, NULL, 0) != -1 && cfg->separateFields) ? strtol(value, NULL, 0) << cfg->separateFields : strtol(value, NULL, 0);};
static void SetFrameRate                        (const char *value, EbConfig_t *cfg) {
    cfg->frameRate = strtoul(value, NULL, 0);
    if (cfg->frameRate <= 1000 ){
        cfg->frameRate = cfg->frameRate << 16;
    }
}
static void SetFrameRateNumerator               (const char *value, EbConfig_t *cfg) {cfg->frameRateNumerator               = strtoul(value, NULL, 0);};
static void SetFrameRateDenominator             (const char *value, EbConfig_t *cfg) {cfg->frameRateDenominator             = strtoul(value, NULL, 0);};
static void SetEncoderBitDepth                  (const char *value, EbConfig_t *cfg) {cfg->encoderBitDepth                  = strtoul(value, NULL, 0);}
static void SetEncoderColorFormat               (const char *value, EbConfig_t *cfg) {cfg->encoderColorFormat               = strtoul(value, NULL, 0);}
static void SetcompressedTenBitFormat           (const char *value, EbConfig_t *cfg) {cfg->compressedTenBitFormat           = strtoul(value, NULL, 0);}
static void SetBaseLayerSwitchMode              (const char *value, EbConfig_t *cfg) {cfg->baseLayerSwitchMode              = (EB_BOOL) strtoul(value, NULL, 0);};
static void SetencMode                          (const char *value, EbConfig_t *cfg) {cfg->encMode                          = (uint8_t)strtoul(value, NULL, 0);};
static void SetCfgIntraPeriod                   (const char *value, EbConfig_t *cfg) {cfg->intraPeriod                      = strtol(value,  NULL, 0);};
static void SetCfgIntraRefreshType              (const char *value, EbConfig_t *cfg) {cfg->intraRefreshType                 = strtol(value,  NULL, 0);};
static void SetHierarchicalLevels               (const char *value, EbConfig_t *cfg) {cfg->hierarchicalLevels               = strtol(value, NULL, 0); };
static void SetCfgPredStructure                 (const char *value, EbConfig_t *cfg) {cfg->predStructure                    = strtol(value, NULL, 0); };
static void SetCfgQp                            (const char *value, EbConfig_t *cfg) {cfg->qp                               = strtoul(value, NULL, 0);};
static void SetCfgUseQpFile                     (const char *value, EbConfig_t *cfg) {cfg->useQpFile                        = (EB_BOOL)strtol(value, NULL, 0); };
static void SetCfgTileColumnCount               (const char *value, EbConfig_t *cfg) { cfg->tileColumnCount                 = (EB_BOOL)strtol(value, NULL, 0); };
static void SetCfgTileRowCount                  (const char *value, EbConfig_t *cfg) { cfg->tileRowCount                    = (EB_BOOL)strtol(value, NULL, 0); };
static void SetCfgTileSliceMode                 (const char *value, EbConfig_t *cfg) { cfg->tileSliceMode                   = (EB_BOOL)strtol(value, NULL, 0); };
static void SetDisableDlfFlag                   (const char *value, EbConfig_t *cfg) {cfg->disableDlfFlag                   = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetEnableSaoFlag                    (const char *value, EbConfig_t *cfg) {cfg->enableSaoFlag                    = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetEnableHmeFlag                    (const char *value, EbConfig_t *cfg) {cfg->enableHmeFlag                    = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetSceneChangeDetection             (const char *value, EbConfig_t *cfg) {cfg->sceneChangeDetection             = strtoul(value, NULL, 0);};
static void SetLookAheadDistance                (const char *value, EbConfig_t *cfg) {cfg->lookAheadDistance                = strtoul(value, NULL, 0);};
static void SetRateControlMode                  (const char *value, EbConfig_t *cfg) {cfg->rateControlMode                  = strtoul(value, NULL, 0);};
static void SetTargetBitRate                    (const char *value, EbConfig_t *cfg) {cfg->targetBitRate                    = strtoul(value, NULL, 0);};
static void SetMaxQpAllowed                     (const char *value, EbConfig_t *cfg) {cfg->maxQpAllowed                     = strtoul(value, NULL, 0);};
static void SetMinQpAllowed                     (const char *value, EbConfig_t *cfg) {cfg->minQpAllowed                     = strtoul(value, NULL, 0);};
static void SetCfgSearchAreaWidth               (const char *value, EbConfig_t *cfg) {cfg->searchAreaWidth                  = strtoul(value, NULL, 0);};
static void SetCfgSearchAreaHeight              (const char *value, EbConfig_t *cfg) {cfg->searchAreaHeight                 = strtoul(value, NULL, 0);};
static void SetCfgUseDefaultMeHme               (const char *value, EbConfig_t *cfg) {cfg->useDefaultMeHme                  = (EB_BOOL)strtol(value, NULL, 0); };
static void SetEnableConstrainedIntra           (const char *value, EbConfig_t *cfg) {cfg->constrainedIntra                 = (EB_BOOL)strtoul(value, NULL, 0);};
static void SetCfgTune                          (const char *value, EbConfig_t *cfg) {cfg->tune                             = (uint8_t)strtoul(value, NULL, 0); };
static void SetBitRateReduction                 (const char *value, EbConfig_t *cfg) {cfg->bitRateReduction                 = (EB_BOOL)strtol(value, NULL, 0); };
static void SetImproveSharpness                 (const char *value, EbConfig_t *cfg) {cfg->improveSharpness                 = (EB_BOOL)strtol(value,  NULL, 0);};
static void SetVbvMaxrate                       (const char *value, EbConfig_t *cfg) { cfg->vbvMaxRate						= strtoul(value, NULL, 0);};
static void SetVbvBufsize                       (const char *value, EbConfig_t *cfg) { cfg->vbvBufsize						= strtoul(value, NULL, 0);};
static void SetVbvBufInit                       (const char *value, EbConfig_t *cfg) { cfg->vbvBufInit						= strtoul(value, NULL, 0);};
static void SetHrdFlag                          (const char *value, EbConfig_t *cfg) { cfg->hrdFlag							= strtoul(value, NULL, 0);};
static void SetVideoUsabilityInfo               (const char *value, EbConfig_t *cfg) {cfg->videoUsabilityInfo               = strtol(value,  NULL, 0);};
static void SetHighDynamicRangeInput            (const char *value, EbConfig_t *cfg) {cfg->highDynamicRangeInput            = strtol(value,  NULL, 0);};
static void SetAccessUnitDelimiter              (const char *value, EbConfig_t *cfg) {cfg->accessUnitDelimiter              = strtol(value,  NULL, 0);};
static void SetBufferingPeriodSEI               (const char *value, EbConfig_t *cfg) {cfg->bufferingPeriodSEI               = strtol(value,  NULL, 0);};
static void SetPictureTimingSEI                 (const char *value, EbConfig_t *cfg) {cfg->pictureTimingSEI                 = strtol(value,  NULL, 0);};
static void SetRegisteredUserDataSEI            (const char *value, EbConfig_t *cfg) {cfg->registeredUserDataSeiFlag        = (EB_BOOL)strtol(value,  NULL, 0);};
static void SetUnRegisteredUserDataSEI          (const char *value, EbConfig_t *cfg) {cfg->unregisteredUserDataSeiFlag      = (EB_BOOL)strtol(value,  NULL, 0);};
static void SetRecoveryPointSEI                 (const char *value, EbConfig_t *cfg) {cfg->recoveryPointSeiFlag             = (EB_BOOL)strtol(value,  NULL, 0);};
static void SetMaxCLL                           (const char *value, EbConfig_t *cfg) {cfg->maxCLL                           = (uint16_t)strtoul(value, NULL, 0);};
static void SetMaxFALL                          (const char *value, EbConfig_t *cfg) {cfg->maxFALL                          = (uint16_t)strtoul(value, NULL, 0);};
static void SetMasterDisplayFlag                (const char *value, EbConfig_t *cfg) {cfg->useMasteringDisplayColorVolume   = (EB_BOOL)strtol(value, NULL, 0);};
static void SetMasterDisplay                    (const char *value, EbConfig_t *cfg) {
    if (cfg->useMasteringDisplayColorVolume)
        EB_STRCPY(cfg->masteringDisplayColorVolumeString, EB_STRLEN(value, MAX_STRING_LENGTH) + 1, value);
};
static void SetDolbyVisionProfile               (const char *value, EbConfig_t *cfg) {
    if (strtoul(value, NULL, 0) != 0 || EB_STRCMP(value, "0") == 0)
        cfg->dolbyVisionProfile = (uint32_t)(10 * strtod(value, NULL));
};
static void SetEnableTemporalId                 (const char *value, EbConfig_t *cfg) {cfg->enableTemporalId                 = strtol(value,  NULL, 0);};
static void SetProfile                          (const char *value, EbConfig_t *cfg) {cfg->profile                          = strtol(value,  NULL, 0);};
static void SetTier                             (const char *value, EbConfig_t *cfg) {cfg->tier                             = strtol(value,  NULL, 0);};
static void SetLevel                            (const char *value, EbConfig_t *cfg) {
    if (strtoul( value, NULL,0) != 0 || EB_STRCMP(value, "0") == 0 )
        cfg->level = (uint32_t)(10*strtod(value,  NULL));
    else
        cfg->level = 9999999;
};
static void SetInjector                         (const char *value, EbConfig_t *cfg) {cfg->injector                         = strtol(value,  NULL, 0);};
static void SpeedControlFlag                    (const char *value, EbConfig_t *cfg) {cfg->speedControlFlag                 = strtol(value, NULL, 0); };
static void SetInjectorFrameRate                (const char *value, EbConfig_t *cfg) {
    cfg->injectorFrameRate = strtoul(value, NULL, 0);
    if (cfg->injectorFrameRate <= 1000 ){
        cfg->injectorFrameRate = cfg->injectorFrameRate << 16;
    }
}
//static void SetLatencyMode                      (const char *value, EbConfig_t *cfg)  {cfg->latencyMode                     = (uint8_t)strtol(value, NULL, 0);};
static void SetAsmType                          (const char *value, EbConfig_t *cfg)  {cfg->asmType                         = (uint32_t)strtoul(value, NULL, 0); };
static void SetLogicalProcessors                (const char *value, EbConfig_t *cfg)  {cfg->logicalProcessors               = (uint32_t)strtoul(value, NULL, 0);};
static void SetTargetSocket                     (const char *value, EbConfig_t *cfg)  {cfg->targetSocket                    = (int32_t)strtol(value, NULL, 0);};
static void SetSwitchThreadsToRtPriority        (const char *value, EbConfig_t *cfg)  {cfg->switchThreadsToRtPriority       = (EB_BOOL)strtol(value, NULL, 0);};
static void SetFpsInVps                         (const char *value, EbConfig_t *cfg)  {cfg->fpsInVps                        = (EB_BOOL)strtol(value, NULL, 0);};
static void SetUnrestrictedMotionVector         (const char *value, EbConfig_t *cfg)  {cfg->unrestrictedMotionVector        = (EB_BOOL)strtol(value, NULL, 0);};

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
    { SINGLE_INPUT, OUTPUT_BITSTREAM_TOKEN, "StreamFile", SetCfgStreamFile },
    { SINGLE_INPUT, ERROR_FILE_TOKEN, "ErrorFile", SetCfgErrorFile },
    { SINGLE_INPUT, OUTPUT_RECON_TOKEN, "ReconFile", SetCfgReconFile },
    { SINGLE_INPUT, USE_QP_FILE_TOKEN, "UseQpFile", SetCfgUseQpFile },
    { SINGLE_INPUT, QP_FILE_TOKEN, "QpFile", SetCfgQpFile },

    // Interlaced Video
    { SINGLE_INPUT, INTERLACED_VIDEO_TOKEN, "InterlacedVideo", SetInterlacedVideo },
    // Do NOT move, the value is used in other entries
    { SINGLE_INPUT, SEPERATE_FILDS_TOKEN, "SeperateFields", SetSeperateFields },

     { SINGLE_INPUT, TILE_ROW_COUNT_TOKEN, "TileRowCount", SetCfgTileRowCount },
     { SINGLE_INPUT, TILE_COL_COUNT_TOKEN, "TileColumnCount", SetCfgTileColumnCount },
     { SINGLE_INPUT, TILE_SLICE_MODE_TOKEN, "TileSliceMode", SetCfgTileSliceMode },

    // Encoding Presets
    { SINGLE_INPUT, ENCMODE_TOKEN, "EncoderMode", SetencMode },
//    { SINGLE_INPUT, LATENCY_MODE, "LatencyMode", SetLatencyMode },
    { SINGLE_INPUT, SPEED_CONTROL_TOKEN, "SpeedControlFlag", SpeedControlFlag },

    // Bit-depth
    { SINGLE_INPUT, ENCODER_BIT_DEPTH, "EncoderBitDepth", SetEncoderBitDepth },
    { SINGLE_INPUT, INPUT_COMPRESSED_TEN_BIT_FORMAT, "CompressedTenBitFormat", SetcompressedTenBitFormat },
    { SINGLE_INPUT, ENCODER_COLOR_FORMAT, "EncoderColorFormat", SetEncoderColorFormat },

    // Source Definitions
    { SINGLE_INPUT, WIDTH_TOKEN, "SourceWidth", SetCfgSourceWidth },
    { SINGLE_INPUT, HEIGHT_TOKEN, "SourceHeight", SetCfgSourceHeight },
    { SINGLE_INPUT, NUMBER_OF_PICTURES_TOKEN, "FrameToBeEncoded", SetCfgFramesToBeEncoded },
    { SINGLE_INPUT, BUFFERED_INPUT_TOKEN, "BufferedInput", SetBufferedInput },

    // Annex A parameters
    { SINGLE_INPUT, PROFILE_TOKEN, "Profile", SetProfile },
    { SINGLE_INPUT, TIER_TOKEN, "Tier", SetTier },
    { SINGLE_INPUT, LEVEL_TOKEN, "Level", SetLevel },

    // Frame Rate
    { SINGLE_INPUT, FRAME_RATE_TOKEN, "FrameRate", SetFrameRate },
    { SINGLE_INPUT, FRAME_RATE_NUMERATOR_TOKEN, "FrameRateNumerator", SetFrameRateNumerator },
    { SINGLE_INPUT, FRAME_RATE_DENOMINATOR_TOKEN, "FrameRateDenominator", SetFrameRateDenominator },

    // Coding Structure
    { SINGLE_INPUT, HIERARCHICAL_LEVELS_TOKEN, "HierarchicalLevels", SetHierarchicalLevels },
    { SINGLE_INPUT, BASE_LAYER_SWITCH_MODE_TOKEN, "BaseLayerSwitchMode", SetBaseLayerSwitchMode },
    { SINGLE_INPUT, PRED_STRUCT_TOKEN, "PredStructure", SetCfgPredStructure },
    { SINGLE_INPUT, INTRA_PERIOD_TOKEN, "IntraPeriod", SetCfgIntraPeriod },
    { SINGLE_INPUT, INTRA_REFRESH_TYPE_TOKEN, "IntraRefreshType", SetCfgIntraRefreshType },

    // Quantization
    { SINGLE_INPUT, QP_TOKEN, "QP", SetCfgQp },
    { SINGLE_INPUT, VBV_MAX_RATE_TOKEN, "vbvMaxRate", SetVbvMaxrate },
    { SINGLE_INPUT, VBV_BUFFER_SIZE_TOKEN, "vbvBufsize", SetVbvBufsize },
    { SINGLE_INPUT, HRD_TOKEN, "hrd", SetHrdFlag },
    { SINGLE_INPUT, VBV_BUFFER_INIT_TOKEN, "vbvBufInit", SetVbvBufInit},


    // Deblock Filter
    { SINGLE_INPUT, LOOP_FILTER_DISABLE_TOKEN, "LoopFilterDisable", SetDisableDlfFlag },

    // Sample Adaptive Offset
    { SINGLE_INPUT, SAO_ENABLE_TOKEN, "SAO", SetEnableSaoFlag },

    // Me Tools
    { SINGLE_INPUT, USE_DEFAULT_ME_HME_TOKEN, "UseDefaultMeHme", SetCfgUseDefaultMeHme },
    { SINGLE_INPUT, HME_ENABLE_TOKEN, "HME", SetEnableHmeFlag },

    // Me Parameters
    { SINGLE_INPUT, SEARCH_AREA_WIDTH_TOKEN, "SearchAreaWidth", SetCfgSearchAreaWidth },
    { SINGLE_INPUT, SEARCH_AREA_HEIGHT_TOKEN, "SearchAreaHeight", SetCfgSearchAreaHeight },

    // MD Parameters
    { SINGLE_INPUT, CONSTRAINED_INTRA_ENABLE_TOKEN, "ConstrainedIntra", SetEnableConstrainedIntra },

    // Rate Control
	{ SINGLE_INPUT, RATE_CONTROL_ENABLE_TOKEN, "RateControlMode", SetRateControlMode },
    { SINGLE_INPUT, TARGET_BIT_RATE_TOKEN, "TargetBitRate", SetTargetBitRate },
    { SINGLE_INPUT, MAX_QP_TOKEN, "MaxQpAllowed", SetMaxQpAllowed },
    { SINGLE_INPUT, MIN_QP_TOKEN, "MinQpAllowed", SetMinQpAllowed },
    { SINGLE_INPUT, LOOK_AHEAD_DIST_TOKEN, "LookAheadDistance", SetLookAheadDistance },
    { SINGLE_INPUT, SCENE_CHANGE_DETECTION_TOKEN, "SceneChangeDetection", SetSceneChangeDetection },

    // Tune
    { SINGLE_INPUT, TUNE_TOKEN, "Tune", SetCfgTune },

    // Adaptive QP Params
    { SINGLE_INPUT, BITRATE_REDUCTION_TOKEN, "BitRateReduction", SetBitRateReduction },
    { SINGLE_INPUT, IMPROVE_SHARPNESS_TOKEN,"ImproveSharpness", SetImproveSharpness },

    // Optional Features
    { SINGLE_INPUT, VIDEO_USE_INFO_TOKEN, "VideoUsabilityInfo", SetVideoUsabilityInfo },
    { SINGLE_INPUT, HDR_INPUT_TOKEN, "HighDynamicRangeInput", SetHighDynamicRangeInput },
    { SINGLE_INPUT, ACCESS_UNIT_DELM_TOKEN, "AccessUnitDelimiter", SetAccessUnitDelimiter },
    { SINGLE_INPUT, BUFF_PERIOD_TOKEN, "BufferingPeriod", SetBufferingPeriodSEI },
    { SINGLE_INPUT, PIC_TIMING_TOKEN, "PictureTiming", SetPictureTimingSEI },
    { SINGLE_INPUT, REG_USER_DATA_TOKEN, "RegisteredUserData", SetRegisteredUserDataSEI },
    { SINGLE_INPUT, UNREG_USER_DATA_TOKEN, "UnregisteredUserData", SetUnRegisteredUserDataSEI },
    { SINGLE_INPUT, RECOVERY_POINT_TOKEN, "RecoveryPoint", SetRecoveryPointSEI },
    { SINGLE_INPUT, TEMPORAL_ID, "TemporalId", SetEnableTemporalId },
    { SINGLE_INPUT, SWITCHTHREADSTOREALTIME_TOKEN, "SwitchThreadsToRtPriority", SetSwitchThreadsToRtPriority },
    { SINGLE_INPUT, FPSINVPS_TOKEN, "FPSInVPS", SetFpsInVps },
    { SINGLE_INPUT, MAXCLL_TOKEN, "MaxCLL", SetMaxCLL },
    { SINGLE_INPUT, MAXFALL_TOKEN, "MaxFALL", SetMaxFALL },
    { SINGLE_INPUT, USE_MASTER_DISPLAY_TOKEN, "UseMasterDisplay", SetMasterDisplayFlag },
    { SINGLE_INPUT, MASTER_DISPLAY_TOKEN, "MasterDisplay", SetMasterDisplay },
    { SINGLE_INPUT, DOLBY_VISION_PROFILE_TOKEN, "DolbyVisionProfile", SetDolbyVisionProfile },
    { SINGLE_INPUT, DOLBY_VISION_RPU_FILE_TOKEN, "DolbyVisionRpuFile", SetCfgDolbyVisionRpuFile },
    { SINGLE_INPUT, NALU_FILE_TOKEN, "NaluFile", SetNaluFile },
    { SINGLE_INPUT, TEMPORAL_ID, "TemporalId", SetEnableTemporalId },
    { SINGLE_INPUT, FPSINVPS_TOKEN, "FPSInVPS", SetFpsInVps },
    { SINGLE_INPUT, UNRESTRICTED_MOTION_VECTOR, "UnrestrictedMotionVector", SetUnrestrictedMotionVector },

    // Latency
    { SINGLE_INPUT, INJECTOR_TOKEN, "Injector", SetInjector },
    { SINGLE_INPUT, INJECTOR_FRAMERATE_TOKEN, "InjectorFrameRate", SetInjectorFrameRate },
    { SINGLE_INPUT, SPEED_CONTROL_TOKEN, "SpeedControlFlag", SpeedControlFlag },

    // Annex A parameters
    { SINGLE_INPUT, PROFILE_TOKEN, "Profile", SetProfile },
    { SINGLE_INPUT, TIER_TOKEN, "Tier", SetTier },
    { SINGLE_INPUT, LEVEL_TOKEN, "Level", SetLevel },
//    { SINGLE_INPUT, LATENCY_MODE, "LatencyMode", SetLatencyMode },

    // Platform Specific Flags
    { SINGLE_INPUT, ASM_TYPE_TOKEN, "AsmType", SetAsmType },
    { SINGLE_INPUT, TARGET_SOCKET, "TargetSocket", SetTargetSocket },
    { SINGLE_INPUT, THREAD_MGMNT, "LogicalProcessors", SetLogicalProcessors },

    // Termination
    { SINGLE_INPUT, NULL, NULL, NULL }
};

/**********************************
 * Constructor
 **********************************/
void EbConfigCtor(EbConfig_t *configPtr)
{
    // File I/O
    configPtr->configFile                           = NULL;
    configPtr->inputFile                            = NULL;
    configPtr->bitstreamFile                        = NULL;
    configPtr->errorLogFile                         = stderr;
    configPtr->reconFile                            = NULL;
    configPtr->bufferFile                           = NULL;
    configPtr->useQpFile                            = EB_FALSE;
    configPtr->qpFile                               = NULL;

    configPtr->y4m_input                            = EB_FALSE;

    configPtr->tileColumnCount                      = 1;
    configPtr->tileRowCount                         = 1;
    configPtr->tileSliceMode                        = 0;

    // SEI
    configPtr->maxCLL                               = 0;
    configPtr->maxFALL                              = 0;
    configPtr->useMasteringDisplayColorVolume       = EB_FALSE;
    configPtr->useNaluFile                          = EB_FALSE;
    configPtr->dolbyVisionProfile                   = 0;
    configPtr->dolbyVisionRpuFile                   = NULL;
    configPtr->naluFile                             = NULL;
    configPtr->displayPrimaryX[0]                   = 0;
    configPtr->displayPrimaryX[1]                   = 0;
    configPtr->displayPrimaryX[2]                   = 0;
    configPtr->displayPrimaryY[0]                   = 0;
    configPtr->displayPrimaryY[1]                   = 0;
    configPtr->displayPrimaryY[2]                   = 0;
    configPtr->whitePointX                          = 0;
    configPtr->whitePointY                          = 0;
    configPtr->maxDisplayMasteringLuminance         = 0;
    configPtr->minDisplayMasteringLuminance         = 0;

    // Encoding Presets
    configPtr->encMode                              = 7;
    //configPtr->latencyMode                          = 0; // Deprecated

    configPtr->speedControlFlag                     = 0;

    // Bit-depth
    configPtr->encoderBitDepth                      = 8;
    configPtr->compressedTenBitFormat               = 0;
    configPtr->encoderColorFormat                   = EB_YUV420;

    // Source Definitions
    configPtr->sourceWidth                          = 0;
    configPtr->sourceHeight                         = 0;
    configPtr->inputPaddedWidth                     = 0;
    configPtr->inputPaddedHeight                    = 0;
    configPtr->framesToBeEncoded                    = 0;
    configPtr->framesEncoded                        = 0;
    configPtr->bufferedInput                        = -1;
    configPtr->sequenceBuffer                       = 0;

    // Annex A Definitions
    configPtr->profile                              = 1;
    configPtr->tier                                 = 0;
    configPtr->level                                = 0;

    // Frame Rate
    configPtr->frameRate                            = 60;
    configPtr->frameRateNumerator                   = 0;
    configPtr->frameRateDenominator                 = 0;
    configPtr->injector                             = 0;
    configPtr->injectorFrameRate                    = 60 << 16;

    // Interlaced Video
    configPtr->interlacedVideo                      = EB_FALSE;
    configPtr->separateFields                       = EB_FALSE;

    // Coding Structure
    configPtr->hierarchicalLevels                   = 3;
    configPtr->baseLayerSwitchMode                  = 0;
    configPtr->predStructure                        = 2;
    configPtr->intraPeriod                          = -2;
    configPtr->intraRefreshType                     = 1;

    // DLF
    configPtr->disableDlfFlag                       = EB_FALSE;

    // Quantization
    configPtr->qp                                   = 32;

    // Sample Adaptive Offset
    configPtr->enableSaoFlag                        = EB_TRUE;

    // ME Tools
    configPtr->useDefaultMeHme                      = EB_TRUE;
    configPtr->enableHmeFlag                        = EB_TRUE;

    // ME Parameters
    configPtr->searchAreaWidth                      = 16;
    configPtr->searchAreaHeight                     = 7;

    // MD Parameters
    configPtr->constrainedIntra                     = EB_FALSE;

    // Rate Control
    configPtr->rateControlMode                      = 0;
    configPtr->targetBitRate                        = 7000000;
    configPtr->maxQpAllowed                         = 48;
    configPtr->minQpAllowed                         = 10;
    configPtr->lookAheadDistance                    = (uint32_t)~0;
    configPtr->sceneChangeDetection                 = 1;

    // Tune: only OQ
    configPtr->tune                                 = 1;

    // Adaptive QP Params
    configPtr->bitRateReduction                     = EB_FALSE;
    configPtr->improveSharpness                     = EB_FALSE;

    // Optional Features
    configPtr->videoUsabilityInfo                   = 0;
    configPtr->highDynamicRangeInput                = 0;
    configPtr->accessUnitDelimiter                  = 0;
    configPtr->bufferingPeriodSEI                   = 0;
    configPtr->pictureTimingSEI                     = 0;
    configPtr->registeredUserDataSeiFlag            = EB_FALSE;
    configPtr->unregisteredUserDataSeiFlag          = EB_FALSE;
    configPtr->recoveryPointSeiFlag                 = EB_FALSE;
    configPtr->enableTemporalId                     = 1;
    configPtr->switchThreadsToRtPriority            = EB_TRUE;
    configPtr->fpsInVps                             = EB_TRUE;
    configPtr->unrestrictedMotionVector             = EB_TRUE;

    // Platform Specific Flags
    configPtr->asmType                              = 1;
    configPtr->targetSocket                         = -1;
    configPtr->logicalProcessors                    = 0;

    // vbv
    configPtr->vbvMaxRate                           = 0;
    configPtr->vbvBufsize                           = 0;
    configPtr->vbvBufInit                           = 90;
    configPtr->hrdFlag                              = 0;

    // Testing
    configPtr->testUserData                         = 0;
    configPtr->eosFlag                              = EB_FALSE;

    // Computational Performance Parameters
    configPtr->performanceContext.libStartTime[0]   = 0;
    configPtr->performanceContext.libStartTime[1]   = 0;

    configPtr->performanceContext.encodeStartTime[0]= 0;
    configPtr->performanceContext.encodeStartTime[1]= 0;
    
    configPtr->performanceContext.totalExecutionTime= 0;
    configPtr->performanceContext.totalEncodeTime   = 0;
    configPtr->performanceContext.frameCount        = 0;
    configPtr->performanceContext.averageSpeed      = 0;
    configPtr->performanceContext.startsTime        = 0;
    configPtr->performanceContext.startuTime        = 0;
    configPtr->performanceContext.maxLatency        = 0;
    configPtr->performanceContext.totalLatency      = 0;
    configPtr->performanceContext.byteCount         = 0;

    configPtr->channelId                            = 0;
    configPtr->activeChannelCount                   = 0;
    configPtr->stopEncoder                          = EB_FALSE;
    configPtr->processedFrameCount                  = 0;
    configPtr->processedByteCount                   = 0;

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

    if (configPtr->naluFile) {
        fclose(configPtr->naluFile);
        configPtr->naluFile = (FILE *)NULL;
    }

    if (configPtr->dolbyVisionRpuFile) {
        fclose(configPtr->dolbyVisionRpuFile);
        configPtr->dolbyVisionRpuFile = (FILE *)NULL;
    }

    return;
}

/**********************************
 * File Size
 **********************************/
static int32_t findFileSize(
    FILE * const pFile)
{
    int32_t fileSize;

    fseek(pFile, 0, SEEK_END);
    fileSize = ftell(pFile);
    rewind(pFile);

    return fileSize;
}

/**********************************
 * Line Split
 **********************************/
static void lineSplit(
    uint32_t       *argc,
    char           *argv  [CONFIG_FILE_MAX_ARG_COUNT],
    uint32_t        argLen[CONFIG_FILE_MAX_ARG_COUNT],
    char           *linePtr)
{
    uint32_t i=0;
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
    char       *name,
    char       *value)
{
    int32_t i=0;

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
    int32_t         size)
{
    uint32_t argc;
    char *argv[CONFIG_FILE_MAX_ARG_COUNT];
    uint32_t argLen[CONFIG_FILE_MAX_ARG_COUNT];

    char varName[CONFIG_FILE_MAX_VAR_LEN];
    char varValue[CONFIG_FILE_MAX_ARG_COUNT][CONFIG_FILE_MAX_VAR_LEN];

    uint32_t valueIndex;

    uint32_t commentSectionFlag = 0;
    uint32_t newLineFlag = 0;

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
static int32_t FindToken(
    int32_t         argc,
    char * const    argv[],
    char const *    token,
    char*           configStr)
{
    int32_t return_error = -1;

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
static int32_t ReadConfigFile(
    EbConfig_t  *config,
    char		*configPath,
    uint32_t     instanceIdx)
{
    int32_t return_error = 0;

    // Open the config file
    FOPEN(config->configFile, configPath, "rb");

    if (config->configFile != (FILE*) NULL) {
        int32_t configFileSize = findFileSize(config->configFile);
        char *configFileBuffer = (char*) malloc(configFileSize);

        if (configFileBuffer != (char *) NULL) {
            int32_t resultSize = (int32_t) fread(configFileBuffer, 1, configFileSize, config->configFile);

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
static EB_ERRORTYPE VerifySettings(EbConfig_t *config, uint32_t channelNumber)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    // Check Input File
    if(config->inputFile == (FILE*) NULL) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid Input File\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->framesToBeEncoded <= -1) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: FrameToBeEncoded must be greater than 0\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->framesToBeEncoded >= LLONG_MAX) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: FrameToBeEncoded must be less than 2^64 - 1\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->bufferedInput < -1) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid BufferedInput. BufferedInput must greater or equal to -1\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->bufferedInput > config->framesToBeEncoded) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid BufferedInput. BufferedInput must be less or equal to the number of frames to be encoded\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->useQpFile == EB_TRUE && config->qpFile == NULL) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Could not find QP file, UseQpFile is set to 1\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }


#define MAX_LCU_SIZE                                64

    int32_t pictureWidthInLcu = (config->sourceWidth + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE;
    int32_t pictureHeightInLcu = (config->sourceHeight + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE;
    int32_t maxTileColumnCount = (pictureWidthInLcu + 3) / 4;
    int32_t maxTileRowCount = pictureHeightInLcu;
    int32_t horizontalTileIndex, verticalTileIndex;


    if (config->tileColumnCount < 1 || config->tileColumnCount > maxTileColumnCount) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid TileColumnCount. Maximum TileColumnCount for width %d is %d\n",
                channelNumber + 1, config->sourceWidth, maxTileColumnCount);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tileRowCount < 1 || config->tileRowCount > maxTileRowCount) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid TileRowCount. Maximum TileRowCount for height %d is %d\n",
                channelNumber + 1, config->sourceHeight, maxTileRowCount);
        return_error = EB_ErrorBadParameter;
    }

    if ((config->tileColumnCount * config->tileRowCount) <= 1 && config->tileSliceMode) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid TileSliceMode. TileSliceMode could be 1 for multi tile mode, please set it to 0 for single tile mode\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }


    {
        for (horizontalTileIndex = 0; horizontalTileIndex < (config->tileColumnCount - 1); ++horizontalTileIndex) {
            if (((horizontalTileIndex + 1) * pictureWidthInLcu / config->tileColumnCount -
                horizontalTileIndex * pictureWidthInLcu / config->tileColumnCount) * MAX_LCU_SIZE < 256) { // 256 samples
                fprintf(config->errorLogFile, "SVT [Error]: Instance %u: TileColumnCount must be chosen such that each tile has a minimum width of 256 luma samples\n", channelNumber + 1);
                return_error = EB_ErrorBadParameter;
                break;
            }
        }
        for (verticalTileIndex = 0; verticalTileIndex < (config->tileRowCount - 1); ++verticalTileIndex) {
            if (((verticalTileIndex + 1) * pictureHeightInLcu / config->tileRowCount -
                verticalTileIndex * pictureHeightInLcu / config->tileRowCount) * MAX_LCU_SIZE < 64) { // 64 samples
                fprintf(config->errorLogFile, "SVT [Error]: Instance %u: TileRowCount must be chosen such that each tile has a minimum height of 64 luma samples\n", channelNumber + 1);
                return_error = EB_ErrorBadParameter;
                break;

            }
        }

    }

    if (config->separateFields > 1) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid SeperateFields Input\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->encoderBitDepth == 10 && config->separateFields == 1)
    {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Separate fields is not supported for 10 bit input \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->injector > 1 ){
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid injector [0 - 1]\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

    if(config->injectorFrameRate > (240<<16) && config->injector){
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: The maximum allowed injectorFrameRate is 240 fps\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }
    // Check that the injector frameRate is non-zero
    if(config->injectorFrameRate <= 0 && config->injector) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: The injector frame rate should be greater than 0 fps \n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

    // TargetSocket
    if (config->targetSocket != -1 && config->targetSocket != 0 && config->targetSocket != 1) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u: Invalid TargetSocket [-1 - 1], your input: %d\n", channelNumber + 1, config->targetSocket);
        return_error = EB_ErrorBadParameter;
    }

    if (config->useNaluFile == 1 && config->naluFile == NULL) {
        fprintf(config->errorLogFile, "SVT [Error]: Instance %u : Invalid Nalu File\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    return return_error;
}

/******************************************
 * Find Token for multiple inputs
 ******************************************/
int32_t FindTokenMultipleInputs(
    int32_t         argc,
    char* const     argv[],
    const char*     token,
    char**          configStr)
{
    int32_t return_error = -1;
    int32_t done = 0;
    while((argc > 0) && (return_error != 0)) {
        return_error = EB_STRCMP(argv[--argc], token);
        if (return_error == 0) {
            int32_t count;
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

uint32_t GetHelp(
    int32_t     argc,
    char *const argv[])
{
    char config_string[COMMAND_LINE_MAX_SIZE];
    if (FindToken(argc, argv, HELP_TOKEN, config_string) == 0) {
        int32_t token_index = -1;

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
uint32_t GetNumberOfChannels(
    int32_t     argc,
    char *const argv[])
{
    char config_string[COMMAND_LINE_MAX_SIZE];
    uint32_t channelNumber;
    if (FindToken(argc, argv, CHANNEL_NUMBER_TOKEN, config_string) == 0) {

        // Set the input file
        channelNumber = strtol(config_string,  NULL, 0);
        if ((channelNumber > (uint32_t) MAX_CHANNEL_NUMBER) || channelNumber == 0){
            printf("Error: The number of channels has to be within the range [1,%u]\n",(uint32_t) MAX_CHANNEL_NUMBER);
            return 0;
        }else{
            return channelNumber;
        }
    }
    return 1;
}

void mark_token_as_read(
    const char  * token,
    char        * cmd_copy[],
    int32_t     * cmd_token_cnt
    )
{
    int32_t cmd_copy_index;
    for (cmd_copy_index = 0; cmd_copy_index < *(cmd_token_cnt); ++cmd_copy_index) {
        if (!EB_STRCMP(cmd_copy[cmd_copy_index], token)) {
            cmd_copy[cmd_copy_index] = cmd_copy[--(*cmd_token_cnt)];
        }
    }
}

EB_BOOL is_negative_number(
    const char* string) {
    int32_t length = (int32_t)strlen(string);
    int32_t index = 0;
    if (string[0] != '-') return EB_FALSE;
    for (index = 1; index < length; index++)
    {
        if (string[index] < '0' || string[index] > '9')
            return EB_FALSE;
    }
    return EB_TRUE;
}

// Computes the number of frames in the input file
int32_t ComputeFramesToBeEncoded(
    EbConfig_t   *config)
{
    uint64_t fileSize = 0;
    int32_t frameCount = 0;
    uint32_t frameSize;
    uint64_t currLoc;

    if (config->inputFile) {
        currLoc = ftello64(config->inputFile); // get current fp location
        fseeko64(config->inputFile, 0L, SEEK_END);
        fileSize = ftello64(config->inputFile);
        fseeko64(config->inputFile, currLoc, SEEK_SET); // seek back to that location
    }

    frameSize = config->inputPaddedWidth * config->inputPaddedHeight; // Luma
    frameSize += 2 * (frameSize >> (3 - config->encoderColorFormat)); // Add Chroma
    frameSize = frameSize << ((config->encoderBitDepth == 10) ? 1 : 0);

    if (frameSize == 0)
        return -1;

    if (config->encoderBitDepth == 10 && config->compressedTenBitFormat == 1)
        frameCount = (int32_t)(2 * ((double)fileSize / frameSize) / 1.25);
    else
        frameCount = (int32_t)(fileSize / frameSize);

    if (frameCount == 0)
        return -1;

    return frameCount;

}

static EB_ERRORTYPE ParseMasteringDisplayColorVolumeSEI(
    EbConfig_t* config)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    uint8_t *context = NULL, *token = NULL;
    const uint8_t *delimeter = (uint8_t*) "(,)GBRWPL";
    uint32_t primaries[10];
    uint32_t i = 0, j = 0, k = 0;

    if (strstr(config->masteringDisplayColorVolumeString, "G") != NULL && strstr(config->masteringDisplayColorVolumeString, "B") != NULL && strstr(config->masteringDisplayColorVolumeString, "R") != NULL && strstr(config->masteringDisplayColorVolumeString, "WP") != NULL && strstr(config->masteringDisplayColorVolumeString, "L") != NULL) {
        for (token = (uint8_t*)EB_STRTOK(config->masteringDisplayColorVolumeString, delimeter, &context); token != NULL; token = (uint8_t*)EB_STRTOK(NULL, delimeter, &context)) {
            primaries[i++] = (uint32_t)strtoul((char*)token, NULL, 0);
        }
        if (i == 10 && token == NULL) {
            for (j = 0; j < 3; j++) {
                config->displayPrimaryX[j] = primaries[k++];
                config->displayPrimaryY[j] = primaries[k++];
            }
            config->whitePointX = primaries[k++];
            config->whitePointY = primaries[k++];
            config->maxDisplayMasteringLuminance = primaries[k++];
            config->minDisplayMasteringLuminance = primaries[k++];
        }
        else {
            return_error = EB_ErrorBadParameter;
        }
    }
    else {
        return_error = EB_ErrorBadParameter;
    }
    return return_error;
}

/******************************************
* Read Command Line
******************************************/
EB_ERRORTYPE ReadCommandLine(
    int32_t        argc,
    char *const    argv[],
    EbConfig_t   **configs,
    uint32_t       numChannels,
    EB_ERRORTYPE  *return_errors)
{

    EB_ERRORTYPE return_error = EB_ErrorBadParameter;
    char		    config_string[COMMAND_LINE_MAX_SIZE];		// for one input options
    char		   *config_strings[MAX_CHANNEL_NUMBER]; // for multiple input options
    char           *cmd_copy[MAX_NUM_TOKENS];                 // keep track of extra tokens
    uint32_t    index           = 0;
    int32_t             cmd_token_cnt   = 0;                        // total number of tokens
    int32_t             token_index     = -1;
    int32_t ret_y4m;

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
    /********************** Parse parameters from input file if in y4m format **************************/
    /********************** overriding config file and command line inputs    **************************/
    /***************************************************************************************************/
    for (index = 0; index < numChannels; ++index) {
        if ((configs[index])->y4m_input == EB_TRUE) {
            ret_y4m = read_y4m_header(configs[index]);
            if (ret_y4m == EB_ErrorBadParameter) {
                printf("Error found when reading the y4m file parameters.\n");
                return EB_ErrorBadParameter;
            }
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
                    fprintf(configs[index]->errorLogFile, "SVT [Error]: Instance %u: Input yuv does not contain enough frames \n", index + 1);
                    return_errors[index] = EB_ErrorBadParameter;
                }

                // Force the injector latency mode, and injector frame rate when speed control is on
                if (return_errors[index] == EB_ErrorNone && configs[index]->speedControlFlag == 1) {
                    configs[index]->injector    = 1;
                }

                // Parse Master Display Color
                if (return_errors[index] == EB_ErrorNone && configs[index]->useMasteringDisplayColorVolume == EB_TRUE) {
                    return_errors[index] = ParseMasteringDisplayColorVolumeSEI(configs[index]);
                    if (return_errors[index] != EB_ErrorNone)
                        fprintf(configs[index]->errorLogFile, "SVT [Error]:  Instance %u: Couldn't parse MasterDisplay info. Make sure its passed in this format \"G(%%hu,%%hu)B(%%hu,%%hu)R(%%hu,%%hu)WP(%%hu,%%hu)L(%%u,%%u)\"", index + 1);
                }
            }
            return_error = (EB_ERRORTYPE)(return_error & return_errors[index]);
        }
    }

    // Print message for unprocessed tokens
    if (cmd_token_cnt > 0) {
        int32_t cmd_copy_index;
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
