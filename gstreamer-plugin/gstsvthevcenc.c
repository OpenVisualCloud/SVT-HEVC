/*
* Copyright(c) 2019 Intel Corporation
*     Authors: Xavier Hallade <xavier.hallade@intel.com> Jun Tian <jun.tian@intel.com>
* SPDX - License - Identifier: LGPL-2.1-or-later
*/

/**
 * SECTION:element-gstsvthevcenc
 *
 * The svthevcenc element does HEVC encoding using Scalable
 * Video Technology for HEVC Encoder (SVT-HEVC Encoder).
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! video/x-raw ! svthevcenc ! mpegtsmux !
 * filesink location=out.ts
 * ]|
 * Encodes test input into H.265 compressed data which is then packaged in
 * out.ts.
 * </refsect2>
 */

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideoencoder.h>
#include "gstsvthevcenc.h"

GST_DEBUG_CATEGORY_STATIC (gst_svthevcenc_debug_category);
#define GST_CAT_DEFAULT gst_svthevcenc_debug_category

/* prototypes */
static void gst_svthevcenc_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_svthevcenc_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_svthevcenc_dispose (GObject * object);
static void gst_svthevcenc_finalize (GObject * object);

gboolean gst_svthevcenc_allocate_svt_buffers (GstSvtHevcEnc * svthevcenc);
void gst_svthevenc_deallocate_svt_buffers (GstSvtHevcEnc * svthevcenc);
static gboolean gst_svthevcenc_configure_svt (GstSvtHevcEnc * svthevcenc);
static GstFlowReturn gst_svthevcenc_encode (GstSvtHevcEnc * svthevcenc,
    GstVideoCodecFrame * frame);
static gboolean gst_svthevcenc_send_eos (GstSvtHevcEnc * svthevcenc);
static GstFlowReturn gst_svthevcenc_dequeue_encoded_frames (GstSvtHevcEnc *
    svthevcenc, gboolean closing_encoder, gboolean output_frames);

static gboolean gst_svthevcenc_open (GstVideoEncoder * encoder);
static gboolean gst_svthevcenc_close (GstVideoEncoder * encoder);
static gboolean gst_svthevcenc_start (GstVideoEncoder * encoder);
static gboolean gst_svthevcenc_stop (GstVideoEncoder * encoder);
static gboolean gst_svthevcenc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state);
static GstFlowReturn gst_svthevcenc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);
static GstFlowReturn gst_svthevcenc_finish (GstVideoEncoder * encoder);
static GstFlowReturn gst_svthevcenc_pre_push (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);
static GstCaps *gst_svthevcenc_getcaps (GstVideoEncoder * encoder,
    GstCaps * filter);
static gboolean gst_svthevcenc_sink_event (GstVideoEncoder * encoder,
    GstEvent * event);
static gboolean gst_svthevcenc_src_event (GstVideoEncoder * encoder,
    GstEvent * event);
static gboolean gst_svthevcenc_negotiate (GstVideoEncoder * encoder);
static gboolean gst_svthevcenc_decide_allocation (GstVideoEncoder * encoder,
    GstQuery * query);
static gboolean gst_svthevcenc_propose_allocation (GstVideoEncoder * encoder,
    GstQuery * query);
static gboolean gst_svthevcenc_flush (GstVideoEncoder * encoder);

/* helpers */
void set_default_svt_configuration (EB_H265_ENC_CONFIGURATION * svt_config);
gint compare_video_code_frame_and_pts (const void *video_codec_frame_ptr,
    const void *pts_ptr);

enum
{
  PROP_0,
  PROP_ENCMODE,
  PROP_TUNE,
  PROP_LOWLATENCY,
  PROP_SPEEDCONTROL,
  PROP_B_PYRAMID,
  PROP_P_FRAMES,
  PROP_PRED_STRUCTURE,
  PROP_GOP_SIZE,
  PROP_INTRA_REFRESH,
  PROP_QP,
  PROP_DEBLOCKING,
  PROP_SAO,
  PROP_CONSTRAINED_INTRA,
  PROP_RC_MODE,
  PROP_BITRATE,
  PROP_QP_MAX,
  PROP_QP_MIN,
  PROP_LOOKAHEAD,
  PROP_SCD,
  PROP_AUD,
  PROP_CORES,
  PROP_SOCKET
};

#define PROP_RC_MODE_CQP 0
#define PROP_RC_MODE_VBR 1

#define PROP_ENCMODE_DEFAULT                9
#define PROP_TUNE_DEFAULT                   1
#define PROP_LOWLATENCY_DEFAULT             FALSE
#define PROP_SPEEDCONTROL_DEFAULT           0
#define PROP_B_PYRAMID_DEFAULT              3
#define PROP_P_FRAMES_DEFAULT               FALSE
#define PROP_PRED_STRUCTURE_DEFAULT         2
#define PROP_GOP_SIZE_DEFAULT               -1
#define PROP_INTRA_REFRESH_DEFAULT          1
#define PROP_QP_DEFAULT                     25
#define PROP_DEBLOCKING_DEFAULT             TRUE
#define PROP_SAO_DEFAULT                    TRUE
#define PROP_CONSTRAINED_INTRA_DEFAULT      FALSE
#define PROP_RC_MODE_DEFAULT                PROP_RC_MODE_CQP
#define PROP_BITRATE_DEFAULT                7000000
#define PROP_QP_MAX_DEFAULT                 48
#define PROP_QP_MIN_DEFAULT                 10
#define PROP_LOOKAHEAD_DEFAULT              (unsigned int)-1
#define PROP_SCD_DEFAULT                    TRUE
#define PROP_AUD_DEFAULT                    FALSE
#define PROP_CORES_DEFAULT                  0
#define PROP_SOCKET_DEFAULT                 -1

