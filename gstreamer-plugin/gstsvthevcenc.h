/*
* Copyright(c) 2019 Intel Corporation
*     Authors: Xavier Hallade <xavier.hallade@intel.com> Jun Tian <jun.tian@intel.com>
* SPDX - License - Identifier: LGPL-2.1-or-later
*/

#ifndef _GST_SVTHEVCENC_H_
#define _GST_SVTHEVCENC_H_

#include <gst/video/video.h>
#include <gst/video/gstvideoencoder.h>

#include <EbApi.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/queue.h>
#endif

G_BEGIN_DECLS
#define GST_TYPE_SVTHEVCENC \
  (gst_svthevcenc_get_type())
#define GST_SVTHEVCENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SVTHEVCENC,GstSvtHevcEnc))
#define GST_SVTHEVCENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SVTHEVCENC,GstSvtHevcEncClass))
#define GST_IS_SVTHEVCENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SVTHEVCENC))
#define GST_IS_SVTHEVCENC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SVTHEVCENC))

typedef enum
{
  GST_SVTHEVC_ENC_B_PYRAMID_FLAT,
  GST_SVTHEVC_ENC_B_PYRAMID_2LEVEL_HIERARCHY,
  GST_SVTHEVC_ENC_B_PYRAMID_3LEVEL_HIERARCHY,
  GST_SVTHEVC_ENC_B_PYRAMID_4LEVEL_HIERARCHY,
} GstSvtHevcEncBPyramid;

typedef enum
{
  GST_SVTHEVC_ENC_BASE_LAYER_MODE_BFRAME,
  GST_SVTHEVC_ENC_BASE_LAYER_MODE_PFRAME,
} GstSvtHevcEncBaseLayerMode;

typedef enum
{
  GST_SVTHEVC_ENC_RC_CQP,
  GST_SVTHEVC_ENC_RC_VBR,
} GstSvtHevcEncRC;

typedef enum
{
  GST_SVTHEVC_ENC_TUNE_SQ,
  GST_SVTHEVC_ENC_TUNE_OQ,
  GST_SVTHEVC_ENC_TUNE_VMAF,
} GstSvtHevcEncTune;

typedef enum
{
  GST_SVTHEVC_ENC_PRED_STRUCT_LOW_DELAY_P,
  GST_SVTHEVC_ENC_PRED_STRUCT_LOW_DELAY_B,
  GST_SVTHEVC_ENC_PRED_STRUCT_RANDOM_ACCESS,
} GstSvtHevcEncPredStruct;

typedef struct _GstInputFrame
{
#ifdef _WIN32
    SLIST_ENTRY list;
#else
    LIST_ENTRY(_GstInputFrame) list;
#endif
    GstVideoCodecFrame *ref_frame;
} GstInputFrame;

typedef struct _GstSvtHevcEnc
{
  GstVideoEncoder video_encoder;

  /* SVT-HEVC Encoder Handle */
  EB_COMPONENTTYPE *svt_encoder;

  /* GStreamer Codec state */
  GstVideoCodecState *state;

  /* GStreamer properties */
  gboolean enable_open_gop;
  guint config_interval;

  /* SVT-HEVC configuration */
  EB_H265_ENC_CONFIGURATION *svt_config;

  EB_BUFFERHEADERTYPE *input_buf;

  long long int frame_count;
  int dts_offset;
  const gchar *svt_version;
  gboolean inited;

  /*
   * Manage the input frames with linked list.
   * But it can be replace by other ways, such
   * as hash table.
   */
#ifdef _WIN32
  SLIST_HEADER input_frame_list;
  SLIST_HEADER tmp_input_frame_list;
#else
  LIST_HEAD(input_frame_list, _GstInputFrame) input_frame_list;
#endif
  gboolean ref_frame;
} GstSvtHevcEnc;

typedef struct _GstSvtHevcEncClass
{
  GstVideoEncoderClass video_encoder_class;
} GstSvtHevcEncClass;

GType gst_svthevcenc_get_type (void);

G_END_DECLS
#endif
