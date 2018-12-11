/*
* Scalable Video Technology for HEVC encoder library plugin
*
* Copyright (c) 2018 Intel Corporation
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "EbErrorCodes.h"
#include "EbTime.h"
#include "EbApi.h"

#include "libavutil/common.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

#include "internal.h"
#include "avcodec.h"

typedef struct SvtEncoder {
    EB_H265_ENC_CONFIGURATION           enc_params;
    EB_COMPONENTTYPE                    *svt_handle;
    EB_BUFFERHEADERTYPE                 *in_buf;
    EB_BUFFERHEADERTYPE                 *out_buf;
    int                                 raw_size;
} SvtEncoder;

typedef struct SvtParams {
    int vui_info;
    int hierarchical_level;
    int la_depth;
    int intra_ref_type;
    int enc_mode;
    int rc_mode;
    int scd;
    int tune;
    int qp;
    int profile;
    int base_layer_switch_mode;
}SvtParams;

typedef struct SvtContext {
    AVClass     *class;
    SvtEncoder  *svt_enc;
    SvtParams   svt_param;
    int         eos_flag;
} SvtContext;

static void free_buffer(SvtEncoder *svt_enc)
{
    if (svt_enc->in_buf) {
        EB_H265_ENC_INPUT *in_data = (EB_H265_ENC_INPUT *)svt_enc->in_buf->pBuffer;
        av_freep(&in_data);
        av_freep(&svt_enc->in_buf);
    }
    av_freep(&svt_enc->out_buf);
}

static EB_ERRORTYPE alloc_buffer(EB_H265_ENC_CONFIGURATION *config, SvtEncoder *svt_enc)
{
    EB_ERRORTYPE       ret         = EB_ErrorNone;

    const int    pack_mode_10bit   =
        (config->encoderBitDepth > 8) && (config->compressedTenBitFormat == 0) ? 1 : 0;
    const size_t luma_size_8bit    =
        config->sourceWidth * config->sourceHeight * (1 << pack_mode_10bit);
    const size_t luma_size_10bit   =
        (config->encoderBitDepth > 8 && pack_mode_10bit == 0) ? luma_size_8bit : 0;

    svt_enc->raw_size = (luma_size_8bit + luma_size_10bit) * 3 / 2;

    // allocate buffer for in and out
    svt_enc->in_buf           = av_mallocz(sizeof(EB_BUFFERHEADERTYPE));
    svt_enc->out_buf          = av_mallocz(sizeof(EB_BUFFERHEADERTYPE));
    if (!svt_enc->in_buf || !svt_enc->out_buf)
        goto failed;

    svt_enc->in_buf->pBuffer  = av_mallocz(sizeof(EB_H265_ENC_INPUT));
    if (!svt_enc->in_buf->pBuffer)
        goto failed;

    svt_enc->in_buf->nSize        = sizeof(EB_BUFFERHEADERTYPE);
    svt_enc->in_buf->pAppPrivate  = NULL;
    svt_enc->out_buf->nSize       = sizeof(EB_BUFFERHEADERTYPE);
    svt_enc->out_buf->nAllocLen   = svt_enc->raw_size;
    svt_enc->out_buf->pAppPrivate = NULL;

    return ret;

failed:
    free_buffer(svt_enc);
    return EB_ErrorInsufficientResources;
}

static int error_mapping(int val)
{
    int err;

    switch (val) {
    case EB_ErrorInsufficientResources:
        err = AVERROR(ENOMEM);
        break;

    case EB_ErrorUndefined:
    case EB_ErrorInvalidComponent:
    case EB_ErrorBadParameter:
        err = AVERROR(EINVAL);
        break;

    case EB_NoErrorEmptyQueue:
        err = AVERROR(EAGAIN);
        break;

    default:
        err = AVERROR_EXTERNAL;
    }

    return err;
}

static EB_ERRORTYPE config_enc_params(EB_H265_ENC_CONFIGURATION *param,
                                      AVCodecContext *avctx)
{
    SvtContext *q       = avctx->priv_data;
    SvtEncoder *svt_enc = q->svt_enc;
    EB_ERRORTYPE    ret = EB_ErrorNone;
    int        ten_bits = 0;

    param->sourceWidth     = avctx->width;
    param->sourceHeight    = avctx->height;

    if (avctx->pix_fmt == AV_PIX_FMT_YUV420P10LE) {
        av_log(avctx, AV_LOG_DEBUG , "Encoder 10 bits depth input\n");
        param->compressedTenBitFormat = 0;
        ten_bits = 1;
    }

    // Update param from options
    param->hierarchicalLevels     = q->svt_param.hierarchical_level;
    param->encMode                = q->svt_param.enc_mode;
    param->intraRefreshType       = q->svt_param.intra_ref_type;
    param->profile                = q->svt_param.profile;
    param->rateControlMode        = q->svt_param.rc_mode;
    param->sceneChangeDetection   = q->svt_param.scd;
    param->tune                   = q->svt_param.tune;
    param->baseLayerSwitchMode    = q->svt_param.base_layer_switch_mode;
    param->qp                     = q->svt_param.qp;

    param->targetBitRate          = avctx->bit_rate;
    param->intraPeriodLength      = avctx->gop_size-1;
    param->frameRateNumerator     = avctx->time_base.den;
    param->frameRateDenominator   = avctx->time_base.num * avctx->ticks_per_frame;

    param->codeVpsSpsPps          = 0;

    if (q->svt_param.vui_info)
        param->videoUsabilityInfo = q->svt_param.vui_info;

    if (q->svt_param.la_depth != -1)
        param->lookAheadDistance  = q->svt_param.la_depth;

    if (ten_bits) {
        param->encoderBitDepth        = 10;
        param->profile                = 2;
    }

    ret = alloc_buffer(param, svt_enc);

    return ret;
}

static void read_in_data(EB_H265_ENC_CONFIGURATION *config,
                         const AVFrame *frame,
                         EB_BUFFERHEADERTYPE *headerPtr)
{
    unsigned int is16bit = config->encoderBitDepth > 8;
    unsigned long long luma_size =
        (unsigned long long)config->sourceWidth * config->sourceHeight<< is16bit;
    EB_H265_ENC_INPUT *in_data = (EB_H265_ENC_INPUT*)headerPtr->pBuffer;

    // support yuv420p and yuv420p010
    in_data->luma = frame->data[0];
    in_data->cb   = frame->data[1];
    in_data->cr   = frame->data[2];

    // stride info
    in_data->yStride  = frame->linesize[0] >> is16bit;
    in_data->cbStride = frame->linesize[1] >> is16bit;
    in_data->crStride = frame->linesize[2] >> is16bit;

    headerPtr->nFilledLen   += luma_size * 3/2u;
}

static av_cold int eb_enc_init(AVCodecContext *avctx)
{
    SvtContext   *q = avctx->priv_data;
    SvtEncoder   *svt_enc = NULL;
    EB_ERRORTYPE ret = EB_ErrorNone;

    q->svt_enc  = av_mallocz(sizeof(*q->svt_enc));
    if (!q->svt_enc)
        return AVERROR(ENOMEM);

    svt_enc = q->svt_enc;

    q->eos_flag = 0;

    ret = EbInitHandle(&svt_enc->svt_handle, q, &svt_enc->enc_params);
    if (ret != EB_ErrorNone) {
        av_log(avctx, AV_LOG_ERROR, "Error init encoder handle\n");
        goto failed;
    }

    ret = config_enc_params(&svt_enc->enc_params, avctx);
    if (ret != EB_ErrorNone) {
        av_log(avctx, AV_LOG_ERROR, "Error configure encoder parameters\n");
        goto failed_init_handle;
    }

    ret = EbH265EncSetParameter(svt_enc->svt_handle, &svt_enc->enc_params);
    if (ret != EB_ErrorNone) {
        av_log(avctx, AV_LOG_ERROR, "Error setting encoder parameters\n");
        goto failed_init_handle;
    }

    ret = EbInitEncoder(svt_enc->svt_handle);
    if (ret != EB_ErrorNone) {
        av_log(avctx, AV_LOG_ERROR, "Error init encoder\n");
        goto failed_init_handle;
    }

    if (avctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER) {
        EB_BUFFERHEADERTYPE headerPtr;
        headerPtr.nSize       = sizeof(EB_BUFFERHEADERTYPE);
        headerPtr.nFilledLen  = 0;
        headerPtr.pBuffer     = av_malloc(10 * 1024 * 1024);
        headerPtr.nAllocLen   = (10 * 1024 * 1024);

        if (!headerPtr.pBuffer) {
            av_log(avctx, AV_LOG_ERROR,
                   "Cannot allocate buffer size %d.\n", headerPtr.nAllocLen);
            ret = EB_ErrorInsufficientResources;
            goto failed_init_enc;
        }

        ret = EbH265EncStreamHeader(svt_enc->svt_handle, &headerPtr);
        if (ret != EB_ErrorNone) {
            av_log(avctx, AV_LOG_ERROR, "Error when build stream header.\n");
            av_freep(&headerPtr.pBuffer);
            goto failed_init_enc;
        }

        avctx->extradata_size = headerPtr.nFilledLen;
        avctx->extradata = av_malloc(avctx->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!avctx->extradata) {
            av_log(avctx, AV_LOG_ERROR,
                   "Cannot allocate HEVC header of size %d.\n", avctx->extradata_size);
            av_freep(&headerPtr.pBuffer);
            ret = EB_ErrorInsufficientResources;
            goto failed_init_enc;
        }
        memcpy(avctx->extradata, headerPtr.pBuffer, avctx->extradata_size);

        av_freep(&headerPtr.pBuffer);
    }

    return 0;

failed_init_enc:
    EbDeinitEncoder(svt_enc->svt_handle);
failed_init_handle:
    EbDeinitHandle(svt_enc->svt_handle);
failed:
    return error_mapping(ret);
}

static int eb_send_frame(AVCodecContext *avctx, const AVFrame *frame)
{
    SvtContext           *q = avctx->priv_data;
    SvtEncoder           *svt_enc = q->svt_enc;
    EB_BUFFERHEADERTYPE  *headerPtr = svt_enc->in_buf;
    int                  ret = 0;

    if (!frame) {
        EB_BUFFERHEADERTYPE headerPtrLast;
        headerPtrLast.nAllocLen   = 0;
        headerPtrLast.nFilledLen  = 0;
        headerPtrLast.nTickCount  = 0;
        headerPtrLast.pAppPrivate = NULL;
        headerPtrLast.nOffset     = 0;
        headerPtrLast.pBuffer     = NULL;
        headerPtrLast.nFlags      = EB_BUFFERFLAG_EOS;

        EbH265EncSendPicture(svt_enc->svt_handle, &headerPtrLast);
        q->eos_flag = 1;
        av_log(avctx, AV_LOG_DEBUG, "Finish sending frames!!!\n");
        return ret;
    }

    read_in_data(&svt_enc->enc_params, frame, headerPtr);

    headerPtr->nOffset      = 0;
    headerPtr->nFlags       = 0;
    headerPtr->pAppPrivate  = NULL;
    headerPtr->pts          = frame->pts;
    headerPtr->sliceType    = INVALID_SLICE;
    EbH265EncSendPicture(svt_enc->svt_handle, headerPtr);

    return ret;
}

static int eb_receive_packet(AVCodecContext *avctx, AVPacket *pkt)
{
    SvtContext  *q = avctx->priv_data;
    SvtEncoder  *svt_enc = q->svt_enc;
    EB_BUFFERHEADERTYPE   *headerPtr = svt_enc->out_buf;
    EB_ERRORTYPE          stream_status = EB_ErrorNone;
    int ret = 0;

    if ((ret = ff_alloc_packet2(avctx, pkt, svt_enc->raw_size, 0)) < 0) {
        av_log(avctx, AV_LOG_ERROR, "Failed to allocate output packet.\n");
        return ret;
    }
    headerPtr->pBuffer = pkt->data;
    stream_status = EbH265GetPacket(svt_enc->svt_handle, headerPtr, q->eos_flag);
    if (stream_status == EB_NoErrorEmptyQueue)
        return AVERROR(EAGAIN);

    pkt->size = headerPtr->nFilledLen;
    pkt->pts  = headerPtr->pts;
    pkt->dts  = headerPtr->dts;
    if (headerPtr->sliceType == IDR_SLICE)
        pkt->flags |= AV_PKT_FLAG_KEY;
    if (headerPtr->sliceType == NON_REF_SLICE)
        pkt->flags |= AV_PKT_FLAG_DISPOSABLE;

    ret = (headerPtr->nFlags & EB_BUFFERFLAG_EOS) ? AVERROR_EOF : 0;
    return ret;
}

static av_cold int eb_enc_close(AVCodecContext *avctx)
{
    SvtContext *q = avctx->priv_data;
    SvtEncoder   *svt_enc = q->svt_enc;

    EbDeinitEncoder(svt_enc->svt_handle);
    EbDeinitHandle(svt_enc->svt_handle);

    free_buffer(svt_enc);
    av_freep(&svt_enc);

    return 0;
}

#define OFFSET(x) offsetof(SvtContext, x)
#define VE AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM
static const AVOption options[] = {
    {"vui", "Enable vui info", OFFSET(svt_param.vui_info),
     AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, VE },
    {"hielevel", "Hierarchical prediction levels setting", OFFSET(svt_param.hierarchical_level),
     AV_OPT_TYPE_INT, { .i64 = 3 }, 0, 3, VE , "hielevel"},
        { "flat",   NULL, 0, AV_OPT_TYPE_CONST, { .i64 = 0 },  INT_MIN, INT_MAX, VE, "hielevel" },
        { "2level", NULL, 0, AV_OPT_TYPE_CONST, { .i64 = 1 },  INT_MIN, INT_MAX, VE, "hielevel" },
        { "3level", NULL, 0, AV_OPT_TYPE_CONST, { .i64 = 2 },  INT_MIN, INT_MAX, VE, "hielevel" },
        { "4level", NULL, 0, AV_OPT_TYPE_CONST, { .i64 = 3 },  INT_MIN, INT_MAX, VE, "hielevel" },
    {"la_depth", "Look ahead distance [0, 256]", OFFSET(svt_param.la_depth),
     AV_OPT_TYPE_INT, { .i64 = -1 }, -1, 256, VE },
    {"intra_ref_type", "Intra refresh type", OFFSET(svt_param.intra_ref_type),
     AV_OPT_TYPE_INT, { .i64 = 1 }, 0, 2, VE , "intra_ref_type"},
        { "none", "No intra refresh", 0, AV_OPT_TYPE_CONST, { .i64 = 0 },  INT_MIN, INT_MAX, VE, "intra_ref_type" },
        { "cra",  "CRA (Open GOP)",   0, AV_OPT_TYPE_CONST, { .i64 = 1 },  INT_MIN, INT_MAX, VE, "intra_ref_type" },
        { "idr",  "IDR",              0, AV_OPT_TYPE_CONST, { .i64 = 2 },  INT_MIN, INT_MAX, VE, "intra_ref_type" },
    {"perset", "Encoding preset [0, 12] (e,g, for subjective quality tuning mode and >=4k resolution), [0, 10] (for >= 1080p resolution), [0, 9] (for all resolution and modes)",
     OFFSET(svt_param.enc_mode), AV_OPT_TYPE_INT, { .i64 = 9 }, 0, 12, VE },
    {"profile", "Profile setting, Main Still Picture Profile not supported", OFFSET(svt_param.profile),
     AV_OPT_TYPE_INT, { .i64 = 2 }, 1, 2, VE, "profile"},
        { "main",   NULL, 0, AV_OPT_TYPE_CONST, { .i64 = 0 },  INT_MIN, INT_MAX, VE, "profile" },
        { "main10", NULL, 0, AV_OPT_TYPE_CONST, { .i64 = 1 },  INT_MIN, INT_MAX, VE, "profile" },
    {"rc", "Bit rate control mode", OFFSET(svt_param.rc_mode),
     AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE , "rc"},
        { "cqp", NULL, 0, AV_OPT_TYPE_CONST, { .i64 = 0 },  INT_MIN, INT_MAX, VE, "rc" },
        { "vbr", NULL, 0, AV_OPT_TYPE_CONST, { .i64 = 1 },  INT_MIN, INT_MAX, VE, "rc" },
    {"qp", "QP value for intra frames", OFFSET(svt_param.qp),
     AV_OPT_TYPE_INT, { .i64 = 32 }, 0, 51, VE },
    {"sc_detection", "Scene change detection", OFFSET(svt_param.scd),
     AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, VE },
    {"tune", "Tune mode", OFFSET(svt_param.tune), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 1, VE, "tune" },
        { "sq", "subjective quality mode", 0, AV_OPT_TYPE_CONST, { .i64 = 0 },  INT_MIN, INT_MAX, VE, "tune" },
        { "oq", "objective quality mode",  0, AV_OPT_TYPE_CONST, { .i64 = 1 },  INT_MIN, INT_MAX, VE, "tune" },
    {"bl_mode", "Random Access Prediction Structure type setting", OFFSET(svt_param.base_layer_switch_mode),
     AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, VE },
    {NULL},
};

static const AVClass class = {
    .class_name = "libsvt_hevc",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

static const AVCodecDefault eb_enc_defaults[] = {
    { "b",         "7M"    },
    { "refs",      "0"     },
    { "g",         "64"   },
    { "flags",     "+cgop" },
    { NULL },
};

AVCodec ff_libsvt_hevc_encoder = {
    .name           = "libsvt_hevc",
    .long_name      = NULL_IF_CONFIG_SMALL("SVT-HEVC(Scalable Video Technology for HEVC) encoder"),
    .priv_data_size = sizeof(SvtContext),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_HEVC,
    .init           = eb_enc_init,
    .send_frame     = eb_send_frame,
    .receive_packet = eb_receive_packet,
    .close          = eb_enc_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_AUTO_THREADS,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_YUV420P,
                                                    AV_PIX_FMT_YUV420P10,
                                                    AV_PIX_FMT_NONE },
    .priv_class     = &class,
    .defaults       = eb_enc_defaults,
    .caps_internal  = FF_CODEC_CAP_INIT_CLEANUP,
    .wrapper_name   = "libsvt_hevc",
};
