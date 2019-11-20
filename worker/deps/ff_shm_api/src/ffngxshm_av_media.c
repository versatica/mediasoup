/*
 * ffngxshm_av_media.c
 *
 *  Created on: May 3, 2018
 *      Author: Amir Pauker
 */

#include <float.h>
#include <inttypes.h>

#include <ffngxshm_av_media.h>
#include <libavcodec/avcodec.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_log.h>

#include <ngx_shm_av.h>
#include <ngx_shm_kpi_utils.h>

#include "ffngxshm.h"
#include "ffngxshm_log.h"

// underflow histogram number of buckets
#define FFNGXSHM_AV_MEDIA_UF_HIST_MAX_NUM_BUK  60
#define FFNGXSHM_AV_MEDIA_UF_HIST_NUM_BUK      20


const char *ffngxshm_av_flow_ctl_action_names[] = {
        "none",
        "use ",
        "disc",
        "dup "
};


/**
 * internal definition of the write context (MUTS NOT BE exposed to user of this library)
 */
typedef struct {
    ngx_shm_av_ctx_t       av_ctx;           // nginx shm av context
    ngx_pool_t             *pool;            // memory pool to be used by this context only (destroyed when the ctx is closed)
} _ffngxshm_av_wr_media_ctx_t;


typedef struct {
    av_pts_t               last_underflows_chk_tm;  // the timestamp of the last underflow check
    uint32_t               num_underflows;          // total number of underflows during the stream
    uint32_t               cur_avg_frame_dur;       // average frame duration (elapse time between frames) in microseconds. We store it here in case we fail to retrieve it from the stream
    uint16_t               trgt_num_pending;        // the target number for pending frames in the buffer. frames over that threshold should be discard
} _ffngxshm_av_media_flow_control_t;


/**
 * internal definition of the read context (MUTS NOT BE exposed to user of this library)
 */
typedef struct {
    ngx_shm_av_ctx_t                  av_ctx;           // nginx shm av context
    ngx_pool_t                        *pool;            // memory pool to be used by this context only (destroyed when the ctx is closed)
    ngx_buf_t                         buf;              // buffer to be used for reading video and audio data from shm

    ngx_uint_t                        last_video_ix;    // the index of the last returned video frame. if set to NGX_SHM_AV_UNSET_UINT it means
    // we are out of sync and the next read should sync with the stream first
    ngx_shm_seq                       last_video_sq;    // the sequence number of the last returned video frame
    av_pts_t                          last_video_dts;   // the timestamp of the last returned video frame (this is used to make sure we never sync back in time)

    ngx_uint_t                        last_audio_ix;    // the index of the last returned audio frame. if set to NGX_SHM_AV_UNSET_UINT it means
    // we are out of sync and the next read should sync with the stream first
    ngx_shm_seq                       last_audio_sq;    // the sequence number of the last returned audio frame
    av_pts_t                          last_audio_dts;   // the timestamp of the last returned audio frame (this is used to make sure we never sync back in time)

    ngx_shm_seq                       last_meta_seq[STREAM_SHM_MAX_CHANNELS];  // every channel potentially has meta-data e.g. SPS/PPS for H.264
    // based on the sequence number the reader can determine whether the meta-data has changed since the last read

    ngx_uint_t                        audio_codec_id;   // every time read next audio function is called we need to know the audio codec id in order to determine for example the size of the
    // header to be discard per frame. We store it here in order to avoid retrieving the meta-data for every frame. The values are constants
    // defined in ngx_shm_av.h


    _ffngxshm_av_media_flow_control_t flow_ctl;         // used for monitoring underflow conditions and dictating the poll interval

} _ffngxshm_av_rd_media_ctx_t;


#define ffngxshm_av_get_avctx(ctx) (ngx_shm_av_ctx_t*)&((ctx)->av_ctx)
#define ffngxshm_av_get_shm(ctx) &(ffngxshm_av_get_avctx(ctx))->shm
#define ffngxshm_av_get_stream_name(ctx) ngx_shm_get_stream_name(ffngxshm_av_get_shm(ctx))
#define ffngxshm_av_media_null_ctx ((_ffngxshm_av_rd_media_ctx_t*)0)

