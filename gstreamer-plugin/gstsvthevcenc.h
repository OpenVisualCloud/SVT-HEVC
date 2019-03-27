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

typedef struct _GstSvtHevcEnc
{
  GstVideoEncoder video_encoder;

  /* SVT-HEVC Encoder Handle */
  EB_COMPONENTTYPE *svt_encoder;

  /* GStreamer Codec state */
  GstVideoCodecState *state;

  /* SVT-HEVC configuration */
  EB_H265_ENC_CONFIGURATION *svt_config;

  EB_BUFFERHEADERTYPE *input_buf;

  long long int frame_count;
  int dts_offset;
} GstSvtHevcEnc;

typedef struct _GstSvtHevcEncClass
{
  GstVideoEncoderClass video_encoder_class;
} GstSvtHevcEncClass;

GType gst_svthevcenc_get_type (void);

G_END_DECLS
#endif
