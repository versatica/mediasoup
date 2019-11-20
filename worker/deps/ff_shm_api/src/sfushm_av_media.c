/*
 * sfushm_av_media.c
 *
 *  Created on: Nov 2, 2019
 *      Author: Amir Pauker
 *
 * implementation of the library used by SFU to write incoming stream over RTP to
 * SHM that will allow xcode to read from
 */
#include <stdlib.h>
#include <inttypes.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_log.h>

#include <ngx_stream_shm.h>
#include <ngx_shm_av.h>

#include "sfushm_av_media.h"


// makes sure codec id values are identical in sfushm_av_media.h and ngx_shm_av.h
// -------------------------------------------------------------------------------
#if SFUSHM_AV_VIDEO_CODEC_H264 != NGX_SHM_AV_VIDEO_CODEC_H264
#error SFUSHM_AV_VIDEO_CODEC_H264 != NGX_SHM_AV_VIDEO_CODEC_H264
#endif
#if SFUSHM_AV_VIDEO_CODEC_VP8 != NGX_SHM_AV_VIDEO_CODEC_VP8
#error SFUSHM_AV_VIDEO_CODEC_VP8 != NGX_SHM_AV_VIDEO_CODEC_VP8
#endif
#if SFUSHM_AV_AUDIO_CODEC_OPUS != NGX_SHM_AV_AUDIO_CODEC_OPUS
#error SFUSHM_AV_AUDIO_CODEC_OPUS != NGX_SHM_AV_AUDIO_CODEC_OPUS
#endif
#if SFUSHM_AV_AUDIO_CODEC_AAC != NGX_SHM_AV_AUDIO_CODEC_AAC
#error SFUSHM_AV_AUDIO_CODEC_AAC != NGX_SHM_AV_AUDIO_CODEC_AAC
#endif


static void _sfushm_av_log(ngx_log_t *log, unsigned int level, const char* str, ...);

#define sfushm_av_get_avctx(ctx) (ngx_shm_av_ctx_t*)&((ctx)->av_ctx)
#define sfushm_av_get_shm(ctx) &(sfushm_av_get_avctx(ctx))->shm
#define sfushm_av_get_stream_name(ctx) ngx_shm_get_stream_name(sfushm_av_get_shm(ctx))

#define sfushm_av_log(ctx, level, str, ...) \
	if((ctx)->log->log_level >= level) _sfushm_av_log(&(ctx)->log, level, str, __VA_ARGS__)