#define FFNGXSHM_AV_LOG_INFO(ctx,str,...)                                             \
        ffngxshm_log(FFNGXSHM_LOG_LEVEL_INFO, "- ffshm - %s %d - %s - " str,     \
                __FUNCTION__,__LINE__,                                            \
                ((ctx && ffngxshm_av_get_stream_name(ctx))? ffngxshm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define FFNGXSHM_AV_LOG_ERR(ctx,str,...)                                             \
        ffngxshm_log(FFNGXSHM_LOG_LEVEL_ERR, "- ffshm - %s %d - %s - " str,     \
                __FUNCTION__,__LINE__,                                            \
                ((ctx && ffngxshm_av_get_stream_name(ctx))? ffngxshm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define FFNGXSHM_AV_LOG_WARN(ctx,str,...)                                             \
        ffngxshm_log(FFNGXSHM_LOG_LEVEL_WARN, "- ffshm - %s %d - %s - " str,     \
                __FUNCTION__,__LINE__,                                            \
                ((ctx && ffngxshm_av_get_stream_name(ctx))? ffngxshm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define FFNGXSHM_AV_LOG_DEBUG(ctx,str,...)                                            \
        ffngxshm_log(FFNGXSHM_LOG_LEVEL_DEBUG, "- ffshm - %s %d - %s - " str,    \
                __FUNCTION__,__LINE__,                                             \
                ((ctx && ffngxshm_av_get_stream_name(ctx))? ffngxshm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)

#ifdef NGX_DEBUG
#define FFNGXSHM_AV_LOG_TRACE(ctx,str,...)                                            \
        ffngxshm_log(FFNGXSHM_LOG_LEVEL_DEBUG, "- ffshm - %s %d - %s - " str,    \
                __FUNCTION__,__LINE__,                                             \
                ((ctx && ffngxshm_av_get_stream_name(ctx))? ffngxshm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)
#else
#define FFNGXSHM_AV_LOG_TRACE(ctx,str,...)
#endif

static uint32_t ffngxshm_read_get_avg_video_inter_arrival_tm(_ffngxshm_av_rd_media_ctx_t *_ctx, uint32_t default_val);

/**
 * Takes as input an ffngxshm_shm_conf_t which is used as an abstraction to the shm real conf structure
 * and fill in the shm make it ready for calling ngx_stream_shm_cre
 */
static int
ffngxshm_av_wr_create_shm_conf(ffngxshm_av_init *init, ngx_shm_av_conf_t *av_conf)
{
    ngx_uint_t               chn;
    char                     *stream_name;
    ffngxshm_shm_conf_t      *conf;
    ngx_stream_shm_conf_t    *shm_conf;


    stream_name = init->stream_name;
    conf = &init->conf;

    memset(av_conf, 0, sizeof(*av_conf));

    av_conf->flags = NGX_SHM_AV_FLAGS_MUX_TYPE_FLV;
    av_conf->stats_win_size = init->stats_win_size;
    shm_conf = &av_conf->shm_conf;

    for(chn = 0;chn < FFNGXSHM_MAX_NUM_CHANNELS;chn++)
    {
        if(conf->channels[chn].video && conf->channels[chn].audio){
            FFNGXSHM_AV_LOG_ERR(ffngxshm_av_media_null_ctx, "interleave channel not allowed. chn=%d name=%s",
                    chn, stream_name);

            return FFNGXSHM_ERR;
        }

        av_conf->chn_conf[chn].audio = conf->channels[chn].audio;
        av_conf->chn_conf[chn].video = conf->channels[chn].video;

        // chunk header size
        shm_conf->chncf[chn].shm_chk_header_size =
                conf->channels[chn].video?
                        sizeof(ngx_shm_av_video_chk_header_t) : sizeof(ngx_shm_av_audio_chk_header_t);

        // channel header size
        shm_conf->chncf[chn].shm_chn_header_size =
                conf->channels[chn].video?
                        (sizeof(ngx_shm_av_chn_header_t) + sizeof(ngx_shm_av_video_meta_t)):
                        (sizeof(ngx_shm_av_chn_header_t) + sizeof(ngx_shm_av_audio_meta_t));

        // number of frames to store in memory for this channel
        shm_conf->chncf[chn].shm_num_chks =
                (conf->channels[chn].target_buf_ms * conf->channels[chn].target_fps) / 1000 + 1;

        // number of data blocks allocated for this channel
        // for video we assuming some degree of variation in frame size
        // therefore we aim at average of 3 blocks per frame
        if(conf->channels[chn].video){
            shm_conf->chncf[chn].shm_num_blks = shm_conf->chncf[chn].shm_num_chks * 3;
        }
        // for audio we aim for 1 block per frame since the frame size
        // is mostly constant.
        else{
            shm_conf->chncf[chn].shm_num_blks = shm_conf->chncf[chn].shm_num_chks;
        }

        // size of each data block. we calculate the average bytes per block
        // note that target_buf_ms is expressed in milliseconds therefore at the end we divide by 1000 to convert to seconds
        shm_conf->chncf[chn].shm_blk_size =
                ((double)conf->channels[chn].target_kbps / 8) * 1024 * conf->channels[chn].target_buf_ms /
                shm_conf->chncf[chn].shm_num_blks / 1000;

        shm_conf->chncf[chn].shm_blk_size = ngx_stream_shm_block_size_memalign(shm_conf->chncf[chn].shm_blk_size);
    }

    return FFNGXSHM_OK;
}

/**
 * Flow control context is used when Jitter buffer is enabled.
 * It is used for smoothing ingress stream flow fluctuations.
 */
static int
ffngxshm_av_reader_init_flow_control(ffngxshm_av_init *init, _ffngxshm_av_rd_media_ctx_t *ctx)
{
    ngx_shm_kpi_value        labels[FFNGXSHM_AV_MEDIA_UF_HIST_MAX_NUM_BUK + 1];
    ngx_shm_kpi_hist_init_t  hist_init;
    int                      i;

    FFNGXSHM_AV_LOG_INFO(ctx, "trgt_num_pending=%ui", init->trgt_num_pending);

    ctx->flow_ctl.last_underflows_chk_tm = ffngxshm_get_cur_timestamp();
    ctx->flow_ctl.trgt_num_pending = init->trgt_num_pending;
    ctx->flow_ctl.cur_avg_frame_dur  = 30000; // wild guess before we know the real average (value is in microseconds)

    return FFNGXSHM_OK;
}




/**
 * Create a new read context, open the specified stream's shm and associate it with the context.
 * In case it succeeds it return an open ready context. Otherwise it destroy the allocated memory and
 * returns NULL
 */
int
ffngxshm_open_av_reader(ffngxshm_av_init *init, ffngxshm_av_rd_ctx_t **ctx_out)
{
    ngx_pool_t                      *pool;
    _ffngxshm_av_rd_media_ctx_t     *_ctx;
    ngx_log_t                       *log;
    ngx_str_t                       stream_name;
    ngx_int_t                       chn;
    ngx_shm_av_chn_header_t         *chn_hd;

    _ctx = NULL;
    log  = ffngxshm_get_log();

    pool = ngx_create_pool(4096, log);

    if(!pool){
        FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
        return FFNGXSHM_ERR;
    }

    *ctx_out = ngx_pcalloc(pool, sizeof(ffngxshm_av_rd_ctx_t));
    if(!ctx_out){
        FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
        goto fail;
    }

    _ctx = ngx_pcalloc(pool, sizeof(_ffngxshm_av_rd_media_ctx_t));
    if(!_ctx){
        FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
        goto fail;
    }

    // input buffer for reading data from shared memory.
    // See avcodec_send_packet. The allocated buffer must contain extra padding bytes for
    // decoders that reads complete dword instead of byte by byes
    _ctx->buf.start = ngx_pcalloc(pool, FFNGXSHM_DEFAULT_BUF_SZ + AV_INPUT_BUFFER_PADDING_SIZE);
    if(!_ctx->buf.start){
        FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
        goto fail;
    }
    _ctx->buf.pos = _ctx->buf.last = _ctx->buf.start;
    _ctx->buf.end = _ctx->buf.start + FFNGXSHM_DEFAULT_BUF_SZ;


    (*ctx_out)->rd_ctx = _ctx;
    _ctx->pool = pool;

    // check if Jitter buffer is enabled and if it does setup the flow control context
    if(ffngxshm_av_reader_init_flow_control(init, _ctx) < 0){
        FFNGXSHM_AV_LOG_ERR(_ctx, "failed to initialize flow control");
        goto fail;
    }

    stream_name.data = (u_char*)init->stream_name;
    stream_name.len = strlen(init->stream_name);

    if(ngx_shm_av_init_reader_ctx(
            &_ctx->av_ctx, log, stream_name, rtmp, 0) != NGX_OK){
        FFNGXSHM_AV_LOG_ERR(_ctx, "fail to open stream '%s'", init->stream_name);
        goto fail;
    }

    _ctx->last_video_ix = NGX_SHM_AV_UNSET_UINT; // marks that we are out of sync
    _ctx->last_audio_ix = NGX_SHM_AV_UNSET_UINT; // marks that we are out of sync

    // copy the channel info to the init struct to let the reader know the channel's layout
    for(chn = 0;chn < FFNGXSHM_MAX_NUM_CHANNELS;chn++){
        chn_hd = ngx_shm_av_get_channel_header(ffngxshm_av_get_avctx(_ctx), chn);
        init->conf.channels[chn].audio = chn_hd->audio;
        init->conf.channels[chn].video = chn_hd->video;
    }

    FFNGXSHM_AV_LOG_INFO(_ctx, "successfully created reader ctx");

    return FFNGXSHM_OK;

    fail:
    ngx_destroy_pool(pool);
    return FFNGXSHM_ERR;
}

int
ffngxshm_close_av_reader(ffngxshm_av_rd_ctx_t *rd_ctx)
{
    _ffngxshm_av_rd_media_ctx_t    *_ctx;

    _ctx = rd_ctx->rd_ctx;

    FFNGXSHM_AV_LOG_INFO(_ctx, "closing reader ctx");

    ngx_shm_av_close_ctx(ffngxshm_av_get_avctx(_ctx));

    ngx_destroy_pool(_ctx->pool);

    av_packet_unref(&(rd_ctx->pkt_out));

    return FFNGXSHM_OK;
}

/**
 * Creates an av writer (for encoded data) context, open the specified shared memory for writing and associate the
 * shm context with the writer context. If the function succeeds it returns the allocated
 * context in ctx_out, otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using ffngxshm_close_av_writer.
 */
int
ffngxshm_open_av_writer(ffngxshm_av_init *init, ffngxshm_av_wr_ctx_t **ctx_out)
{
    ngx_pool_t                      *pool;
    _ffngxshm_av_wr_media_ctx_t     *_ctx;
    ngx_log_t                       *log;
    ngx_str_t                       stream_name;
    ngx_shm_av_conf_t               av_conf;
    ngx_int_t                       errno_out;

    _ctx = NULL;
    log  = ffngxshm_get_log();

    pool = ngx_create_pool(4096, log);

    if(!pool){
        FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
        return FFNGXSHM_ERR;
    }

    *ctx_out = ngx_pcalloc(pool, sizeof(ffngxshm_av_wr_ctx_t));
    if(!ctx_out){
        FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
        goto fail;
    }

    _ctx = ngx_pcalloc(pool, sizeof(_ffngxshm_av_wr_media_ctx_t));
    if(!_ctx){
        FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
        goto fail;
    }

    (*ctx_out)->wr_ctx = _ctx;
    _ctx->pool = pool;

    ffngxshm_av_wr_create_shm_conf(init, &av_conf);

    stream_name.data = (u_char*)init->stream_name;
    stream_name.len = strlen(init->stream_name);

    if(ngx_shm_av_init_writer_ctx(
            &_ctx->av_ctx, &av_conf, pool, log, stream_name, rtmp,
            (uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec,
            &errno_out) < 0){
        FFNGXSHM_AV_LOG_ERR(_ctx, "fail to create shm '%s' errno=%d", init->stream_name, errno_out);
        goto fail;
    }

    FFNGXSHM_AV_LOG_INFO(_ctx, "successfully created writer ctx");

    return FFNGXSHM_OK;

    fail:
    ngx_destroy_pool(pool);
    return FFNGXSHM_ERR;
}

int ffngxshm_close_av_writer(ffngxshm_av_wr_ctx_t *ctx, int time_wait)
{
    _ffngxshm_av_wr_media_ctx_t     *_ctx;

    _ctx = ctx->wr_ctx;

    if(_ctx && time_wait && ngx_stream_shm_is_writer_ready(ffngxshm_av_get_shm(_ctx))){
        FFNGXSHM_AV_LOG_INFO(_ctx, "marking shm time_wait");
        ngx_stream_shm_mark_as_time_wait(ffngxshm_av_get_shm(_ctx));
    }

    FFNGXSHM_AV_LOG_INFO(_ctx, "closing writer ctx");

    ngx_shm_av_close_ctx(ffngxshm_av_get_avctx(_ctx));

    ngx_destroy_pool(_ctx->pool);

    return FFNGXSHM_OK;
}

int
ffngxshm_get_video_avcodec_parameters(
        ffngxshm_av_rd_ctx_t *rd_ctx, struct AVCodecParameters *out)
{
    ngx_shm_av_video_meta_t         video_meta;
    ngx_shm_seq                     out_seq;
    ngx_int_t                       rc;
    _ffngxshm_av_rd_media_ctx_t     *_ctx;

    _ctx = rd_ctx->rd_ctx;

    // make sure we have encoding parameters
    rc = ngx_shm_av_get_channel_meta(
            ffngxshm_av_get_avctx(_ctx),
            ngx_shm_av_get_active_video_chn_ix(ffngxshm_av_get_avctx(_ctx)),
            &video_meta, sizeof(ngx_shm_av_video_meta_t),
            &out_seq);

    if(rc < 0){
        FFNGXSHM_AV_LOG_ERR(_ctx, "failed to get video channel meta. rc=%d", rc);
        // we are not mapping the returns code to one of FFNGXSHM_XXX since we
        // ensure the codes are the same
        return rc;
    }

    if(video_meta.codec_id != NGX_SHM_AV_VIDEO_CODEC_H264){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "unsupported codec id. cid=%ui", video_meta.codec_id);

        return FFNGXSHM_ERR;
    }

    if(out->extradata_size < video_meta.len){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "allocated buffer too small. req_sz=%uL buf_sz=%uL",
                video_meta.len, out->extradata_size);
        return FFNGXSHM_ERR;
    }

    memcpy(out->extradata, video_meta.buf, video_meta.len);
    out->extradata_size = video_meta.len;

    out->codec_type = AVMEDIA_TYPE_VIDEO;
    out->codec_id = AV_CODEC_ID_H264;
    out->width = video_meta.width;
    out->height = video_meta.height;

    return FFNGXSHM_OK;
}



int
ffngxshm_get_audio_avcodec_parameters(ffngxshm_av_rd_ctx_t *rd_ctx, struct AVCodecParameters *out)
{
    ngx_shm_av_audio_meta_t         audio_meta;
    ngx_shm_seq                     out_seq;
    ngx_int_t                       rc;
    _ffngxshm_av_rd_media_ctx_t     *_ctx;

    _ctx = rd_ctx->rd_ctx;

    static const uint8_t opus_default_extradata[30] = {
            'O', 'p', 'u', 's', 'H', 'e', 'a', 'd',
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    // make sure we have encoding parameters
    rc = ngx_shm_av_get_channel_meta(
            ffngxshm_av_get_avctx(_ctx),
            ngx_shm_av_get_active_audio_chn_ix(ffngxshm_av_get_avctx(_ctx)),
            &audio_meta, sizeof(ngx_shm_av_audio_meta_t),
            &out_seq);

    if(rc < 0){
        // in case the current active audio channel doesn't get any data we try to switch channel
        // this happens when both AAC and OPUS channels are enabled by configuration but the broadcast
        // only sends OPUS. By default the active channel is set to AAC
        if(ngx_shm_av_select_active_audio_channel(ffngxshm_av_get_avctx(_ctx)) == NGX_OK){
            rc = ngx_shm_av_get_channel_meta(
                    ffngxshm_av_get_avctx(_ctx),
                    ngx_shm_av_get_active_audio_chn_ix(ffngxshm_av_get_avctx(_ctx)),
                    &audio_meta, sizeof(ngx_shm_av_audio_meta_t),
                    &out_seq);

            if(rc < 0){
                FFNGXSHM_AV_LOG_DEBUG(_ctx, "failed to get audio channel meta. rc=%d", rc);

                // we are not mapping the returns code to one of FFNGXSHM_XXX since we
                // ensure the codes are the same
                return rc;
            }
        }
        else{
            return rc;
        }
    }

    switch(audio_meta.codec_id){
    case NGX_SHM_AV_AUDIO_CODEC_AAC:

        if(audio_meta.len <= 2){
            FFNGXSHM_AV_LOG_ERR(_ctx, "no ADTS headers");
            return FFNGXSHM_ERR;
        }

        // for AAC we have to copy the ADTS headers to the extra data
        // we subtract 2 from the meta buf len: 1 for FLV audio tag header and 1 for AVC audio tag header
        if(out->extradata_size + 2 < audio_meta.len){
            FFNGXSHM_AV_LOG_ERR(_ctx,
                    "allocated buffer too small. req_sz=%uL buf_sz=%uL",
                    audio_meta.len - 2, out->extradata_size);
            return FFNGXSHM_ERR;
        }

        memcpy(out->extradata, audio_meta.buf + 2, audio_meta.len - 2);
        out->extradata_size = audio_meta.len - 2;

        out->codec_id = AV_CODEC_ID_AAC;

        break;

    case NGX_SHM_AV_AUDIO_CODEC_MP3:

        out->codec_id = AV_CODEC_ID_MP3;
        break;

    case NGX_SHM_AV_AUDIO_CODEC_SPEEX:
        // speex for flv is always 16kHz, mono 16-bit
        // https://wwwimages2.adobe.com/content/dam/acom/en/devnet/flv/video_file_format_spec_v10.pdf
        // https://wwwimages2.adobe.com/content/dam/acom/en/devnet/pdf/swf-file-format-spec-v10.pdf
        out->channels = 1;
        out->sample_rate = 16000;
        out->channel_layout = av_get_default_channel_layout(1);
        out->codec_id = AV_CODEC_ID_SPEEX;
        break;

    case NGX_SHM_AV_AUDIO_CODEC_OPUS:
        if(out->extradata_size < sizeof(opus_default_extradata)){
            FFNGXSHM_AV_LOG_ERR(_ctx,
                    "allocated buffer too small. req_sz=%uL buf_sz=%uL",
                    sizeof(opus_default_extradata), out->extradata_size);
            return FFNGXSHM_ERR;
        }
        memcpy(out->extradata, opus_default_extradata, sizeof(opus_default_extradata));
        // NOTE: setting the below extradata values is mimicking the extradata
        // parsing in -> https://www.ffmpeg.org/doxygen/4.0/opus_8c_source.html
        out->extradata_size = sizeof(opus_default_extradata);
        // number of audio channels for opus
        out->extradata[9] = audio_meta.chn;
        // style 1 audio map type (non-ambisonic)
        out->extradata[18] = 1;
        // number of audio streams
        out->extradata[19] = 1;
        // number of stereo streams
        out->extradata[20] = audio_meta.chn - 1;
        out->codec_id = AV_CODEC_ID_OPUS;
        out->channels = audio_meta.chn;
        out->channel_layout = av_get_default_channel_layout(audio_meta.chn);
        break;

    default:
        FFNGXSHM_AV_LOG_ERR(_ctx, "unknown audio codec id. cid=%d", audio_meta.codec_id);
        return FFNGXSHM_ERR;
    }

    out->codec_type = AVMEDIA_TYPE_AUDIO;

    // we store the audio codec id in the ctx in order to avoid reading the meta-data for every
    // audio frame
    _ctx->audio_codec_id = audio_meta.codec_id;

    return FFNGXSHM_OK;
}


int
ffngxshm_get_av_chn_stats(ffngxshm_av_rd_ctx_t *rd_ctx, int chn, ffngxshm_av_chn_stats *out)
{
    ngx_shm_kpi_ma_stats            av_stats;
    ngx_shm_av_chn_header_t         *chn_hd;
    _ffngxshm_av_rd_media_ctx_t     *_ctx;
    int                             rc;

    _ctx = rd_ctx->rd_ctx;
    chn_hd = ngx_shm_av_get_channel_header(ffngxshm_av_get_avctx(_ctx), chn);

    rc = ngx_shm_kpi_ma_get_stats_no_stdev(&chn_hd->interarrival_tm, &av_stats);

    if(rc < 0){
        return FFNGXSHM_AGAIN;
    }

    out->mv_avg_interarrival_tm = av_stats.avg;


    rc = ngx_shm_kpi_ma_get_stats_no_stdev(&chn_hd->bitrate, &av_stats);

    if(rc < 0){
        return FFNGXSHM_AGAIN;
    }

    out->mv_avg_frame_sz = av_stats.avg;

    out->cur_avg_video_frame_dur = _ctx->flow_ctl.cur_avg_frame_dur;

    return FFNGXSHM_OK;
}

/**
 * an internal function that read the next video frame
 */
static int
__ffngxshm_read_next_av_video(
        ffngxshm_av_rd_ctx_t *rd_ctx,
        ffngxshm_av_frame_info_t *frame_info)
{
    ngx_shm_av_chn_header_t         *chn_hd;
    ngx_shm_av_video_meta_t         video_meta;
    ngx_shm_seq                     out_seq, seq, nxt_seq;
    ngx_stream_shm                  *shm;
    ngx_int_t                       rc;
    _ffngxshm_av_rd_media_ctx_t     *_ctx;
    uint8_t                         *side;
    AVPacket                        *pkt;
    ngx_int_t                       ix;
    ngx_stream_shm_chnk             *chunk;
    ngx_shm_av_video_chk_header_t   v_hd;
    size_t                          size;
    u_char                          *p;
    ngx_uint_t 						chn;
    ngx_av_shm_sync_spec_t          sync_spec;

    _ctx = rd_ctx->rd_ctx;
    shm = ffngxshm_av_get_shm(_ctx);

    // output parameters
    frame_info->num_pending     = 0;
    frame_info->most_recent_pts = 0;
    frame_info->video_rotation  = ngx_shm_av_get_video_rotation(ffngxshm_av_get_avctx(_ctx));

    if(ngx_stream_shm_is_time_wait(shm)){
        return FFNGXSHM_TIME_WAIT;
    }

    if(ngx_stream_shm_is_closing(shm)){
        return FFNGXSHM_CLOSING;
    }

    pkt = &(rd_ctx->pkt_out);
    chn = ngx_shm_av_get_active_video_chn_ix(ffngxshm_av_get_avctx(_ctx));

    av_packet_unref(pkt);

    chn_hd = ngx_shm_av_get_channel_header(ffngxshm_av_get_avctx(_ctx), chn);

    __ngx_shm_mem_barrier__
    seq = ngx_shm_volatile(chn_hd->seq);

    // we never read the meta-data before or there is new meta-data to read
    if(!_ctx->last_meta_seq[chn] || seq != _ctx->last_meta_seq[chn]){
        // make sure we have encoding parameters and if there are new encoding parameters
        // then add them as side data to the packet
        rc = ngx_shm_av_get_channel_meta(
                ffngxshm_av_get_avctx(_ctx),
                chn,
                &video_meta, sizeof(ngx_shm_av_video_meta_t),
                &out_seq);

        if(rc < 0){
            FFNGXSHM_AV_LOG_ERR(_ctx, "failed to get video channel meta. rc=%d", rc);
            return FFNGXSHM_ENC_PARAM_ERR;
        }

        // we have new meta-data and the codec is h.264, add it to the packet side data
        if(out_seq != _ctx->last_meta_seq[chn] && video_meta.codec_id == NGX_SHM_AV_VIDEO_CODEC_H264){


            FFNGXSHM_AV_LOG_TRACE(_ctx, "adding side data. "
                    "width=%d "
                    "height=%d "
                    "nal_unit_len=%d "
                    "pts=%d "
                    "len=%d ",
                    video_meta.width,
                    video_meta.height,
                    video_meta.nal_unit_len,
                    video_meta.pts,
                    video_meta.len);

            side = av_packet_new_side_data(
                    pkt, AV_PKT_DATA_NEW_EXTRADATA, video_meta.len);

            if (side) {
                memcpy(side, video_meta.buf, video_meta.len);
            }

            _ctx->last_meta_seq[chn] = out_seq;
        }
    }

    // in case we are out of sync with the channel, find the starting point
    if(_ctx->last_video_ix == NGX_SHM_AV_UNSET_UINT){

        // we are in the middle of the stream, sync based on last DTS
        if(_ctx->last_video_sq){
            memset(&sync_spec, 0, sizeof(sync_spec));
            sync_spec.pts = _ctx->last_video_dts;
            sync_spec.mode = NGX_AV_SHM_SYNC_MODE_AFTER_PTS;

            ix = ngx_shm_av_get_sync_channel_index(
                    ffngxshm_av_get_avctx(_ctx), chn, &sync_spec);
        }
        else{
            ix = ngx_shm_av_get_sync_channel_index(
                    ffngxshm_av_get_avctx(_ctx), chn, NULL);
        }

        if(ix < 0){
            FFNGXSHM_AV_LOG_WARN(_ctx, "fail to sync. chn=%ui.", chn);
            return FFNGXSHM_OUT_OF_SYNC; // was EOF
        }
        chunk = ngx_shm_av_get_chn_chunk(ffngxshm_av_get_avctx(_ctx), ix, chn);

        // acquire the lazy read lock
        seq = chunk->seq_num;
        __ngx_shm_mem_barrier__

        _ctx->last_video_sq = seq - 1; // make sure the "falling behind" below test will not fail
    }
    else{
        ix = ngx_shm_av_adjust_chn_index(ffngxshm_av_get_avctx(_ctx), _ctx->last_video_ix + 1, chn);
        chunk = ngx_shm_av_get_chn_chunk(ffngxshm_av_get_avctx(_ctx), ix, chn);

        // acquire the lazy read lock
        seq = chunk->seq_num;
        __ngx_shm_mem_barrier__
    }


    // the chunk sequence number must be greater than zero
    // if it is zero it means it hasn't been used or it has been invalidated
    if(seq == 0){
        FFNGXSHM_AV_LOG_DEBUG(_ctx,
                "found video chunk with sequence zero mid stream. "
                "chn=%d "
                "oldest_ix=%d oldest_sq=%uD cur_ix=%d cur_sq=%uD "
                "ix=%d lst_sq=%uD nxt_ix=%d ts=%uL",
                chn,
                ngx_shm_av_get_oldest_video_index(ffngxshm_av_get_avctx(_ctx)),
                (ngx_shm_av_get_oldest_video_chunk(ffngxshm_av_get_avctx(_ctx)))->seq_num,
                ngx_shm_av_get_cur_video_index(ffngxshm_av_get_avctx(_ctx)),
                ngx_shm_av_get_cur_video_seq(ffngxshm_av_get_avctx(_ctx)),
                _ctx->last_video_ix, _ctx->last_video_sq, ix,
                ((uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec));

        // invalid sequence mid stream. It most likely happens when the
        // writers invalidate the oldest chunk in order to free data blocks
        // if the reader falls behind to the point when it attempt to read the
        // oldest chunk we have to re-sync forward
        if(_ctx->last_video_sq < ngx_shm_av_get_cur_video_seq(ffngxshm_av_get_avctx(_ctx))){

            FFNGXSHM_AV_LOG_DEBUG(_ctx, "force re-sync");

            _ctx->last_video_ix = NGX_SHM_AV_UNSET_UINT; // make sure we re-sync at the next iteration
            return FFNGXSHM_OUT_OF_SYNC;
        }

        FFNGXSHM_AV_LOG_DEBUG(_ctx,"chunk not ready. ");

        return FFNGXSHM_EOF;
    }
    // current chunk is being edited or we already returned the most recent chunk
    else if(seq == NGX_SHM_AV_UNSET_UINT || _ctx->last_video_sq > seq){
        FFNGXSHM_AV_LOG_DEBUG(_ctx,
                "chunk not ready. "
                "chn=%ui ix=%d sq=%uL last_sq=%uL",
                chn, ix, seq, _ctx->last_video_sq);

        return FFNGXSHM_EOF;
    }


    // reader is falling behind, we need to re-sync
    if(_ctx->last_video_sq  + 1 < seq){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "reader is falling behind. "
                "chn=%ui ix=%d sq=%uL last_sq=%uL",
                chn, ix, seq, _ctx->last_video_sq);

        _ctx->last_video_ix = NGX_SHM_AV_UNSET_UINT; // make sure we re-sync at the next iteration

        return FFNGXSHM_OUT_OF_SYNC;
    }

    // we have to make a copy of the chunk's header to make sure it is not being edited while
    // we read from it
    v_hd = *(ngx_shm_av_get_video_chunk_header(chunk, ffngxshm_av_get_avctx(_ctx)));

    // the chunk must contain at least 5 bytes for the AVC header
    if(v_hd.size <= 5){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "invalid chunk size. sq=%uL sz=%uL", seq, v_hd.size);
        // mark that we are out of sync and return
        goto out_of_sync;
    }

    if(_ctx->buf.start + v_hd.size > _ctx->buf.end){
        FFNGXSHM_AV_LOG_INFO(_ctx,
                "input buffer too small. increasing. "
                "cur_sz=%uL req_sz=%uL",
                (_ctx->buf.end - _ctx->buf.start), v_hd.size);

        size = (v_hd.size > (_ctx->buf.end - _ctx->buf.start) * 2)?
                v_hd.size : ((_ctx->buf.end - _ctx->buf.start) * 2);

        if(size > FFNGXSHM_DEFAULT_BUF_MAX_SZ){
            FFNGXSHM_AV_LOG_ERR(_ctx, "chunk size too large. re-synching. sz=%uL", v_hd.size);
            // mark that we are out of sync and return
            goto out_of_sync;
        }
        else{
            FFNGXSHM_AV_LOG_INFO(_ctx,
                    "reallocating input buffer. sz=%uL", size);

            p = ngx_palloc(_ctx->pool, size);
            if(!p){
                FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
                return FFNGXSHM_ERR;
            }

            ngx_pfree(_ctx->pool, _ctx->buf.start);
            _ctx->buf.start = p;
            _ctx->buf.end = _ctx->buf.start + size;
        }
    }

    _ctx->buf.pos = _ctx->buf.last = _ctx->buf.start;

    // copy the content of the chunk to the given buffer
    if(ngx_stream_shm_copy_chunk(shm, chunk, &_ctx->buf, v_hd.size) != v_hd.size){
        FFNGXSHM_AV_LOG_ERR(_ctx, "failed to read chunk content. "
                "sq=%uL sz=%uL", seq, v_hd.size);
        // mark that we are out of sync and return
        goto out_of_sync;
    }

    // retrieving the number of pending frames in the buffer
    // NOTE: it should be done before we re-check the lazy lock
    nxt_seq = ngx_shm_av_get_chn_cur_seq(ffngxshm_av_get_avctx(_ctx), chn);
    if(nxt_seq != NGX_SHM_AV_UNSET_UINT && nxt_seq > seq){
        frame_info->num_pending     = nxt_seq - seq;
        frame_info->most_recent_pts = chn_hd->last_pts;
    }


    // in case the writer overridden the chunk as we read we have
    // to re-sync
    __ngx_shm_mem_barrier__
    if(!ngx_stream_shm_cmp_seq(seq, chunk->seq_num)){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "reader is behind. sq=%uL", seq);
        // mark that we are out of sync and return
        goto out_of_sync;
    }

    _ctx->last_video_ix = ix;
    _ctx->last_video_dts = v_hd.src_dts;
    _ctx->last_video_sq = seq;
    _ctx->buf.last += v_hd.size;

    // discard FLV video tag header (1 byte format, 1 byte AVC packet type, 3 bytes CTS)
    pkt->data = _ctx->buf.pos + 5;
    pkt->size = v_hd.size - 5;
    pkt->dts = v_hd.src_dts;
    pkt->pts = v_hd.src_dts + v_hd.src_cts;

    if(v_hd.keyframe){
        pkt->flags = AV_PKT_FLAG_KEY;
    }


    FFNGXSHM_AV_LOG_DEBUG(_ctx,
            "video pkt: ix=%d sq=%uL dts=%uL pts=%uL sz=%uL kf=%ui",
            ix, seq, pkt->dts, pkt->pts, pkt->size, v_hd.keyframe);

    return FFNGXSHM_OK;

    out_of_sync:
    _ctx->last_video_ix = NGX_SHM_AV_UNSET_UINT;
    _ctx->last_video_dts = v_hd.src_dts;
    _ctx->last_video_sq = seq;
    return FFNGXSHM_OUT_OF_SYNC;
}

/**
 * returns the average video inter-arrival time in microseconds based on the video channel stats stored
 * in the channel's header. The function takes into account that at the beginning of the
 * stream the avg might be much higher than the real fps due to on start push
 */
static inline uint32_t
ffngxshm_read_get_avg_video_inter_arrival_tm(_ffngxshm_av_rd_media_ctx_t *_ctx, uint32_t default_val)
{
    int                             rc;
    ngx_uint_t                      pts_dur, num_chunks;
    ngx_shm_av_chn_header_t         *chn_hd;
    ngx_uint_t                      chn;
    ngx_shm_kpi_ma_stats            interarrival_tm;
    uint32_t                        ret;

    chn = ngx_shm_av_get_active_video_chn_ix(ffngxshm_av_get_avctx(_ctx));

    // calculate the encoder's FPS based on the PTS of the first and last frames in the buffer
    // and the number of frames in the buffer. This calculation is based on the encoder clock and is
    // agnostic to network conditions
    rc = ngx_shm_av_get_video_chn_buf_duration(ffngxshm_av_get_avctx(_ctx), chn, &pts_dur, &num_chunks);

#if NGX_DEBUG
    if(rc < 0){
        FFNGXSHM_AV_LOG_ERR(_ctx, "failed to get fps using buf duration. rc=%d", rc);
    }
#endif

    // in case we fail to calculate based on encoder PTS we fall back to moving average based on received
    // times (server clock)
    if(rc < 0 || !num_chunks){
        // we get the average FPS from the source stream. This is used by the reader to
        // determine the PTS of the duplicated frame
        chn_hd = ngx_shm_av_get_channel_header(ffngxshm_av_get_avctx(_ctx), chn);
        rc = ngx_shm_kpi_ma_get_stats_no_stdev(&chn_hd->interarrival_tm, &interarrival_tm);

        // in case we failed use default value
        if(rc < 0){
            FFNGXSHM_AV_LOG_ERR(_ctx, "failed to get fps using both encoder fps and received times.");
            ret = default_val;
        }
        else{
            ret = (uint32_t)(interarrival_tm.avg * 1000.0); // denoted in microseconds
        }
    }
    else{
        // inter-arrival time in microseconds
        ret = (uint32_t)(((double)pts_dur * 1000.0) / (double)num_chunks );
    }

    FFNGXSHM_AV_LOG_TRACE(_ctx,
            "inter-arrival: ret=%uL ma.avg=%.3f ma.cnt=%uL rc=%d default=%uL",
            ret, interarrival_tm.avg, interarrival_tm.count, rc, default_val);

    return ret;
}

int
ffngxshm_read_next_av_video_with_flow_ctl(
        ffngxshm_av_rd_ctx_t *rd_ctx,
        ffngxshm_av_frame_info_t *frame_info)
{
    int                             rc;
    _ffngxshm_av_rd_media_ctx_t     *_ctx;
    av_pts_t                        cur_tm;

    frame_info->num_pending = 0;

    _ctx   = rd_ctx->rd_ctx;

    rc = __ffngxshm_read_next_av_video(rd_ctx, frame_info);

    // we get the average FPS from the source stream. This is used by the reader to
    // determine the PTS of the duplicated frame
    _ctx->flow_ctl.cur_avg_frame_dur = frame_info->frame_dur =
            ffngxshm_read_get_avg_video_inter_arrival_tm(_ctx, _ctx->flow_ctl.cur_avg_frame_dur);

    frame_info->poll_interval = frame_info->frame_dur;

    switch(rc){
    case FFNGXSHM_ERR:
    case FFNGXSHM_AGAIN:
    case FFNGXSHM_ENC_PARAM_ERR:
    case FFNGXSHM_OUT_OF_SYNC:
        frame_info->flow_ctl_action = ffngxshm_av_fc_dup_prev_frame;
        FFNGXSHM_AV_LOG_DEBUG(_ctx, "read next av failed. rc=%d", rc);
        return FFNGXSHM_AGAIN;

    case FFNGXSHM_CLOSING:
        // perform a clean shutdown
        frame_info->flow_ctl_action = ffngxshm_av_fc_none;
        return FFNGXSHM_CLOSING;

    case FFNGXSHM_TIME_WAIT:
        // abnormal disconnect, signal wait
        frame_info->flow_ctl_action = ffngxshm_av_fc_none;
        return FFNGXSHM_TIME_WAIT;

        // underflow
    case FFNGXSHM_EOF:
        _ctx->flow_ctl.num_underflows++;

        // in case of an underflow we have to instruct the client to duplicate the previous frame
        frame_info->flow_ctl_action = ffngxshm_av_fc_dup_prev_frame;
        frame_info->poll_interval *= 2; // we have to slow down the client in order to fill up the buffer
        FFNGXSHM_AV_LOG_TRACE(_ctx, "EOF: poll_intr=%ui", frame_info->poll_interval);
        goto done;
    }

    // overflow - the number of pending frames exceeds the target. we need to discard frames and poll immediately
    // (note that we take into account some polling mistakes)
    if(_ctx->flow_ctl.trgt_num_pending + 1 < frame_info->num_pending){
        frame_info->poll_interval = 0; // in the case of overflow we assume there is a need to re-sync immediately and empty the buffer
        frame_info->flow_ctl_action = ffngxshm_av_fc_discard_frame;
    }
    else{
        frame_info->flow_ctl_action = ffngxshm_av_fc_use_frame;
    }

done:

    cur_tm = ffngxshm_get_cur_timestamp();
    if(_ctx->flow_ctl.last_underflows_chk_tm + 120000 < cur_tm){
        _ctx->flow_ctl.last_underflows_chk_tm = cur_tm;

        if(_ctx->flow_ctl.num_underflows < 2){
            if(_ctx->flow_ctl.trgt_num_pending > 3)
                _ctx->flow_ctl.trgt_num_pending--;
        }
        else if(_ctx->flow_ctl.num_underflows > 4){
            _ctx->flow_ctl.trgt_num_pending++;
        }

        _ctx->flow_ctl.num_underflows = 0;
    }


    FFNGXSHM_AV_LOG_TRACE(_ctx, "flow control: "
            "num_pending=%ui trgt_num_pending=%ui action=%s frame_dur=%ui",
            frame_info->num_pending, _ctx->flow_ctl.trgt_num_pending, ffngxshm_av_flow_ctl_action_names[frame_info->flow_ctl_action], frame_info->frame_dur);

    return FFNGXSHM_OK;
}

/**
 * allows the client to alter the target num pending threshold of the jitter buffer at run time
 */
int
ffngxshm_set_trgt_num_pending(ffngxshm_av_rd_ctx_t *rd_ctx, uint16_t trgt_num_pending)
{
    _ffngxshm_av_rd_media_ctx_t     *_ctx;

    _ctx   = rd_ctx->rd_ctx;
    _ctx->flow_ctl.trgt_num_pending = trgt_num_pending;
    return FFNGXSHM_OK;
}

int
ffngxshm_read_next_av_audio(ffngxshm_av_rd_ctx_t *rd_ctx, uint64_t max_pts)
{
    ngx_shm_av_chn_header_t         *chn_hd;
    ngx_shm_av_audio_meta_t         audio_meta;
    ngx_shm_seq                     out_seq, seq;
    ngx_stream_shm                  *shm;
    ngx_int_t                       rc;
    _ffngxshm_av_rd_media_ctx_t     *_ctx;
    uint8_t                         *side;
    AVPacket                        *pkt;
    ngx_int_t                       ix;
    ngx_stream_shm_chnk             *chunk;
    ngx_shm_av_audio_chk_header_t   a_hd;
    size_t                          size, flv_hd_size;
    u_char                          *p;
    ngx_uint_t 						chn;
    ngx_av_shm_sync_spec_t          sync_spec;

    _ctx = rd_ctx->rd_ctx;
    shm = ffngxshm_av_get_shm(_ctx);

    if(ngx_stream_shm_is_time_wait(shm)){
        return FFNGXSHM_TIME_WAIT;
    }

    if(ngx_stream_shm_is_closing(shm)){
        return FFNGXSHM_CLOSING;
    }

    pkt = &(rd_ctx->pkt_out);
    chn = ngx_shm_av_get_active_audio_chn_ix(ffngxshm_av_get_avctx(_ctx));

    av_packet_unref(pkt);

    chn_hd = ngx_shm_av_get_channel_header(ffngxshm_av_get_avctx(_ctx), chn);

    __ngx_shm_mem_barrier__
    seq = ngx_shm_volatile(chn_hd->seq);

    // we never read the meta-data before or there is new meta-data to read
    if(!_ctx->last_meta_seq[chn] || seq != _ctx->last_meta_seq[chn] || !_ctx->audio_codec_id){
        // make sure we have encoding parameters and if there are new encoding parameters
        // then add them as side data to the packet
        rc = ngx_shm_av_get_channel_meta(
                ffngxshm_av_get_avctx(_ctx),
                chn,
                &audio_meta, sizeof(ngx_shm_av_audio_meta_t),
                &out_seq);

        if(rc < 0){
            FFNGXSHM_AV_LOG_ERR(_ctx, "failed to get audio channel meta. rc=%d", rc);
            // we are not mapping the returns code to one of FFNGXSHM_XXX since we
            // ensure the codes are the same
            return rc;
        }

        // we cache it in the ctx in order to avoid reading the meta-data for every frame
        _ctx->audio_codec_id = audio_meta.codec_id;

        // in case the last delivered meta_seq is the same as the channel sequence
        // it can be that either we delivered the meta-data or the meta-data was never
        // set for this channel
        if(out_seq == _ctx->last_meta_seq[chn]){
            // meta-data is not ready
            if(!out_seq){
                FFNGXSHM_AV_LOG_WARN(_ctx, "audio meta data is not ready.");
                return FFNGXSHM_EOF;
            }
        }
        // we have new meta-data and the codec is AAC, add it to the packet side data
        else if(audio_meta.codec_id == NGX_SHM_AV_AUDIO_CODEC_AAC && audio_meta.len > 2){
            // we skip the first two bytes which contains the FLV audio tag header and AAC AVC header
            side = av_packet_new_side_data(
                    pkt, AV_PKT_DATA_NEW_EXTRADATA, audio_meta.len - 2);

            if (side) {
                memcpy(side, audio_meta.buf + 2, audio_meta.len - 2);
            }
        }
    }

    // in case we are out of sync with the channel, find the starting point
    if(_ctx->last_audio_ix == NGX_SHM_AV_UNSET_UINT){
        // we are in the middle of the stream, sync based on last DTS
        if(_ctx->last_audio_sq){
            memset(&sync_spec, 0, sizeof(sync_spec));
            sync_spec.pts = _ctx->last_audio_dts;
            sync_spec.mode = NGX_AV_SHM_SYNC_MODE_AFTER_PTS;

            ix = ngx_shm_av_get_sync_channel_index(
                    ffngxshm_av_get_avctx(_ctx), chn, &sync_spec);
        }
        else{
            memset(&sync_spec, 0, sizeof(sync_spec));
            sync_spec.pts = max_pts;
            sync_spec.mode = NGX_AV_SHM_SYNC_MODE_BEFORE_PTS;

            ix = ngx_shm_av_get_sync_channel_index(
                    ffngxshm_av_get_avctx(_ctx), chn, &sync_spec);
        }

        if(ix < 0){
            FFNGXSHM_AV_LOG_WARN(_ctx, "fail to sync. chn=%ui.", chn);
            return FFNGXSHM_EOF;
        }
        chunk = ngx_shm_av_get_chn_chunk(ffngxshm_av_get_avctx(_ctx), ix, chn);

        // acquire the lazy read lock
        seq = chunk->seq_num;
        __ngx_shm_mem_barrier__

        _ctx->last_audio_sq = seq - 1; // make sure the "falling behind" below test will not fail
    }
    else{
        ix = ngx_shm_av_adjust_chn_index(ffngxshm_av_get_avctx(_ctx), _ctx->last_audio_ix + 1, chn);
        chunk = ngx_shm_av_get_chn_chunk(ffngxshm_av_get_avctx(_ctx), ix, chn);

        // acquire the lazy read lock
        seq = chunk->seq_num;
        __ngx_shm_mem_barrier__
    }

    // the chunk sequence number must be greater than zero
    // if it is zero it means it hasn't been used or it has been invalidated
    if(seq == 0){
        FFNGXSHM_AV_LOG_DEBUG(_ctx,
                "found audio chunk with sequence zero mid stream. "
                "chn=%d "
                "oldest_ix=%d oldest_sq=%uD cur_ix=%d cur_sq=%uD "
                "ix=%d lst_sq=%uD nxt_ix=%d ts=%uL",
                chn,
                ngx_shm_av_get_oldest_audio_index(ffngxshm_av_get_avctx(_ctx)),
                (ngx_shm_av_get_oldest_audio_chunk(ffngxshm_av_get_avctx(_ctx)))->seq_num,
                ngx_shm_av_get_cur_audio_index(ffngxshm_av_get_avctx(_ctx)),
                ngx_shm_av_get_cur_audio_seq(ffngxshm_av_get_avctx(_ctx)),
                _ctx->last_audio_ix, _ctx->last_audio_sq, ix,
                ((uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec));

        // invalid sequence mid stream. It most likely happens when the
        // writers invalidate the oldest chunk in order to free data blocks.
        // if the reader falls behind we will try to skip forward to the to oldest chunk
        // and if the oldest chunk is invalid we have to re-sync forward
        if(_ctx->last_audio_sq < ngx_shm_av_get_cur_audio_seq(ffngxshm_av_get_avctx(_ctx))){
            ix = ngx_shm_av_get_chn_oldest_index(ffngxshm_av_get_avctx(_ctx), chn);
            chunk = ngx_shm_av_get_chn_chunk(ffngxshm_av_get_avctx(_ctx), ix, chn);
            FFNGXSHM_AV_LOG_DEBUG(_ctx, "skipping forward to oldest seq. ix=%ui sq=%uL", ix, chunk->seq_num);

            // in this case when we have to skip forward we set the last_audio_ix right a way
            // even if we don't deliver this chunk since there is no point to start again from the
            // current index which was already invalidated. in case we end up no delivering this chunk
            // and not forcing re-sync, we set it so at the next iteration we will start with the same
            // index
            _ctx->last_audio_ix = ngx_shm_av_adjust_chn_index(ffngxshm_av_get_avctx(_ctx), ix - 1, chn);

            // acquire the lazy read lock
            seq = chunk->seq_num;
            __ngx_shm_mem_barrier__

            if(seq == 0 || seq == NGX_SHM_AV_UNSET_UINT || _ctx->last_audio_sq > seq){
                _ctx->last_audio_ix = NGX_SHM_AV_UNSET_UINT; // make sure we re-sync at the next iteration
                return FFNGXSHM_OUT_OF_SYNC;
            }

            _ctx->last_audio_sq = seq - 1; // we set it here to make sure the next test will not fail
        }
        else{
            FFNGXSHM_AV_LOG_DEBUG(_ctx,"chunk not ready. ");
            return FFNGXSHM_EOF;
        }
    }
    else if(seq == NGX_SHM_AV_UNSET_UINT || _ctx->last_audio_sq > seq){
        FFNGXSHM_AV_LOG_DEBUG(_ctx,
                "chunk out of order or not ready. "
                "chn=%ui ix=%d sq=%uL last_sq=%uL",
                chn, ix, seq, _ctx->last_audio_sq);

        return FFNGXSHM_EOF;
    }

    // the reader is falling behind
    if(_ctx->last_audio_sq + 1 < seq){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "reader is falling behind. "
                "chn=%ui ix=%d sq=%uL last_sq=%uL",
                chn, ix, seq, _ctx->last_audio_sq);

        // mark that we are out of sync
        _ctx->last_audio_ix = NGX_SHM_AV_UNSET_UINT;

        return FFNGXSHM_AGAIN;
    }

    // we have to make a copy of the chunk's header to make sure it is not being edited while
    // we read from it
    a_hd = *(ngx_shm_av_get_audio_chunk_header(chunk, ffngxshm_av_get_avctx(_ctx)));

    // too early for this frame, we have to wait
    if(a_hd.src_pts > max_pts){
        FFNGXSHM_AV_LOG_DEBUG(_ctx,"audio pts > max_pts. pts=%uL max_pts=%uL",
                a_hd.src_pts, max_pts);
        return FFNGXSHM_EOF;
    }

    // the data in the shm is muxed as FLV tag (historical reasons). For AAC there is
    // one additional header byte for AACAUDIO header
    flv_hd_size = (_ctx->audio_codec_id == NGX_SHM_AV_AUDIO_CODEC_AAC)? 2 : 1;

    // the chunk must contain at least FLV header size
    if(a_hd.size <= flv_hd_size){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "invalid chunk size. sq=%uL sz=%uL", seq, a_hd.size);
        // mark that we are out of sync and return
        goto out_of_sync;
    }

    if(_ctx->buf.start + a_hd.size > _ctx->buf.end){
        FFNGXSHM_AV_LOG_INFO(_ctx,
                "input buffer too small. increasing. "
                "cur_sz=%uL req_sz=%uL",
                (_ctx->buf.end - _ctx->buf.start), a_hd.size);

        size = (a_hd.size > (_ctx->buf.end - _ctx->buf.start) * 2)?
                a_hd.size : ((_ctx->buf.end - _ctx->buf.start) * 2);

        if(size > FFNGXSHM_DEFAULT_BUF_MAX_SZ){
            FFNGXSHM_AV_LOG_ERR(_ctx, "chunk size too large. re-synching. sz=%uL", a_hd.size);
            // mark that we are out of sync and return
            goto out_of_sync;
        }
        else{
            FFNGXSHM_AV_LOG_INFO(_ctx,
                    "reallocating input buffer. sz=%uL", size);

            p = ngx_palloc(_ctx->pool, size);
            if(!p){
                FFNGXSHM_AV_LOG_ERR(_ctx, "out of memory");
                return FFNGXSHM_ERR;
            }

            ngx_pfree(_ctx->pool, _ctx->buf.start);
            _ctx->buf.start = p;
            _ctx->buf.end = _ctx->buf.start + size;
        }
    }

    _ctx->buf.pos = _ctx->buf.last = _ctx->buf.start;

    // copy the content of the chunk to the given buffer
    if(ngx_stream_shm_copy_chunk(shm, chunk, &_ctx->buf, a_hd.size) != a_hd.size){

        FFNGXSHM_AV_LOG_ERR(_ctx, "failed to read chunk content. "
                "sq=%uL sz=%uL", seq, a_hd.size);
        // mark that we are out of sync and return
        goto out_of_sync;
    }

    // in case the writer overridden the chunk as we read we have
    // to re-sync
    __ngx_shm_mem_barrier__
    if(!ngx_stream_shm_cmp_seq(seq, chunk->seq_num)){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "reader is behind. sq=%uL", seq);
        // mark that we are out of sync and return
        goto out_of_sync;
    }

    _ctx->last_audio_ix = ix;
    _ctx->last_audio_dts = a_hd.src_pts;
    _ctx->last_audio_sq = seq;
    _ctx->buf.last += a_hd.size;

    // discard FLV audio tag header (1 byte format, in case of AAC additional 1 byte)
    pkt->data = _ctx->buf.pos + flv_hd_size;
    pkt->size = a_hd.size - flv_hd_size;
    pkt->pts = pkt->dts = a_hd.src_pts;

    pkt->flags = AV_PKT_FLAG_KEY;

    FFNGXSHM_AV_LOG_DEBUG(_ctx,
            "audio pkt: ix=%d sq=%uL dts=%uL pts=%uL sz=%uL",
            ix, seq, pkt->dts, pkt->pts, pkt->size);

    return FFNGXSHM_OK;

    out_of_sync:
    _ctx->last_audio_ix = NGX_SHM_AV_UNSET_UINT;
    _ctx->last_audio_dts = a_hd.src_pts;
    _ctx->last_audio_sq = seq;
    return FFNGXSHM_AGAIN;
}


/**
 * write the given encoded video frame to the associated shm in the specified channel.
 * The function clones the given packet (it doesn't ref it)
 *
 *
 * NOTE:           AT THE MOMENT WE ASSUME H264 WITHOUT CHECKING!!!
 *
 */
int
ffngxshm_write_av_video(ffngxshm_av_wr_ctx_t *ctx, unsigned int chn, AVPacket *pkt)
{
    _ffngxshm_av_wr_media_ctx_t     *_ctx;
    ngx_shm_av_ctx_t                *av_ctx;
    ngx_stream_shm_chnk             *chunk;
    ngx_int_t                       rc;
    size_t                          size;
    int                             side_size;
    uint8_t                         *side;

    // in case the input packet contains new extra data e.g. SPS/PPS we added it at the
    // beginning of the chunk. the av shm library will parse it and populate the
    // channel's meta data
    static ngx_buf_t                meta_buf = { .last_buf = 1 };
    static ngx_buf_t                data_buf = { .last_buf = 1 };
    static ngx_chain_t              in_data = { .buf = &data_buf, .next = NULL };
    static ngx_chain_t              in_meta = { .buf = &meta_buf, .next = NULL };

    chunk = NULL;

    _ctx = ctx->wr_ctx;
    av_ctx = &_ctx->av_ctx;

    side_size = 0;
    side = av_packet_get_side_data(pkt, AV_PKT_DATA_NEW_EXTRADATA, &side_size);

    data_buf.start = data_buf.pos = (u_char*)pkt->data;
    data_buf.end = data_buf.last = (u_char*)pkt->data + pkt->size;


    /**************************************************************************
     *
     *                  WE ASSUME NO B-FRAMES i.e. CTS == ZERO
     *
     * if that is not the case, we must pass cts to ngx_shm_av_append_video_data
     *
     *************************************************************************/
    if(pkt->dts != pkt->pts){
        FFNGXSHM_AV_LOG_ERR(_ctx, "b-frames are not supported!!!. dts=%uL pts=%uL", pkt->dts, pkt->pts);
    }

    // in case the packet contains new extra data i.e. SPS/PPS, then add it to the packet first
    if (side && side_size > 0){
        meta_buf.start = meta_buf.pos = side;
        meta_buf.end = meta_buf.last = side + side_size;
        rc = ngx_shm_av_append_video_data(av_ctx, pkt->dts, chn, &in_meta, &chunk, &size,
                NGX_SHM_AV_FLAGS_MUX_TYPE_H264_ANNEXB, NGX_SHM_AV_VIDEO_CODEC_H264);

        if(rc < 0){
            FFNGXSHM_AV_LOG_ERR(_ctx, "failed to append data to chunk. "
                    "chn=%ui dts=%uL pts=%uL size=%uL rc=%d", chn, pkt->dts, pkt->pts, size, rc);

            return FFNGXSHM_ERR;
        }

        FFNGXSHM_AV_LOG_TRACE(_ctx, "append meta data. "
        		"chn=%ui dts=%uL pts=%uL size=%uL rc=%d", chn, pkt->dts, pkt->pts, size, rc);
    }

    rc = ngx_shm_av_append_video_data(av_ctx, pkt->dts, chn, &in_data, &chunk, &size,
            NGX_SHM_AV_FLAGS_MUX_TYPE_H264_ANNEXB, NGX_SHM_AV_VIDEO_CODEC_H264);

    if(rc < 0){
        FFNGXSHM_AV_LOG_ERR(_ctx, "failed to append data to chunk. "
                "chn=%ui dts=%uL pts=%uL size=%uL rc=%d", chn, pkt->dts, pkt->pts, size, rc);

        return FFNGXSHM_ERR;
    }

    // update chunk headers by parsing AVC headers
    if (ngx_shm_av_video_avc_update_chunk_header(av_ctx, chn, chunk) != FFNGXSHM_OK) {
    	return FFNGXSHM_ERR;
    }

    // publish the new chunk
    if(ngx_shm_av_append_video_chunk(av_ctx, pkt->dts, chn, chunk, size) == NULL){
        return FFNGXSHM_ERR;
    }

    FFNGXSHM_AV_LOG_DEBUG(_ctx, "video: chn=%ui sq=%uL dts=%uL pts=%uL pkt->size=%uL sz=%uL kf=%ui. ",
            chn, chunk->seq_num, pkt->dts, pkt->pts, pkt->size, size, (pkt->flags & AV_PKT_FLAG_KEY));

    return FFNGXSHM_OK;
}


/**
 * For the moment we assume the audio encoding is AAC therefore we process the AAC in here
 * in the future that might not be the case and those function will have to be moved somewhere
 * else
 *
 * this function take as input AAC AudioSpecificConfig and fills up the av shm meta struct that
 * can be set on the audio channel
 *
 * See ISO 14496-3 AudioSpecificConfig
 */
static int
ffngxshm_av_parse_aac_header(
        _ffngxshm_av_wr_media_ctx_t *_ctx, uint8_t *data, size_t size, ngx_shm_av_audio_meta_t *a_meta)
{
    int        num_bits;
    uint64_t   bits, bits2;
    int        i,j;

    static ngx_uint_t      aac_sample_rates[] =
    { 96000, 88200, 64000, 48000,
            44100, 32000, 24000, 22050,
            16000, 12000, 11025,  8000,
            7350,     0,     0,     0 };

    if(size < 2){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "AAC audio headers must be at least 2 bytes long. sz=%ui", size);
        return FFNGXSHM_ERR;
    }

    if(size + 2 > sizeof(a_meta->buf)){
        FFNGXSHM_AV_LOG_ERR(_ctx,
                "AAC audio headers too large. sz=%ui", size);
        return FFNGXSHM_ERR;
    }

    bits = 0;
    for(i = 0;i < 5 && i < size;i++){
        bits += (((uint64_t)data[i]) << ((7 - i) * 8));
    }
    num_bits = i * 8;

    a_meta->spf          = 1024; // fixed for AAC by the standard
    a_meta->codec_id     = NGX_SHM_AV_AUDIO_CODEC_AAC;
    a_meta->sample_size  = 16; // it is ignored

    // 5 bits
    a_meta->aac_objtype = (ngx_uint_t) ((bits & 0xF800000000000000) >> 59);
    num_bits -= 5;
    bits <<= 5;
    if (a_meta->aac_objtype == 31) {
        // 6 bits
        a_meta->aac_objtype = ((ngx_uint_t) ((bits & 0xFC00000000000000) >> 58)) + 32;
        num_bits -= 6;
        bits <<= 6;
    }

    // 4 bits
    a_meta->aac_srindex = (ngx_uint_t) ((bits & 0xF000000000000000) >> 60);
    num_bits -= 4;
    bits <<= 4;
    if (a_meta->aac_srindex == 15) {
        if(num_bits < 24){
            FFNGXSHM_AV_LOG_ERR(_ctx, "srindex == 15. not enough data. sz=%ui", size);
            return FFNGXSHM_ERR;
        }
        // 24 bits
        a_meta->sr = a_meta->aac_srindex =
                (ngx_uint_t) ((bits & 0xFFFFFF0000000000) >> 40);
        num_bits -= 24;
        bits <<= 24;

    } else {
        a_meta->sr = aac_sample_rates[a_meta->aac_srindex];
    }

    if(num_bits < 4){
        FFNGXSHM_AV_LOG_ERR(_ctx, "not enough data. sz=%ui", size);
        return FFNGXSHM_ERR;
    }
    // 4 bits
    a_meta->chn = a_meta->aac_chn_conf = (ngx_uint_t) ((bits & 0xF000000000000000) >> 60);
    num_bits -= 4;
    bits <<= 4;

    if (a_meta->aac_objtype == 5 || a_meta->aac_objtype == 29) {

        bits2 = 0;
        for(j = i;j < 5 && j < size;j++){
            bits2 += (((uint64_t)data[j]) << ((7 - j + i) * 8));
        }
        bits2 >>= num_bits;
        bits |= bits2;
        num_bits += (j - i) * 8;

        if(num_bits < 4){
            FFNGXSHM_AV_LOG_ERR(_ctx, "not enough data. sz=%ui", size);
            return FFNGXSHM_ERR;
        }
        a_meta->aac_srindex = (ngx_uint_t) ((bits & 0xF000000000000000) >> 60);
        num_bits -= 4;
        bits <<= 4;

        if (a_meta->aac_srindex == 15) {
            if(num_bits < 24){
                FFNGXSHM_AV_LOG_ERR(_ctx, "srindex == 15. not enough data. sz=%ui", size);
                return FFNGXSHM_ERR;
            }
            // 24 bits
            a_meta->sr = a_meta->aac_srindex =
                    (ngx_uint_t) ((bits & 0xFFFFFF0000000000) >> 40);
            num_bits -= 24;
            bits <<= 24;

        } else {
            a_meta->sr = aac_sample_rates[a_meta->aac_srindex];
        }

        // 5 bits
        if(num_bits < 5){
            FFNGXSHM_AV_LOG_ERR(_ctx, "not enough data. sz=%ui", size);
            return FFNGXSHM_ERR;
        }

        a_meta->aac_objtype = (ngx_uint_t) ((bits & 0xF800000000000000) >> 59);
        num_bits -= 5;
        bits <<= 5;
        if (a_meta->aac_objtype == 31) {
            if(num_bits < 6){
                FFNGXSHM_AV_LOG_ERR(_ctx, "not enough data. sz=%ui", size);
                return FFNGXSHM_ERR;
            }

            // 6 bits
            a_meta->aac_objtype = ((ngx_uint_t) ((bits & 0xFC00000000000000) >> 58)) + 32;
        }
    }

    a_meta->buf[0] = 0xAF; // FLV audio tag header for AAC
    a_meta->buf[1] = 0;    // Indicates AAC FLV Tag meta-data (sequence header)
    memcpy(&a_meta->buf[2], data, size);
    a_meta->len = 2 + size;


    return FFNGXSHM_OK;
}



/**
 * write the given encoded audio frame to the associated shm in the specified channel.
 * The function clones the given packet (it doesn't ref it)
 *
 * NOTE:           AT THE MOMENT WE ASSUME AAC WITHOUT CHECKING!!!
 *
 */
int
ffngxshm_write_av_audio(ffngxshm_av_wr_ctx_t *ctx, unsigned int chn, enum AVCodecID codecid, AVPacket *pkt)
{
    _ffngxshm_av_wr_media_ctx_t     *_ctx;
    ngx_shm_av_ctx_t                *av_ctx;
    ngx_shm_av_chn_header_t         *chn_hd;
    ngx_stream_shm_chnk             *chunk;
    ngx_shm_av_audio_chk_header_t   *audio_header;
    ngx_int_t                       rc;
    size_t                          size;
    ngx_shm_av_audio_meta_t         a_meta;
    int                             side_size;
    uint8_t                         *side;

    // used for wrapping the input data as linked list of buffers. For backward compatibility reasons
    // we mux the data as FLV tag
    static u_char                   flv_aac_hd[2]    = { 0xAF, 0x01 }; // FLV audio tag header for AAC
    static ngx_buf_t                flv_aac_hd_buf   = { .start = flv_aac_hd, .pos = flv_aac_hd, .end = flv_aac_hd + sizeof(flv_aac_hd), .last = flv_aac_hd + sizeof(flv_aac_hd)};

    static u_char                   flv_opus_hd[1]    = { 0x9F }; // FLV audio tag header for OPUS
    static ngx_buf_t                flv_opus_hd_buf   = { .start = flv_opus_hd, .pos = flv_opus_hd, .end = flv_opus_hd + sizeof(flv_opus_hd), .last = flv_opus_hd + sizeof(flv_opus_hd)};

    static ngx_buf_t                flv_data_buf     = { .last_buf = 1 };
    static ngx_chain_t              in_data          = { .buf = &flv_data_buf };
    static ngx_chain_t              in_hd            = { .buf = &flv_aac_hd_buf, .next = &in_data };

    _ctx = ctx->wr_ctx;
    av_ctx = &_ctx->av_ctx;

    if (codecid == AV_CODEC_ID_AAC) {
    	in_hd.buf = &flv_aac_hd_buf;

        side_size = 0;
        side = av_packet_get_side_data(pkt, AV_PKT_DATA_NEW_EXTRADATA, &side_size);

        // in case the packet contains new extra i.e. AAC AudioSpecificConfig
        // we first check if meta-data on the channel wasn't set. in case this is the first
        // time we receive meta-data then create new audio meta and set it on the channel
        if (side && side_size > 0){
            chn_hd = ngx_shm_av_get_channel_header(av_ctx, chn);
            if(!chn_hd->seq){
                memset(&a_meta, 0, sizeof(a_meta));
                rc = ffngxshm_av_parse_aac_header(_ctx, side, side_size, &a_meta);

                if(rc < 0){
                    FFNGXSHM_AV_LOG_ERR(_ctx, "failed to parse AAC parameters");
                    return FFNGXSHM_ERR;
                }
                else {
                    FFNGXSHM_AV_LOG_DEBUG(_ctx, "parsed AAC parameters: a_meta spf %ui sr %ui chn %ui sample_size %ui buf_len %ui",
                            a_meta.spf, a_meta.sr, a_meta.chn, a_meta.sample_size, a_meta.len);
                }

                a_meta.pts = pkt->pts;

                rc = ngx_shm_av_update_audio_meta(av_ctx, &a_meta, chn);
            }
        }
    } else if (codecid == AV_CODEC_ID_OPUS) {
    	in_hd.buf = &flv_opus_hd_buf;

        chn_hd = ngx_shm_av_get_channel_header(av_ctx, chn);
        if(!chn_hd->seq){
            memset(&a_meta, 0, sizeof(a_meta));
            a_meta.pts = pkt->pts;
            a_meta.codec_id = NGX_SHM_AV_AUDIO_CODEC_OPUS;
            a_meta.chn = 2;          // fixed by rfc7587 section 7
            a_meta.sr = 48000;       // fixed by rfc7587 section 7
            a_meta.sample_size = 16; // unused don't care
            a_meta.spf = 960;        // unused don't care

            rc = ngx_shm_av_update_audio_meta(av_ctx, &a_meta, chn);
        }
    } else {
    	FFNGXSHM_AV_LOG_ERR(_ctx, "unsupported codecid %d", codecid);
    	return FFNGXSHM_ERR;
    }

    // populate the new chunk with audio data from the packet
    flv_data_buf.start = flv_data_buf.pos = pkt->data;
    flv_data_buf.end = flv_data_buf.last = pkt->data + pkt->size;

    rc = ngx_shm_av_append_audio_data(av_ctx, pkt->pts, chn, &in_hd, &chunk, &size);

    if(rc < 0){
        FFNGXSHM_AV_LOG_ERR(_ctx, "failed to append audio data to chunk");
        return FFNGXSHM_ERR;
    }

    // publish the new chunk
    if(ngx_shm_av_append_audio_chunk(av_ctx, pkt->pts, chn, chunk, size) == NULL){
        FFNGXSHM_AV_LOG_ERR(_ctx, "failed to publish audio data");
        return FFNGXSHM_ERR;
    }

    return FFNGXSHM_OK;
}



/**
 * Access params is an opque field that is set by the application in the shared memory and
 * helps to control the access to the stream i.e. determine if the access is public or require
 * authorization. The transcoder copy the value of the access parameters as is from the source all
 * the way out to the encoded stream. This makes it easier to enforce stream acceess accross all
 * variants and replication by simply copy the value from the source all the way to the border
 */
int
ffngxshm_get_av_acess_params(ffngxshm_av_rd_ctx_t *ctx, ffngxshm_access_param *out)
{
    _ffngxshm_av_rd_media_ctx_t *_ctx;
    _ctx = ctx->rd_ctx;
    *out = (ffngxshm_access_param) ngx_stream_shm_get_acc_param(ffngxshm_av_get_shm(_ctx));
    return FFNGXSHM_OK;
}

int
ffngxshm_set_av_acess_params(ffngxshm_av_wr_ctx_t *ctx, ffngxshm_access_param access_param)
{
    _ffngxshm_av_wr_media_ctx_t     *_ctx;
    _ctx = ctx->wr_ctx;
    ngx_stream_shm_set_acc_param(ffngxshm_av_get_shm(_ctx), access_param);
    return FFNGXSHM_OK;
}