/* pad templates */
static GstStaticPadTemplate gst_svthevcenc_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) {I420, I420_10LE}, "
        "width = (int) [64, 8192], "
        "height = (int) [64, 4320], " "framerate = (fraction) [0, MAX]")
    );

static GstStaticPadTemplate gst_svthevcenc_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h265, "
        "stream-format = (string) byte-stream, "
        "alignment = (string) au, "
        "width = (int) [64, 8192], "
        "height = (int) [64, 4320], " "framerate = (fraction) [0, MAX]")
    );

/* class initialization */
G_DEFINE_TYPE_WITH_CODE (GstSvtHevcEnc, gst_svthevcenc, GST_TYPE_VIDEO_ENCODER,
    GST_DEBUG_CATEGORY_INIT (gst_svthevcenc_debug_category, "svthevcenc", 0,
        "debug category for SVT-HEVC encoder element"));

/* this mutex is required to avoid race conditions in SVT-HEVC memory allocations, which aren't thread-safe */
G_LOCK_DEFINE_STATIC (init_mutex);

static void
gst_svthevcenc_class_init (GstSvtHevcEncClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoEncoderClass *video_encoder_class = GST_VIDEO_ENCODER_CLASS (klass);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_svthevcenc_src_pad_template);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_svthevcenc_sink_pad_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "SvtHevcEnc", "Codec/Encoder/Video",
      "Scalable Video Technology for HEVC Encoder (SVT-HEVC Encoder)",
      "Xavier Hallade <xavier.hallade@intel.com> Jun Tian <jun.tian@intel.com>");

  gobject_class->set_property = gst_svthevcenc_set_property;
  gobject_class->get_property = gst_svthevcenc_get_property;
  gobject_class->dispose = gst_svthevcenc_dispose;
  gobject_class->finalize = gst_svthevcenc_finalize;
  video_encoder_class->open = GST_DEBUG_FUNCPTR (gst_svthevcenc_open);
  video_encoder_class->close = GST_DEBUG_FUNCPTR (gst_svthevcenc_close);
  video_encoder_class->start = GST_DEBUG_FUNCPTR (gst_svthevcenc_start);
  video_encoder_class->stop = GST_DEBUG_FUNCPTR (gst_svthevcenc_stop);
  video_encoder_class->set_format =
      GST_DEBUG_FUNCPTR (gst_svthevcenc_set_format);
  video_encoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_svthevcenc_handle_frame);
  video_encoder_class->finish = GST_DEBUG_FUNCPTR (gst_svthevcenc_finish);
  video_encoder_class->pre_push = GST_DEBUG_FUNCPTR (gst_svthevcenc_pre_push);
  video_encoder_class->getcaps = GST_DEBUG_FUNCPTR (gst_svthevcenc_getcaps);
  video_encoder_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_svthevcenc_sink_event);
  video_encoder_class->src_event = GST_DEBUG_FUNCPTR (gst_svthevcenc_src_event);
  video_encoder_class->negotiate = GST_DEBUG_FUNCPTR (gst_svthevcenc_negotiate);
  video_encoder_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_svthevcenc_decide_allocation);
  video_encoder_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_svthevcenc_propose_allocation);
  video_encoder_class->flush = GST_DEBUG_FUNCPTR (gst_svthevcenc_flush);


  g_object_class_install_property (gobject_class, PROP_ENCMODE,
      g_param_spec_uint ("speed", "speed (Encoder Mode)",
          "Quality vs density tradeoff point"
          " that the encoding is to be performed at"
          " (0 is the highest quality mode, 12 is the highest density mode) ",
          0, 12, PROP_ENCMODE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TUNE,
      g_param_spec_uint ("tune", "Tune",
          "0 gives a visually optimized mode."
          " Set to 1 to tune for PSNR/SSIM, 2 for VMAF.",
          0, 2, PROP_TUNE_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LOWLATENCY,
      g_param_spec_boolean ("low-latency", "Low latency",
          "Set to true for lower latency",
          PROP_LOWLATENCY_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SPEEDCONTROL,
      g_param_spec_uint ("speed-control", "Speed Control (in fps)",
          "Dynamically change the encoding speed preset"
          " to meet this defined average encoding speed (in fps)",
          0, 240, PROP_SPEEDCONTROL_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_B_PYRAMID,
      g_param_spec_uint ("b-pyramid", "B Pyramid (Hierarchical levels)",
          "0 : Flat, 1: 2 - Level Hierarchy, "
          "2 : 3 - Level Hierarchy, 3 : 4 - Level Hierarchy",
          0, 3, PROP_B_PYRAMID_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_P_FRAMES,
      g_param_spec_boolean ("p-frames", "P Frames",
          "Use P-frames in the base layer",
          PROP_P_FRAMES_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PRED_STRUCTURE,
      g_param_spec_uint ("pred-struct", "Prediction Structure",
          "0 : Low Delay P, 1 : Low Delay B"
          ", 2 : Random Access",
          0, 2, PROP_PRED_STRUCTURE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_GOP_SIZE,
      g_param_spec_int ("gop-size", "GOP size",
          "Period of Intra Frames insertion (-1 is auto)",
          -1, 251, PROP_GOP_SIZE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, PROP_INTRA_REFRESH,
      g_param_spec_int("intra-refresh", "Intra refresh type",
          "CRA (open GOP)"
          "or IDR frames (closed GOP)",
          1, 2, PROP_INTRA_REFRESH_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP,
      g_param_spec_uint ("qp", "Quantization parameter",
          "Initial quantization parameter for the Intra pictures in CQP mode",
          0, 51, PROP_QP_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEBLOCKING,
      g_param_spec_boolean ("deblocking", "Deblocking Loop Filtering",
          "Enable Deblocking Loop Filtering",
          PROP_DEBLOCKING_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SAO,
      g_param_spec_boolean ("sao", "Sample Adaptive Filter",
          "Enable Sample Adaptive Filtering",
          PROP_SAO_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CONSTRAINED_INTRA,
      g_param_spec_boolean ("constrained-intra", "Constrained Intra",
          "Enable Constrained Intra"
          "- this yields to sending two PPSs in the HEVC Elementary streams",
          PROP_CONSTRAINED_INTRA_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RC_MODE,
      g_param_spec_uint ("rc", "Rate-control mode",
          "0 : CQP, 1 : VBR",
          0, 1, PROP_RC_MODE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* TODO: add GST_PARAM_MUTABLE_PLAYING property and handle it? */
  g_object_class_install_property (gobject_class, PROP_BITRATE,
      g_param_spec_uint ("bitrate", "Target bitrate",
          "Target bitrate in bits/sec. Only used when in VBR mode",
          1, G_MAXUINT, PROP_BITRATE_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP_MAX,
      g_param_spec_uint ("max-qp", "Max Quantization parameter",
          "Maximum QP value allowed for rate control use"
          " Only used in VBR mode.",
          0, 51, PROP_QP_MAX_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QP_MIN,
      g_param_spec_uint ("min-qp", "Min Quantization parameter",
          "Minimum QP value allowed for rate control use"
          " Only used in VBR mode.",
          0, 50, PROP_QP_MIN_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LOOKAHEAD,
      g_param_spec_int ("lookahead", "Look Ahead Distance",
          "Number of frames to look ahead. -1 lets the encoder pick a value",
          -1, 250, PROP_LOOKAHEAD_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SCD,
      g_param_spec_boolean ("scd", "Scene Change Detection",
          "Enable Scene Change Detection algorithm",
          PROP_SCD_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_AUD,
      g_param_spec_boolean ("aud", "Access Unit Delimiters",
          "Insert Access Unit Delimiters in the bitstream",
          PROP_AUD_DEFAULT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CORES,
      g_param_spec_uint ("cores", "Number of logical cores",
          "Number of logical cores to be used. 0: auto",
          0, UINT_MAX, PROP_CORES_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SOCKET,
      g_param_spec_int ("socket", "Target socket",
          "Target socket to run on. -1: all available",
          -1, 15, PROP_SOCKET_DEFAULT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_svthevcenc_init (GstSvtHevcEnc * svthevcenc)
{
  GST_OBJECT_LOCK (svthevcenc);
  svthevcenc->svt_config = g_malloc (sizeof (EB_H265_ENC_CONFIGURATION));
  if (!svthevcenc->svt_config) {
    GST_ERROR_OBJECT (svthevcenc, "insufficient resources");
    GST_OBJECT_UNLOCK (svthevcenc);
    return;
  }
  memset (&svthevcenc->svt_encoder, 0, sizeof (svthevcenc->svt_encoder));
  svthevcenc->frame_count = 0;
  svthevcenc->dts_offset = 0;

  EB_ERRORTYPE res =
      EbInitHandle (&svthevcenc->svt_encoder, NULL, svthevcenc->svt_config);
  if (res != EB_ErrorNone) {
    GST_ERROR_OBJECT (svthevcenc, "EbInitHandle failed with error %d", res);
    GST_OBJECT_UNLOCK (svthevcenc);
    return;
  }
  /* setting configuration here since EbInitHandle overrides it */
  set_default_svt_configuration (svthevcenc->svt_config);
  GST_OBJECT_UNLOCK (svthevcenc);
}

void
gst_svthevcenc_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (object);

  /* TODO: support reconfiguring on the fly when possible */
  if (svthevcenc->state) {
    GST_ERROR_OBJECT (svthevcenc,
        "encoder state has been set before properties, this isn't supported yet.");
    return;
  }

  GST_LOG_OBJECT (svthevcenc, "setting property %u", property_id);

  switch (property_id) {
    case PROP_ENCMODE:
      svthevcenc->svt_config->encMode = g_value_get_uint (value);
      break;
    case PROP_TUNE:
      svthevcenc->svt_config->tune = g_value_get_uint (value);
      break;
    case PROP_LOWLATENCY:
      svthevcenc->svt_config->latencyMode = g_value_get_boolean (value);
      break;
    case PROP_SPEEDCONTROL:
      if (g_value_get_uint (value) > 0) {
        svthevcenc->svt_config->injectorFrameRate = g_value_get_uint (value);
        svthevcenc->svt_config->speedControlFlag = 1;
      } else {
        svthevcenc->svt_config->injectorFrameRate = 60 << 16;
        svthevcenc->svt_config->speedControlFlag = 0;
      }
      break;
    case PROP_B_PYRAMID:
      svthevcenc->svt_config->hierarchicalLevels = g_value_get_uint (value);
      break;
    case PROP_P_FRAMES:
      svthevcenc->svt_config->baseLayerSwitchMode = g_value_get_boolean (value);
      break;
    case PROP_PRED_STRUCTURE:
      svthevcenc->svt_config->predStructure = g_value_get_uint (value);
      break;
    case PROP_GOP_SIZE:
      svthevcenc->svt_config->intraPeriodLength = g_value_get_int (value) - 1;
      break;
    case PROP_INTRA_REFRESH:
      svthevcenc->svt_config->intraRefreshType = g_value_get_int(value);
      break;
    case PROP_QP:
      svthevcenc->svt_config->qp = g_value_get_uint (value);
      break;
    case PROP_DEBLOCKING:
      svthevcenc->svt_config->disableDlfFlag = !g_value_get_boolean (value);
      break;
    case PROP_SAO:
      svthevcenc->svt_config->enableSaoFlag = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAINED_INTRA:
      svthevcenc->svt_config->constrainedIntra = g_value_get_boolean (value);
      break;
    case PROP_RC_MODE:
      svthevcenc->svt_config->rateControlMode = g_value_get_uint (value);
      break;
    case PROP_BITRATE:
      svthevcenc->svt_config->targetBitRate = g_value_get_uint (value) * 1024;
      break;
    case PROP_QP_MAX:
      svthevcenc->svt_config->maxQpAllowed = g_value_get_uint (value);
      break;
    case PROP_QP_MIN:
      svthevcenc->svt_config->minQpAllowed = g_value_get_uint (value);
      break;
    case PROP_LOOKAHEAD:
      svthevcenc->svt_config->lookAheadDistance =
          (unsigned int) g_value_get_int (value);
      break;
    case PROP_SCD:
      svthevcenc->svt_config->sceneChangeDetection =
          g_value_get_boolean (value);
      break;
    case PROP_AUD:
      svthevcenc->svt_config->accessUnitDelimiter = g_value_get_boolean (value);
      break;
    case PROP_CORES:
      svthevcenc->svt_config->logicalProcessors = g_value_get_uint (value);
      break;
    case PROP_SOCKET:
      svthevcenc->svt_config->targetSocket = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_svthevcenc_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (object);

  GST_LOG_OBJECT (svthevcenc, "getting property %u", property_id);

  switch (property_id) {
    case PROP_ENCMODE:
      g_value_set_uint (value, svthevcenc->svt_config->encMode);
      break;
    case PROP_TUNE:
      g_value_set_uint (value, svthevcenc->svt_config->tune);
      break;
    case PROP_LOWLATENCY:
      g_value_set_boolean (value, svthevcenc->svt_config->latencyMode == 1);
      break;
    case PROP_SPEEDCONTROL:
      if (svthevcenc->svt_config->speedControlFlag) {
        g_value_set_uint (value, svthevcenc->svt_config->injectorFrameRate);
      } else {
        g_value_set_uint (value, 0);
      }
      break;
    case PROP_B_PYRAMID:
      g_value_set_uint (value, svthevcenc->svt_config->hierarchicalLevels);
      break;
    case PROP_P_FRAMES:
      g_value_set_boolean (value,
          svthevcenc->svt_config->baseLayerSwitchMode == 1);
      break;
    case PROP_PRED_STRUCTURE:
      g_value_set_uint (value, svthevcenc->svt_config->predStructure);
      break;
    case PROP_GOP_SIZE:
      g_value_set_int (value, svthevcenc->svt_config->intraPeriodLength + 1);
      break;
    case PROP_INTRA_REFRESH:
      g_value_set_int(value, svthevcenc->svt_config->intraRefreshType);
      break;
    case PROP_QP:
      g_value_set_uint (value, svthevcenc->svt_config->qp);
      break;
    case PROP_DEBLOCKING:
      g_value_set_boolean (value, svthevcenc->svt_config->disableDlfFlag == 0);
      break;
    case PROP_SAO:
      g_value_set_boolean (value, svthevcenc->svt_config->enableSaoFlag == 1);
      break;
    case PROP_CONSTRAINED_INTRA:
      g_value_set_boolean (value,
          svthevcenc->svt_config->constrainedIntra == 1);
      break;
    case PROP_RC_MODE:
      g_value_set_uint (value, svthevcenc->svt_config->rateControlMode);
      break;
    case PROP_BITRATE:
      g_value_set_uint (value, svthevcenc->svt_config->targetBitRate / 1024);
      break;
    case PROP_QP_MAX:
      g_value_set_uint (value, svthevcenc->svt_config->maxQpAllowed);
      break;
    case PROP_QP_MIN:
      g_value_set_uint (value, svthevcenc->svt_config->minQpAllowed);
      break;
    case PROP_LOOKAHEAD:
      g_value_set_int (value, (int) svthevcenc->svt_config->lookAheadDistance);
      break;
    case PROP_SCD:
      g_value_set_boolean (value,
          svthevcenc->svt_config->sceneChangeDetection == 1);
      break;
    case PROP_AUD:
      g_value_set_boolean (value,
          svthevcenc->svt_config->accessUnitDelimiter == 1);
      break;
    case PROP_CORES:
      g_value_set_uint (value, svthevcenc->svt_config->logicalProcessors);
      break;
    case PROP_SOCKET:
      g_value_set_int (value, svthevcenc->svt_config->targetSocket);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_svthevcenc_dispose (GObject * object)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (object);

  GST_DEBUG_OBJECT (svthevcenc, "dispose");

  /* clean up as possible.  may be called multiple times */
  if (svthevcenc->state)
    gst_video_codec_state_unref (svthevcenc->state);
  svthevcenc->state = NULL;

  G_OBJECT_CLASS (gst_svthevcenc_parent_class)->dispose (object);
}

void
gst_svthevcenc_finalize (GObject * object)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (object);

  GST_DEBUG_OBJECT (svthevcenc, "finalizing svthevcenc");

  GST_OBJECT_LOCK (svthevcenc);
  EbDeinitHandle (svthevcenc->svt_encoder);
  svthevcenc->svt_encoder = NULL;
  g_free (svthevcenc->svt_config);
  GST_OBJECT_UNLOCK (svthevcenc);

  G_OBJECT_CLASS (gst_svthevcenc_parent_class)->finalize (object);
}

gboolean
gst_svthevcenc_allocate_svt_buffers (GstSvtHevcEnc * svthevcenc)
{
  svthevcenc->input_buf = g_malloc (sizeof (EB_BUFFERHEADERTYPE));
  if (!svthevcenc->input_buf) {
    GST_ERROR_OBJECT (svthevcenc, "insufficient resources");
    return FALSE;
  }
  svthevcenc->input_buf->pBuffer = g_malloc (sizeof (EB_H265_ENC_INPUT));
  if (!svthevcenc->input_buf->pBuffer) {
    GST_ERROR_OBJECT (svthevcenc, "insufficient resources");
    return FALSE;
  }

  memset (svthevcenc->input_buf->pBuffer, 0, sizeof (EB_H265_ENC_INPUT));
  svthevcenc->input_buf->nSize = sizeof (EB_BUFFERHEADERTYPE);
  svthevcenc->input_buf->pAppPrivate = NULL;
  svthevcenc->input_buf->sliceType = EB_I_PICTURE;

  return TRUE;
}

void
gst_svthevenc_deallocate_svt_buffers (GstSvtHevcEnc * svthevcenc)
{
  if (svthevcenc->input_buf) {
    g_free (svthevcenc->input_buf->pBuffer);
    svthevcenc->input_buf->pBuffer = NULL;
    g_free (svthevcenc->input_buf);
    svthevcenc->input_buf = NULL;
  }
}

gboolean
gst_svthevcenc_configure_svt (GstSvtHevcEnc * svthevcenc)
{
  if (!svthevcenc->state) {
    GST_WARNING_OBJECT (svthevcenc, "no state, can't configure encoder yet");
    return FALSE;
  }

  /* set properties out of GstVideoInfo */
  GstVideoInfo *info = &svthevcenc->state->info;
  svthevcenc->svt_config->encoderBitDepth = GST_VIDEO_INFO_COMP_DEPTH (info, 0);
  svthevcenc->svt_config->sourceWidth = GST_VIDEO_INFO_WIDTH (info);
  svthevcenc->svt_config->sourceHeight = GST_VIDEO_INFO_HEIGHT (info);
  svthevcenc->svt_config->interlacedVideo = GST_VIDEO_INFO_IS_INTERLACED (info);
  svthevcenc->svt_config->frameRateNumerator = GST_VIDEO_INFO_FPS_N (info) > 0 ? GST_VIDEO_INFO_FPS_N (info) : 1;
  svthevcenc->svt_config->frameRateDenominator = GST_VIDEO_INFO_FPS_D (info) > 0 ? GST_VIDEO_INFO_FPS_D (info) : 1;
  svthevcenc->svt_config->frameRate =
      svthevcenc->svt_config->frameRateNumerator /
      svthevcenc->svt_config->frameRateDenominator;

  /* pick a default value for the look ahead distance
   * in CQP mode:2*minigop+1. in VBR:  intra Period */
  if (svthevcenc->svt_config->lookAheadDistance == (unsigned int) -1) {
    svthevcenc->svt_config->lookAheadDistance =
        (svthevcenc->svt_config->rateControlMode == PROP_RC_MODE_VBR) ?
        svthevcenc->svt_config->intraPeriodLength :
        2 * (1 << svthevcenc->svt_config->hierarchicalLevels) + 1;
  }

  /* TODO: better handle HDR metadata when GStreamer will have such support
   * https://gitlab.freedesktop.org/gstreamer/gst-plugins-base/issues/400 */
  if (GST_VIDEO_INFO_COLORIMETRY (info).matrix == GST_VIDEO_COLOR_MATRIX_BT2020
      && GST_VIDEO_INFO_COMP_DEPTH (info, 0) > 8) {
    svthevcenc->svt_config->highDynamicRangeInput = TRUE;
  }

  /* TODO: set profile and tier depending on negotiated caps */
  if (GST_VIDEO_INFO_COMP_DEPTH (info, 0) == 8)
    svthevcenc->svt_config->profile = 1;
  else
    svthevcenc->svt_config->profile = 2;

  EB_ERRORTYPE res =
      EbH265EncSetParameter (svthevcenc->svt_encoder, svthevcenc->svt_config);
  if (res != EB_ErrorNone) {
    GST_ERROR_OBJECT (svthevcenc, "EbH265EncSetParameter failed with error %d",
        res);
    return FALSE;
  }
  return TRUE;
}

gboolean
gst_svthevcenc_start_svt (GstSvtHevcEnc * svthevcenc)
{
  G_LOCK (init_mutex);
  EB_ERRORTYPE res = EbInitEncoder (svthevcenc->svt_encoder);
  G_UNLOCK (init_mutex);

  if (res != EB_ErrorNone) {
    GST_ERROR_OBJECT (svthevcenc, "EbH265EncSetParameter failed with error %d",
        res);
    return FALSE;
  }
  return TRUE;
}

void
set_default_svt_configuration (EB_H265_ENC_CONFIGURATION * svt_config)
{
  memset (svt_config, 0, sizeof (EB_H265_ENC_CONFIGURATION));
  svt_config->sourceWidth = 64;
  svt_config->sourceHeight = 64;
  svt_config->interlacedVideo = FALSE;
  svt_config->intraPeriodLength = PROP_GOP_SIZE_DEFAULT - 1;
  svt_config->intraRefreshType = PROP_INTRA_REFRESH_DEFAULT;
  svt_config->baseLayerSwitchMode = PROP_P_FRAMES_DEFAULT;
  svt_config->encMode = PROP_ENCMODE_DEFAULT;
  svt_config->frameRate = 60;
  svt_config->frameRateDenominator = 1;
  svt_config->frameRateNumerator = 60;
  svt_config->hierarchicalLevels = PROP_B_PYRAMID_DEFAULT;
  svt_config->predStructure = PROP_PRED_STRUCTURE_DEFAULT;
  svt_config->sceneChangeDetection = PROP_SCD_DEFAULT;
  svt_config->lookAheadDistance = PROP_LOOKAHEAD_DEFAULT;
  svt_config->framesToBeEncoded = 0;
  svt_config->rateControlMode = PROP_RC_MODE_DEFAULT;
  svt_config->targetBitRate = PROP_BITRATE_DEFAULT;
  svt_config->maxQpAllowed = PROP_QP_MAX_DEFAULT;
  svt_config->minQpAllowed = PROP_QP_MIN_DEFAULT;
  svt_config->qp = PROP_QP_DEFAULT;
  svt_config->useQpFile = FALSE;
  svt_config->disableDlfFlag = (PROP_DEBLOCKING_DEFAULT == FALSE);
  svt_config->enableSaoFlag = PROP_SAO_DEFAULT;
  svt_config->useDefaultMeHme = TRUE;
  svt_config->enableHmeFlag = TRUE;
  svt_config->searchAreaWidth = 16;
  svt_config->searchAreaHeight = 7;
  svt_config->constrainedIntra = PROP_CONSTRAINED_INTRA_DEFAULT;
  svt_config->tune = PROP_TUNE_DEFAULT;
  svt_config->channelId = 0;
  svt_config->activeChannelCount = 1;
  svt_config->logicalProcessors = PROP_CORES_DEFAULT;
  svt_config->targetSocket = PROP_SOCKET_DEFAULT;
  svt_config->bitRateReduction = TRUE;
  svt_config->improveSharpness = TRUE;
  /* needed to have correct framerate/duration in bitstream */
  svt_config->videoUsabilityInfo = TRUE;
  svt_config->highDynamicRangeInput = FALSE;
  svt_config->accessUnitDelimiter = PROP_AUD_DEFAULT;
  svt_config->bufferingPeriodSEI = FALSE;
  svt_config->pictureTimingSEI = FALSE;
  svt_config->registeredUserDataSeiFlag = FALSE;
  svt_config->unregisteredUserDataSeiFlag = FALSE;
  svt_config->recoveryPointSeiFlag = FALSE;
  svt_config->enableTemporalId = 1;
  svt_config->encoderBitDepth = 8;
  svt_config->encoderColorFormat = EB_YUV420;
  svt_config->compressedTenBitFormat = FALSE;
  svt_config->profile = 1;
  svt_config->tier = 0;
  svt_config->level = 0;
  svt_config->injectorFrameRate =
      (PROP_SPEEDCONTROL_DEFAULT > 0) ? PROP_SPEEDCONTROL_DEFAULT : 60 << 16;
  svt_config->speedControlFlag = (PROP_SPEEDCONTROL_DEFAULT > 0);
  svt_config->latencyMode = PROP_LOWLATENCY_DEFAULT;
  svt_config->codeVpsSpsPps = 1;
  svt_config->fpsInVps = TRUE;
  svt_config->codeEosNal = FALSE;
  svt_config->asmType = 1;
  svt_config->tileColumnCount = 1;
  svt_config->tileRowCount = 1;
}

GstFlowReturn
gst_svthevcenc_encode (GstSvtHevcEnc * svthevcenc, GstVideoCodecFrame * frame)
{
  GstFlowReturn ret = GST_FLOW_OK;
  EB_ERRORTYPE res = EB_ErrorNone;
  EB_BUFFERHEADERTYPE *input_buffer = svthevcenc->input_buf;
  EB_H265_ENC_INPUT *input_picture_buffer =
      (EB_H265_ENC_INPUT *) svthevcenc->input_buf->pBuffer;
  GstVideoFrame video_frame;

  GST_LOG_OBJECT (svthevcenc, "encode");

  if (!gst_video_frame_map (&video_frame, &svthevcenc->state->info,
          frame->input_buffer, GST_MAP_READ)) {
    GST_ERROR_OBJECT (svthevcenc, "couldn't map input frame");
    return GST_FLOW_ERROR;
  }

  input_picture_buffer->yStride =
      GST_VIDEO_FRAME_COMP_STRIDE (&video_frame,
      0) / GST_VIDEO_FRAME_COMP_PSTRIDE (&video_frame, 0);
  input_picture_buffer->cbStride =
      GST_VIDEO_FRAME_COMP_STRIDE (&video_frame,
      1) / GST_VIDEO_FRAME_COMP_PSTRIDE (&video_frame, 1);
  input_picture_buffer->crStride =
      GST_VIDEO_FRAME_COMP_STRIDE (&video_frame,
      2) / GST_VIDEO_FRAME_COMP_PSTRIDE (&video_frame, 2);

  input_picture_buffer->luma = GST_VIDEO_FRAME_PLANE_DATA (&video_frame, 0);
  input_picture_buffer->cb = GST_VIDEO_FRAME_PLANE_DATA (&video_frame, 1);
  input_picture_buffer->cr = GST_VIDEO_FRAME_PLANE_DATA (&video_frame, 2);

  input_buffer->nFilledLen = GST_VIDEO_FRAME_SIZE (&video_frame);

  /* Fill in Buffers Header control data */
  input_buffer->nFlags = 0;
  input_buffer->pAppPrivate = (void *) frame;
  input_buffer->pts = frame->pts;
  input_buffer->sliceType = EB_INVALID_PICTURE;

  if (GST_VIDEO_CODEC_FRAME_IS_FORCE_KEYFRAME (frame)) {
    input_buffer->sliceType =
        (svthevcenc->svt_config->intraRefreshType == 2) ?
        EB_IDR_PICTURE : EB_I_PICTURE;
  }

  res = EbH265EncSendPicture (svthevcenc->svt_encoder, input_buffer);
  if (res != EB_ErrorNone) {
    GST_ERROR_OBJECT (svthevcenc, "Issue %d sending picture to SVT-HEVC.", res);
    ret = GST_FLOW_ERROR;
  }
  gst_video_frame_unmap (&video_frame);

  return ret;
}

gboolean
gst_svthevcenc_send_eos (GstSvtHevcEnc * svthevcenc)
{
  EB_ERRORTYPE ret = EB_ErrorNone;

  EB_BUFFERHEADERTYPE input_buffer;
  input_buffer.nAllocLen = 0;
  input_buffer.nFilledLen = 0;
  input_buffer.nTickCount = 0;
  input_buffer.pAppPrivate = NULL;
  input_buffer.nFlags = EB_BUFFERFLAG_EOS;
  input_buffer.pBuffer = NULL;

  ret = EbH265EncSendPicture (svthevcenc->svt_encoder, &input_buffer);

  if (ret != EB_ErrorNone) {
    GST_ERROR_OBJECT (svthevcenc, "couldn't send EOS frame.");
    return FALSE;
  }

  return (ret == EB_ErrorNone);
}

gboolean
gst_svthevcenc_flush (GstVideoEncoder * encoder)
{
  GstFlowReturn ret =
      gst_svthevcenc_dequeue_encoded_frames (GST_SVTHEVCENC (encoder), TRUE,
      FALSE);

  return (ret != GST_FLOW_ERROR);
}

gint
compare_video_code_frame_and_pts (const void *video_codec_frame_ptr,
    const void *pts_ptr)
{
  return ((GstVideoCodecFrame *) video_codec_frame_ptr)->pts -
      *((GstClockTime *) pts_ptr);
}

GstFlowReturn
gst_svthevcenc_dequeue_encoded_frames (GstSvtHevcEnc * svthevcenc,
    gboolean done_sending_pics, gboolean output_frames)
{
  GstFlowReturn ret = GST_FLOW_OK;
  EB_ERRORTYPE res = EB_ErrorNone;
  gboolean encode_at_eos = FALSE;

  do {
    GList *pending_frames = NULL;
    GList *frame_list_element = NULL;
    GstVideoCodecFrame *frame = NULL;
    EB_BUFFERHEADERTYPE *output_buf = NULL;

    res =
        EbH265GetPacket (svthevcenc->svt_encoder, &output_buf,
        done_sending_pics);

    if (output_buf != NULL)
      encode_at_eos =
          ((output_buf->nFlags & EB_BUFFERFLAG_EOS) == EB_BUFFERFLAG_EOS);

    if (res == EB_ErrorMax) {
      GST_ERROR_OBJECT (svthevcenc, "Error while encoding, return\n");
      return GST_FLOW_ERROR;
    } else if (res != EB_NoErrorEmptyQueue && output_frames && output_buf) {
      /* if pAppPrivate is indeed propagated, get the frame through it
       * it's not currently the case with SVT-HEVC 1.3.0
       * so we fallback on using its PTS to find it back */
      if (output_buf->pAppPrivate) {
        frame = (GstVideoCodecFrame *) output_buf->pAppPrivate;
      } else {
        pending_frames = gst_video_encoder_get_frames (GST_VIDEO_ENCODER
            (svthevcenc));
        frame_list_element = g_list_find_custom (pending_frames,
            &output_buf->pts, compare_video_code_frame_and_pts);

        if (frame_list_element == NULL)
          return GST_FLOW_ERROR;

        frame = (GstVideoCodecFrame *) frame_list_element->data;
      }

      if (output_buf->sliceType == EB_IDR_PICTURE
          || output_buf->sliceType == EB_I_PICTURE) {
        GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);
      }

      frame->output_buffer =
          gst_buffer_new_allocate (NULL, output_buf->nFilledLen, NULL);
      gst_buffer_fill (frame->output_buffer, 0,
          output_buf->pBuffer, output_buf->nFilledLen);

      /* SVT-HEVC may return first frames with a negative DTS,
       * offsetting it to start at 0 since GStreamer 1.x doesn't support it */
      if (output_buf->dts + svthevcenc->dts_offset < 0) {
        svthevcenc->dts_offset = -output_buf->dts;
      }
      /* Gstreamer doesn't support negative DTS so we return
       * very small increasing ones for the first frames. */
      if (output_buf->dts < 1) {
        frame->dts = frame->output_buffer->dts =
            output_buf->dts + svthevcenc->dts_offset;
      } else {
        frame->dts = frame->output_buffer->dts =
            (output_buf->dts *
            svthevcenc->svt_config->frameRateDenominator * GST_SECOND) /
            svthevcenc->svt_config->frameRateNumerator;
      }

      frame->pts = frame->output_buffer->pts = output_buf->pts;

      /* fill header before the first frame gets out */
      if (svthevcenc->svt_config->codeVpsSpsPps == 0
          && svthevcenc->frame_count == 0) {
        EB_BUFFERHEADERTYPE *header_buf = NULL;
        GstBuffer *header_buffer = NULL;

        res = EbH265EncStreamHeader (svthevcenc->svt_encoder, &header_buf);

        if (res != EB_ErrorNone || header_buf == NULL) {
          GST_ERROR_OBJECT (svthevcenc,
              "EbH265EncStreamHeader failed with error %d", res);
          return GST_FLOW_ERROR;
        }

        header_buffer =
            gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
            header_buf->pBuffer, header_buf->nFilledLen,
            0, header_buf->nFilledLen, NULL, NULL);

        frame->output_buffer =
            gst_buffer_append (header_buffer, frame->output_buffer);
      }

      GST_LOG_OBJECT (svthevcenc,
          "\nDecode Order:\t%lld\tdts:\t%" GST_TIME_FORMAT "\tpts:\t%"
          GST_TIME_FORMAT "\tSliceType:\t%d", svthevcenc->frame_count,
          GST_TIME_ARGS (frame->dts), GST_TIME_ARGS (frame->pts),
          output_buf->sliceType);

      EbH265ReleaseOutBuffer (&output_buf);
      output_buf = NULL;

      ret = gst_video_encoder_finish_frame (GST_VIDEO_ENCODER (svthevcenc),
          frame);

      if (pending_frames != NULL) {
        g_list_free_full (pending_frames,
            (GDestroyNotify) gst_video_codec_frame_unref);
      }

      svthevcenc->frame_count++;
    }

  } while (res == EB_ErrorNone && !encode_at_eos);

  return ret;
}

static gboolean
gst_svthevcenc_open (GstVideoEncoder * encoder)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "open");

  return TRUE;
}

static gboolean
gst_svthevcenc_close (GstVideoEncoder * encoder)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "close");

  return TRUE;
}

static gboolean
gst_svthevcenc_start (GstVideoEncoder * encoder)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "start");
  /* starting the encoder is done in set_format,
   * once caps are fully negotiated */

  return TRUE;
}

static gboolean
gst_svthevcenc_stop (GstVideoEncoder * encoder)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "stop");

  GstVideoCodecFrame *remaining_frame = NULL;
  while ((remaining_frame =
          gst_video_encoder_get_oldest_frame (encoder)) != NULL) {
    GST_WARNING_OBJECT (svthevcenc,
        "encoder is being stopped, dropping frame %d",
        remaining_frame->system_frame_number);
    remaining_frame->output_buffer = NULL;
    gst_video_encoder_finish_frame (encoder, remaining_frame);
  }

  GST_OBJECT_LOCK (svthevcenc);
  if (svthevcenc->state)
    gst_video_codec_state_unref (svthevcenc->state);
  svthevcenc->state = NULL;
  GST_OBJECT_UNLOCK (svthevcenc);

  GST_OBJECT_LOCK (svthevcenc);
  EbDeinitEncoder (svthevcenc->svt_encoder);
  /* Destruct the buffer memory pool */
  gst_svthevenc_deallocate_svt_buffers (svthevcenc);
  GST_OBJECT_UNLOCK (svthevcenc);

  return TRUE;
}

static gboolean
gst_svthevcenc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);
  GstClockTime min_latency_frames = 0;
  GstCaps *src_caps = NULL;
  GST_DEBUG_OBJECT (svthevcenc, "set_format");

  /* TODO: handle configuration changes while encoder is running
   * and if there was already a state. */
  svthevcenc->state = gst_video_codec_state_ref (state);

  gst_svthevcenc_configure_svt (svthevcenc);
  gst_svthevcenc_allocate_svt_buffers (svthevcenc);
  gst_svthevcenc_start_svt (svthevcenc);

  if (svthevcenc->svt_config->latencyMode)
    min_latency_frames =
        svthevcenc->svt_config->lookAheadDistance +
        (svthevcenc->svt_config->encMode < 6
        || svthevcenc->svt_config->speedControlFlag ==
        1) ? svthevcenc->svt_config->frameRate : (svthevcenc->
        svt_config->encMode <
        8) ? svthevcenc->svt_config->
        frameRate >> 1 : ((2 << svthevcenc->svt_config->hierarchicalLevels) +
        6);
  else
    min_latency_frames =
        svthevcenc->svt_config->lookAheadDistance +
        (svthevcenc->svt_config->frameRate * 3 >> 1);

  /* TODO: find a better value for max_latency */
  gst_video_encoder_set_latency (encoder,
      min_latency_frames * GST_SECOND / svthevcenc->svt_config->frameRate,
      3 * GST_SECOND);

  src_caps =
      gst_static_pad_template_get_caps (&gst_svthevcenc_src_pad_template);
  gst_video_encoder_set_output_state (GST_VIDEO_ENCODER (encoder), src_caps,
      svthevcenc->state);
  gst_caps_unref (src_caps);

  GST_DEBUG_OBJECT (encoder, "output caps: %" GST_PTR_FORMAT,
      svthevcenc->state->caps);

  return TRUE;
}

static GstFlowReturn
gst_svthevcenc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);
  GstFlowReturn ret = GST_FLOW_OK;

  GST_DEBUG_OBJECT (svthevcenc, "handle_frame");

  ret = gst_svthevcenc_encode (svthevcenc, frame);
  if (ret != GST_FLOW_OK) {
    GST_DEBUG_OBJECT (encoder, "gst_svthevcenc_encode returned %d", ret);
    return ret;
  }

  return gst_svthevcenc_dequeue_encoded_frames (svthevcenc, FALSE, TRUE);
}

static GstFlowReturn
gst_svthevcenc_finish (GstVideoEncoder * encoder)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "finish");

  gst_svthevcenc_send_eos (svthevcenc);

  return gst_svthevcenc_dequeue_encoded_frames (svthevcenc, TRUE, TRUE);
}

static GstFlowReturn
gst_svthevcenc_pre_push (GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "pre_push");

  return GST_FLOW_OK;
}

static GstCaps *
gst_svthevcenc_getcaps (GstVideoEncoder * encoder, GstCaps * filter)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "getcaps");

  GstCaps *sink_caps =
      gst_static_pad_template_get_caps (&gst_svthevcenc_sink_pad_template);
  GstCaps *ret =
      gst_video_encoder_proxy_getcaps (GST_VIDEO_ENCODER (svthevcenc),
      sink_caps, filter);
  gst_caps_unref (sink_caps);

  return ret;
}

static gboolean
gst_svthevcenc_sink_event (GstVideoEncoder * encoder, GstEvent * event)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "sink_event");

  return
      GST_VIDEO_ENCODER_CLASS (gst_svthevcenc_parent_class)->sink_event
      (encoder, event);
}

static gboolean
gst_svthevcenc_src_event (GstVideoEncoder * encoder, GstEvent * event)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "src_event");

  return
      GST_VIDEO_ENCODER_CLASS (gst_svthevcenc_parent_class)->src_event (encoder,
      event);
}

static gboolean
gst_svthevcenc_negotiate (GstVideoEncoder * encoder)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "negotiate");

  return
      GST_VIDEO_ENCODER_CLASS (gst_svthevcenc_parent_class)->negotiate
      (encoder);
}

static gboolean
gst_svthevcenc_decide_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "decide_allocation");

  return TRUE;
}

static gboolean
gst_svthevcenc_propose_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  GstSvtHevcEnc *svthevcenc = GST_SVTHEVCENC (encoder);

  GST_DEBUG_OBJECT (svthevcenc, "propose_allocation");

  return TRUE;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "svthevcenc", GST_RANK_SECONDARY,
      GST_TYPE_SVTHEVCENC);
}

#ifndef VERSION
#define VERSION "1.0"
#endif
#ifndef PACKAGE
#define PACKAGE "gstreamer-svt-hevc"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "SVT-HEVC Encoder plugin for GStreamer"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://github.com/OpenVisualCloud"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    svthevcenc,
    "Scalable Video Technology for HEVC Encoder (SVT-HEVC Encoder)",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