#define SFUSHM_AV_LOG_INFO(ctx,str,...)                                             \
        sfushm_av_log(ctx, SFUSHM_AV_LOG_LEVEL_INFO, "- sfushm - %s %d - %s - " str,     \
                __FUNCTION__,__LINE__,                                            \
                ((ctx && sfushm_av_get_stream_name(ctx))? sfushm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define SFUSHM_AV_LOG_ERR(ctx,str,...)                                             \
        sfushm_av_log(ctx, SFUSHM_AV_LOG_LEVEL_ERR, "- sfushm - %s %d - %s - " str,     \
                __FUNCTION__,__LINE__,                                            \
                ((ctx && sfushm_av_get_stream_name(ctx))? sfushm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define SFUSHM_AV_LOG_WARN(ctx,str,...)                                             \
        sfushm_av_log(ctx, SFUSHM_AV_LOG_LEVEL_WARN, "- sfushm - %s %d - %s - " str,     \
                __FUNCTION__,__LINE__,                                            \
                ((ctx && sfushm_av_get_stream_name(ctx))? sfushm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define SFUSHM_AV_LOG_DEBUG(ctx,str,...)                                            \
        sfushm_av_log(ctx, SFUSHM_AV_LOG_LEVEL_DEBUG, "- sfushm - %s %d - %s - " str,    \
                __FUNCTION__,__LINE__,                                             \
                ((ctx && sfushm_av_get_stream_name(ctx))? sfushm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)

#ifdef NGX_DEBUG
#define SFUSHM_AV_LOG_TRACE(ctx,str,...)                                            \
        sfushm_av_log(ctx, SFUSHM_AV_LOG_LEVEL_DEBUG, "- sfushm - %s %d - %s - " str,    \
                __FUNCTION__,__LINE__,                                             \
                ((ctx && sfushm_av_get_stream_name(ctx))? sfushm_av_get_stream_name(ctx):"na"),##__VA_ARGS__)
#else
#define SFUSHM_AV_LOG_TRACE(ctx,str,...)
#endif


typedef struct {
	av_pts_t          last_ntp_unixtime; // most recent ntp time received by RTCP sender
	                                     // report represented as unix timestamp in milliseconds
	av_pts_t          last_rtp_time;     // the RTP time associated with the most recent RTCP sender report
	                                     // it is represented as 64 bits to account for overflow
	uint32_t          max_rtp_seq;       // store the max rtp sequence as 32 bits to account for overflow
	uint32_t          ssrc;              // source RTP SSRC. Used for correlating with RTCP messages
	uint32_t          sample_rate;       // clock sample rate for the this channel
	uint32_t          cur_chunk_ix;      

	uint8_t           codec_id;          // codec id as one of SFUSHM_AV_VIDEO_CODEC_XXX constants
	uint8_t           num_chn;           // audio number of channels

} _sfushm_av_chn_ctx_t;

/**
 * internal definition of the write context (MUTS NOT BE exposed to user of this library)
 */
typedef struct {
	ngx_shm_av_ctx_t       av_ctx;           // nginx shm av context
	_sfushm_av_chn_ctx_t   video_ctx;        // video channel context
	_sfushm_av_chn_ctx_t   audio_ctx;        // audio channel context
    ngx_pool_t             *pool;            // memory pool to be used by this context only (destroyed when the ctx is closed)
    ngx_log_t              *log;              // log for this stream
} _sfushm_av_ctx_t;



static void
sfushm_av_log_destroy(ngx_log_t *log)
{
	if (!log) return;

	if (log->file && log->file->fd > 2) {
		if(close(log->file->fd) < 0){
			fprintf(stderr, "sfushm_av_log_destroy - fail to close file. fd=%d %s\n",
					log->file->fd, strerror(errno));
		}
		log->file->fd = -1;

		free(log->file->name.data);
		log->file->name.data = NULL;
		free(log->file);
		log->file = NULL;
	}

	free(log);
}

/**
 * allocate and initializes a log object
 * IMPORTANT NOTE: the function allocates memory using malloc therefore the returned log
 *                 must be closed using sfushm_av_log_destroy
 */
static int
sfushm_av_log_init(
        const char* filename,
        unsigned int level,
        unsigned int redirect_stdio,
		ngx_log_t **log_out)
{
	ngx_log_t        *log;

	*log_out = NULL;

	log = calloc(sizeof(ngx_log_t), 1);
	if(!log){
		fprintf(stderr, "_sfushm_av_log_init - fail to open log '%s'. "
				"out of memory. %s\n", filename, strerror(errno));
		return SFUSHM_AV_ERR;
	}

	log->file = calloc(sizeof(ngx_open_file_t), 1);
	if(!log->file){
		fprintf(stderr, "_sfushm_av_log_init - fail to open log '%s'. "
				"out of memory. %s\n", filename, strerror(errno));
		goto fail;
	}

	log->file->name.len = strlen(filename) + 1;
	log->file->name.data = malloc(log->file->name.len);
    if(!log->file->name.data){
        fprintf(stderr, "_sfushm_av_log_init - fail to open log '%s'. "
        		"out of memory. %s\n", filename, strerror(errno));
        goto fail;
    }

    strcpy(log->file->name.data, filename);

    log->file->fd = open(filename,
    		O_WRONLY|O_APPEND|O_CREAT,
    		S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);

    if(log->file->fd < 0){
    	fprintf(stderr, "_sfushm_av_log_init - fail to open log '%s'.  %s\n",
    			filename, strerror(errno));
    	goto fail;
    }

    log->log_level = level;
    log->disk_full_time = ngx_time() + 1;

    if(redirect_stdio){
        dup2(log->file->fd, 1);
        dup2(log->file->fd, 2);
    }

    return SFUSHM_AV_OK;

fail:
	sfushm_av_log_destroy(log);
	return SFUSHM_AV_ERR;
}



static void
_sfushm_av_log(ngx_log_t *log, unsigned int level, const char* str, ...)
{
	va_list      args;
    u_char       *p, *last, *msg;
    ssize_t      n;
    u_char       errstr[NGX_MAX_ERROR_STR];

    last = errstr + NGX_MAX_ERROR_STR;

    p = ngx_cpymem(errstr, ngx_cached_err_log_time.data,
                   ngx_cached_err_log_time.len);

    p = ngx_slprintf(p, last, " [%s] ", errstr[level]);

    p = ngx_slprintf(p, last, "%P#0: ", ngx_pid);

    // numeric unique identifier of the session associated with this log
    if (log->connection) {
        p = ngx_slprintf(p, last, ".%uA ", log->connection);
    }

    msg = p;

    va_start(args, str);
    p = ngx_vslprintf(p, last, str, args);
    va_end(args);

    if (p > last - NGX_LINEFEED_SIZE) {
        p = last - NGX_LINEFEED_SIZE;
    }

    ngx_linefeed(p);

    if (log->writer) {
        log->writer(log, level, errstr, p - errstr);
        return;
    }

    if (ngx_time() == log->disk_full_time) {

        /*
         * on FreeBSD writing to a full filesystem with enabled soft updates
         * may block process for much longer time than writing to non-full
         * filesystem, so we skip writing to a log for one second
         */

        return;
    }

    n = ngx_write_fd(log->file->fd, errstr, p - errstr);

    if (n == -1 && ngx_errno == NGX_ENOSPC) {
        log->disk_full_time = ngx_time();
    }
}


/**
 * Takes as input an sfushm_av_writer_init_t which is used as an abstraction to the shm real conf structure
 * and fill in the shm make it ready for calling ngx_stream_shm_cre
 */
static int
sfushm_av_create_shm_conf(
		_sfushm_av_ctx_t *_ctx,
		sfushm_av_writer_init_t *init,
		ngx_shm_av_conf_t *av_conf)
{
    ngx_uint_t               chn;
    char                     *stream_name;
    sfushm_av_conf_t         *conf;
    ngx_stream_shm_conf_t    *shm_conf;
    uint32_t                 target_fps;


    stream_name = init->stream_name;
    conf = &init->conf;

    memset(av_conf, 0, sizeof(*av_conf));

    av_conf->flags = NGX_SHM_AV_FLAGS_MUX_TYPE_RTP_PAYLOAD;
    av_conf->stats_win_size = init->stats_win_size;
    shm_conf = &av_conf->shm_conf;

    for(chn = 0;chn < SFUSHM_AV_MAX_NUM_CHANNELS;chn++)
    {
        if((conf->channels[chn].video + conf->channels[chn].audio) != 1) {
        	SFUSHM_AV_LOG_ERR(_ctx, "invalid channel conf. chn=%d name=%s "
        			"video=%d audio=%d",
                    chn, stream_name, conf->channels[chn].video, conf->channels[chn].audio);

            return SFUSHM_AV_ERR;
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

        // for video we assume standard 30 fps
        // for audio we assume 20 ms frame which is common for OPUS
        target_fps = conf->channels[chn].video ? 30 : 50;

        // number of frames to store in memory for this channel
        shm_conf->chncf[chn].shm_num_chks =
                (conf->channels[chn].target_buf_ms * target_fps) / 1000 + 1;

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
        // note that target_buf_ms is expressed in milliseconds therefore
        // at the end we divide by 1000 to convert to seconds
        shm_conf->chncf[chn].shm_blk_size =
                ((double)conf->channels[chn].target_kbps / 8) * 1024 * conf->channels[chn].target_buf_ms /
                shm_conf->chncf[chn].shm_num_blks / 1000;

        shm_conf->chncf[chn].shm_blk_size = ngx_stream_shm_block_size_memalign(shm_conf->chncf[chn].shm_blk_size);
    }

    return SFUSHM_AV_OK;
}


/**
 * Creates an av writer context (for encoded data that comes from RTP source e.g. SFU),
 * open the specified shared memory for writing and associate the shm context with
 * the new context. If the function succeeds it returns the allocated context in ctx_out,
 * otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using sfushm_av_close_writer.
 */
int
sfushm_av_open_writer(
		sfushm_av_writer_init_t *init, sfushm_av_wr_ctx_t **ctx_out)
{
    ngx_pool_t                *pool;
    _sfushm_av_ctx_t          *_ctx;
    ngx_log_t                 *log;
    ngx_str_t                 stream_name;
    ngx_shm_av_conf_t         av_conf;
    ngx_int_t                 errno_out;
    ngx_int_t                 rc;
    ngx_uint_t                chn;

    _ctx = NULL;
    *ctx_out = NULL;

    rc = sfushm_av_log_init(
    		init->conf.log_file_name, init->conf.log_level, init->conf.redirect_stdio, &log);

    if (rc != SFUSHM_AV_OK) {
    	return rc;
    }

    pool = ngx_create_pool(4096, log);

    if(!pool){
    	fprintf(stderr, "sfushm_av_open_writer - out of memory");
    	goto fail;
    }

    *ctx_out = ngx_pcalloc(pool, sizeof(sfushm_av_wr_ctx_t));
    if(!ctx_out){
    	fprintf(stderr, "sfushm_av_open_writer - out of memory");
        goto fail;
    }

    _ctx = ngx_pcalloc(pool, sizeof(_sfushm_av_ctx_t));
    if(!_ctx){
    	fprintf(stderr, "sfushm_av_open_writer - out of memory");
        goto fail;
    }

    for(chn = 0;chn < SFUSHM_AV_MAX_NUM_CHANNELS;chn++)
    {
        if (init->conf.channels[chn].video) {
        	if (_ctx->video_ctx.codec_id) {
            	SFUSHM_AV_LOG_ERR(_ctx, "invalid config. multiple video channels. "
            			"chn=%d name=%s ", chn, stream_name);
            	goto fail;
        	}

        	_ctx->video_ctx.codec_id    = init->conf.channels[chn].codec_id;
        	_ctx->video_ctx.sample_rate = init->conf.channels[chn].sample_rate;
        	_ctx->video_ctx.ssrc        = init->conf.channels[chn].ssrc;

        } else if (init->conf.channels[chn].audio) { // audio
        	if (_ctx->audio_ctx.codec_id) {
            	SFUSHM_AV_LOG_ERR(_ctx, "invalid config. multiple audio channels. "
            			"chn=%d name=%s ", chn, stream_name);
            	goto fail;
        	}

        	_ctx->audio_ctx.codec_id    = init->conf.channels[chn].codec_id;
        	_ctx->audio_ctx.sample_rate = init->conf.channels[chn].sample_rate;
        	_ctx->audio_ctx.ssrc        = init->conf.channels[chn].ssrc;
        	_ctx->audio_ctx.num_chn     = init->conf.channels[chn].num_chn;
        }
    }

    // create the config descriptor for initialization of the shared memory structure
    // this config describes the list of enabled channels and their layout
    if (sfushm_av_create_shm_conf(_ctx, init, &av_conf) != SFUSHM_AV_OK) {
    	SFUSHM_AV_LOG_ERR(_ctx, "fail to create shm conf");
    	goto fail;
    }

    stream_name.data = (u_char*)init->stream_name;
    stream_name.len = strlen(init->stream_name);

    if(ngx_shm_av_init_writer_ctx(
            &_ctx->av_ctx, &av_conf, pool, log, stream_name, rtp,
            (uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec,
            &errno_out) < 0){
        SFUSHM_AV_LOG_ERR(_ctx, "fail to create shm '%s' errno=%d", init->stream_name, errno_out);
        goto fail;
    }

    (*ctx_out)->wr_ctx = _ctx;
    _ctx->pool = pool;

    _ctx->video_ctx.cur_chunk_ix = NGX_SHM_AV_UNSET_UINT;
    _ctx->audio_ctx.cur_chunk_ix = NGX_SHM_AV_UNSET_UINT;

    // initialize the wall clock based on server time until we get the first RTCP sender report
    _ctx->video_ctx.last_ntp_unixtime = NGX_SHM_AV_UNSET_PTS;
    _ctx->audio_ctx.last_ntp_unixtime = NGX_SHM_AV_UNSET_PTS;
    _ctx->video_ctx.last_rtp_time     = NGX_SHM_AV_UNSET_PTS;
    _ctx->audio_ctx.last_rtp_time     = NGX_SHM_AV_UNSET_PTS;

    SFUSHM_AV_LOG_INFO(_ctx, "successfully created sfu writer ctx");

    return SFUSHM_AV_OK;

fail:
	if (pool) ngx_destroy_pool(pool);
    sfushm_av_log_destroy(log);
    *ctx_out = NULL;
    return SFUSHM_AV_ERR;
}

int
ffngxshm_close_av_writer(sfushm_av_wr_ctx_t *ctx, int time_wait)
{
    _sfushm_av_ctx_t     *_ctx;
    ngx_log_t            *log;

    _ctx = ctx->wr_ctx;

    if(_ctx && time_wait && ngx_stream_shm_is_writer_ready(sfushm_av_get_shm(_ctx))){
        SFUSHM_AV_LOG_INFO(_ctx, "marking shm time_wait");
        ngx_stream_shm_mark_as_time_wait(sfushm_av_get_shm(_ctx));
    }

    SFUSHM_AV_LOG_INFO(_ctx, "closing writer ctx");

    ngx_shm_av_close_ctx(sfushm_av_get_avctx(_ctx));

    log = _ctx->log;
    ngx_destroy_pool(_ctx->pool);
    sfushm_av_log_destroy(log);

    return SFUSHM_AV_OK;
}

// adjust uint32_t for overflow using unint64_t as base
static inline void
sfushm_av_adjust_for_overflow_32_64(uint64_t cur, uint64_t *val, uint32_t max_delta)
{
	if (*val < (cur & 0xFFFFFFFF)) {
		if ( (cur & 0xFFFFFFFF) + max_delta > (*val & 0xFFFFFFFF) + 0x100000000 ) {
			*val = (cur & 0xFFFFFFFF00000000) + 0x100000000 + (*val & 0xFFFFFFFF);
		}
	}
}

static av_pts_t
sfushm_av_rtp_time_to_unix_timestamp(_sfushm_av_chn_ctx_t *chn_ctx, av_pts_t rtp_time)
{
	if (chn_ctx->last_rtp_time == NGX_SHM_AV_UNSET_PTS) {
		chn_ctx->last_rtp_time = rtp_time;
		chn_ctx->last_ntp_unixtime = (uint64_t)(ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec;
		return ((double)rtp_time / chn_ctx->sample_rate) * 1000; // convert RTP time to ms
	} else {
		return chn_ctx->last_ntp_unixtime +
				(((double)rtp_time - (double)chn_ctx->last_rtp_time) / chn_ctx->sample_rate) * 1000;
	}
}

int
sfushm_av_write_video(sfushm_av_wr_ctx_t *ctx, sfushm_av_frame_frag_t *data)
{
	_sfushm_av_ctx_t                *_ctx;
	_sfushm_av_chn_ctx_t            *chn_ctx;
	ngx_stream_shm_chnk             *chunk;
	ngx_shm_av_video_chk_header_t   *video_header;
	av_pts_t                        adj_rtp_tm;
	ngx_int_t                       rc;
	ngx_uint_t                      chn;
	size_t                          size;
	//ngx_shm_h264_desc               h264_desc;
	ngx_shm_av_video_meta_t         v_meta;
	ngx_shm_seq                     seq;

    static ngx_buf_t                data_buf = { .last_buf = 1 };
    static ngx_chain_t              in_data = { .buf = &data_buf, .next = NULL };

	_ctx    = ctx->wr_ctx;
	chn_ctx = &_ctx->video_ctx;
	chn     = ngx_shm_av_get_active_video_chn_ix(sfushm_av_get_avctx(_ctx));

	if (data->first_rtp_seq < chn_ctx->max_rtp_seq) {
		SFUSHM_AV_LOG_ERR(_ctx, "invalid seq. "
				"chn=%ui "
				"data: time=%uL first_sq=%ui last_sq=%ui "
				"ctx: max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
				chn,
				data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
				chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
		return SFUSHM_AV_INVALID_SEQ;
	}

	adj_rtp_tm = data->rtp_time;

	// adjust for int overflow allowing max gap of 16 sec
	sfushm_av_adjust_for_overflow_32_64(chn_ctx->last_rtp_time, &adj_rtp_tm, chn_ctx->sample_rate * 16);

	// convert the RTP time to ms and offset based on the NTP timestamp that is received over RTCP
	adj_rtp_tm = sfushm_av_rtp_time_to_unix_timestamp(chn_ctx, adj_rtp_tm);

	data_buf.pos = data_buf.start = (u_char*)data->data;
	data_buf.last = data_buf.end = data_buf.start + data->len;

	chunk        = ngx_shm_av_get_chn_cur_chunk(sfushm_av_get_avctx(_ctx), chn);
	seq          = chunk->seq_num;

	// in case we have an open chunk
	if (seq == NGX_SHM_UNSET_UINT) {
		// if this fragment is the beginning of a new chunk it means we discard the previous one
		if (data->begin) {
			SFUSHM_AV_LOG_DEBUG(_ctx, "discarding video chunk in the middle. "
					"chn=%ui "
					"data: time=%uL first_sq=%ui last_sq=%ui "
					"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
					chn,
					data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
					seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);

			chunk = NULL; // force re-open this chunk i.e. free all the data that was copy to it
		}
	} else if (!data->begin) { // we don't have an open chunk but this is not the start of a chunk
		SFUSHM_AV_LOG_ERR(_ctx, "no open chunk to append video data to. "
				"chn=%ui "
				"data: time=%uL first_sq=%ui last_sq=%ui "
				"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
				chn,
				data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
				seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
		return SFUSHM_AV_ERR;
	} else {
		chunk = NULL; // force opening new chunk
	}

	rc = ngx_shm_av_append_video_data(
			sfushm_av_get_avctx(_ctx),
			adj_rtp_tm,
			chn,
			&in_data,
			&chunk,
			&size,
			NGX_SHM_AV_FLAGS_MUX_TYPE_RTP_PAYLOAD,
			chn_ctx->codec_id);

	if (rc != SFUSHM_AV_OK || size != data->len) {
		SFUSHM_AV_LOG_ERR(_ctx, "fail to append video data to chunk. "
				"chn=%ui "
				"data: time=%uL first_sq=%ui last_sq=%ui "
				"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
				chn,
				data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
				seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
		return SFUSHM_AV_ERR;
	}

	video_header = ngx_shm_av_get_video_chunk_header(chunk, sfushm_av_get_avctx(_ctx));

	if (data->begin) {
		video_header->size = size;
	} else {
		video_header->size += size;
	}

	if (data->end) {
		ngx_memzero(&v_meta, sizeof(v_meta));

		switch (chn_ctx->codec_id) {
		case SFUSHM_AV_VIDEO_CODEC_H264:
/*			ngx_shm_h264_quick_parse_annex_b_chunk(sfushm_av_get_shm(_ctx), chunk, &h264_desc);

			// IMPORTANT NOTE: WE ASSUME NO B-FRAMES!!!
			video_header->src_cts = 0;
			video_header->keyframe = h264_desc.keyframe;

			if (h264_desc.has_sps) {
				v_meta.codec_id = chn_ctx->codec_id;
				v_meta.width    = h264_desc.width;
				v_meta.height   = h264_desc.height;
				v_meta.pts      = adj_rtp_tm;
			} */
			break;
		case SFUSHM_AV_VIDEO_CODEC_VP8:
			rc = ngx_shm_codec_parse_vp8_frame_head(sfushm_av_get_shm(_ctx), data->data, data->len, &v_meta.width, &v_meta.height);

			video_header->src_cts = 0;

			// we have a key frame
			if (rc == SFUSHM_AV_OK && v_meta.width && v_meta.height) {
				video_header->keyframe = 1;
				v_meta.codec_id = chn_ctx->codec_id;
				v_meta.pts      = adj_rtp_tm;
			}
			break;
		default:
			SFUSHM_AV_LOG_ERR(_ctx, "unsupported codec id %d", chn_ctx->codec_id);
			video_header->src_cts = 0;
			video_header->keyframe = 0;
		}

		ngx_shm_av_append_video_chunk(sfushm_av_get_avctx(_ctx), adj_rtp_tm, chn, chunk, video_header->size);

		// this is a key frame and we have some meta-data to update
		if (v_meta.pts == adj_rtp_tm) {
			if (ngx_shm_av_is_new_video_meta(sfushm_av_get_avctx(_ctx), &v_meta, chn)) {
/*				if (ngx_shm_av_update_video_meta(sfushm_av_get_avctx(_ctx), &v_meta, chn) != NGX_OK) {
					SFUSHM_AV_LOG_ERR(_ctx, "fail to update video channel meta-data. "
							"chn=%ui "
							"data: time=%uL first_sq=%ui last_sq=%ui "
							"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL "
							"width=%ui height=%ui",
							chn,
							data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
							seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime,
							h264_desc.width, h264_desc.height);
				} */
			}
		}


		SFUSHM_AV_LOG_DEBUG(_ctx,
                "video data: "
                "chn=%uD codecid=%ui rtp_max_sq=%ui dts=%uL "
                "ix=%d sq=%uD sz=%uD kf=%uD "
                "old_ix=%d old_sq=%ui",
                chn, chn_ctx->codec_id, chn_ctx->max_rtp_seq, video_header->src_dts,
                ngx_shm_av_get_chn_cur_index(sfushm_av_get_avctx(_ctx), chn),
                chunk->seq_num,
                video_header->size,
                video_header->keyframe,
                ngx_shm_av_get_chn_oldest_index(sfushm_av_get_avctx(_ctx), chn),
                (ngx_shm_av_get_chn_oldest_chunk(sfushm_av_get_avctx(_ctx), chn))->seq_num);
	}

	chn_ctx->max_rtp_seq = data->last_rtp_seq;

	return SFUSHM_AV_OK;
}


int
sfushm_av_write_audio(sfushm_av_wr_ctx_t *wr_ctx, sfushm_av_frame_frag_t *data)
{
	_sfushm_av_ctx_t                *_ctx;
	_sfushm_av_chn_ctx_t            *chn_ctx;
	ngx_stream_shm_chnk             *chunk;
	ngx_shm_av_audio_chk_header_t   *audio_header;
	av_pts_t                        adj_rtp_tm;
	ngx_int_t                       rc;
	ngx_uint_t                      chn;
	size_t                          size;
	ngx_shm_av_audio_meta_t         a_meta;
	ngx_shm_seq                     seq;

    static ngx_buf_t                data_buf = { .last_buf = 1 };
    static ngx_chain_t              in_data = { .buf = &data_buf, .next = NULL };

	_ctx    = wr_ctx;
	chn_ctx = &_ctx->audio_ctx;
	chn     = ngx_shm_av_get_active_audio_chn_ix(sfushm_av_get_avctx(_ctx));

	if (data->first_rtp_seq < chn_ctx->max_rtp_seq) {
		SFUSHM_AV_LOG_ERR(_ctx, "invalid seq. "
				"chn=%ui "
				"data: time=%uL first_sq=%ui last_sq=%ui "
				"ctx: max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
				chn,
				data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
				chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
		return SFUSHM_AV_INVALID_SEQ;
	}

	adj_rtp_tm = data->rtp_time;

	// adjust for int overflow allowing max gap of 16 sec
	sfushm_av_adjust_for_overflow_32_64(chn_ctx->last_rtp_time, &adj_rtp_tm, chn_ctx->sample_rate * 16);

	// convert the RTP time to ms and offset based on the NTP timestamp that is received over RTCP
	adj_rtp_tm = sfushm_av_rtp_time_to_unix_timestamp(chn_ctx, adj_rtp_tm);

	data_buf.pos = data_buf.start = (u_char*)data->data;
	data_buf.last = data_buf.end = data_buf.start + data->len;

	chunk        = ngx_shm_av_get_chn_cur_chunk(sfushm_av_get_avctx(_ctx), chn);
	seq          = chunk->seq_num;

	// in case we have an open chunk
	if (seq == NGX_SHM_UNSET_UINT) {
		// if this fragment is the beginning of a new chunk it means we discard the previous one
		if (data->begin) {
			SFUSHM_AV_LOG_DEBUG(_ctx, "discarding audio chunk in the middle. "
					"chn=%ui "
					"data: time=%uL first_sq=%ui last_sq=%ui "
					"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
					chn,
					data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
					seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
			chunk = NULL; // force re-open this chunk i.e. free all the data that was copy to it
		}
	} else if (!data->begin) { // we don't have an open chunk but this is not the start of a chunk
		SFUSHM_AV_LOG_ERR(_ctx, "no open chunk to append audio data to. "
				"chn=%ui "
				"data: time=%uL first_sq=%ui last_sq=%ui "
				"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
				chn,
				data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
				seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
		return SFUSHM_AV_ERR;
	} else {
		chunk = NULL; // force opening new chunk
	}

	if (!chunk) {
	    // start a new chunk. This function advance the index
	    // BUT NOT the sequence number. The sequence number is
	    // advanced only when the chunk is ready
	    chunk = ngx_stream_shm_open_chunk(sfushm_av_get_shm(_ctx), chn);
	}

    // copy data to the chunk
    rc = ngx_shm_av_copy_to_chunk(sfushm_av_get_avctx(_ctx), chunk, chn, &in_data);

    if(rc < 0 || rc != data->len){
		SFUSHM_AV_LOG_ERR(_ctx, "fail to append audio data to chunk. "
				"chn=%ui "
				"data: time=%uL first_sq=%ui last_sq=%ui "
				"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
				chn,
				data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
				seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
		return SFUSHM_AV_ERR;
    }

	audio_header = ngx_shm_av_get_audio_chunk_header(chunk, sfushm_av_get_avctx(_ctx));

	if (data->begin) {
		audio_header->size = size;
	} else {
		audio_header->size += size;
	}

	if (data->end) {
		ngx_shm_av_append_audio_chunk(sfushm_av_get_avctx(_ctx), adj_rtp_tm, chn, chunk, audio_header->size);

		rc = ngx_shm_av_get_audio_meta(sfushm_av_get_avctx(_ctx), &a_meta, &seq);

		if (rc != SFUSHM_AV_OK) {
			SFUSHM_AV_LOG_ERR(_ctx, "fail to get audio meta data. "
					"chn=%ui "
					"data: time=%uL first_sq=%ui last_sq=%ui "
					"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
					chn,
					data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
					seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
		} else {
			if (!seq || seq != NGX_SHM_UNSET_UINT) {
				ngx_memzero(&a_meta, sizeof(a_meta));
				a_meta.codec_id = chn_ctx->codec_id;
				a_meta.sr = chn_ctx->sample_rate;
				a_meta.pts = adj_rtp_tm;
				a_meta.chn = chn_ctx->num_chn;

				rc = ngx_shm_av_update_channel_meta(sfushm_av_get_avctx(_ctx), chn, &a_meta, sizeof(a_meta));
				if (rc != SFUSHM_AV_OK) {
					SFUSHM_AV_LOG_ERR(_ctx, "fail to update audio meta data. "
							"chn=%ui "
							"data: time=%uL first_sq=%ui last_sq=%ui "
							"ctx: sq=%uL max_sq=%ui last_rtp_tm=%uL last_ntp_tm=%uL",
							chn,
							data->rtp_time, data->first_rtp_seq, data->last_rtp_seq,
							seq, chn_ctx->max_rtp_seq, chn_ctx->last_rtp_time, chn_ctx->last_ntp_unixtime);
				} else {
					SFUSHM_AV_LOG_TRACE(_ctx, "successfully updated audio meta.");
				}
			}
		}

		SFUSHM_AV_LOG_DEBUG(_ctx,
                "audio data: "
                "chn=%uD rtp_max_sq=%ui dts=%uL "
                "ix=%d sq=%uD sz=%uD "
                "old_ix=%d old_sq=%ui",
                chn, chn_ctx->max_rtp_seq, audio_header->src_pts,
                ngx_shm_av_get_chn_cur_index(sfushm_av_get_avctx(_ctx), chn),
                chunk->seq_num,
                audio_header->size,
                ngx_shm_av_get_chn_oldest_index(sfushm_av_get_avctx(_ctx), chn),
                (ngx_shm_av_get_chn_oldest_chunk(sfushm_av_get_avctx(_ctx), chn))->seq_num);
	}

	chn_ctx->max_rtp_seq = data->last_rtp_seq;

	return SFUSHM_AV_OK;
}

int sfushm_av_write_rtcp(sfushm_av_wr_ctx_t *wr_ctx, sfushm_av_frame_frag_t *data)
{
	// TBI
	return SFUSHM_AV_OK;
}

// write opaque data to shared memory. this allows an external controller to set stream meta-data such as room state
// in the shared memory.
int sfushm_av_write_stream_metadata(sfushm_av_wr_ctx_t *wr_ctx, uint8_t *data, size_t len)
{
	// TBI
	return SFUSHM_AV_OK;
}
