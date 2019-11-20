/*
 * ffngxshm_raw_media.c
 *
 *  Created on: May 16, 2018
 *      Author: Amir Pauker
 *
 * reader / writer for raw media data from / to shared memory.
 * it is different than the stream writer since the raw shm data is used internally by
 * the transcode process and is not read by the streaming server
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_log.h>
#include <ngx_stream_shm.h>
#include <ngx_shm_raw.h>

#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>

#include "ffngxshm.h"
#include "ffngxshm_raw_media.h"
#include "ffngxshm_log_header.h"
#include "ffngxshm_log.h"

// these two structs must be defined in ngx_shm_av.h so the shm_explorer will be able to
// read raw shm
typedef ngx_shm_raw_chn_header_t _ffngxshm_raw_wr_chn_header_t;
typedef ngx_shm_raw_stream_t _ffngxshm_raw_wr_stream_t;

/**
 * internal definition of the context (MUTS NOT BE exposed to user of this library)
 */
typedef struct {
	ngx_stream_shm             shm;              // nginx shm context
	ngx_pool_t                 *pool;            // memory pool to be used by this context only (destroyed when the ctx is closed)
	_ffngxshm_raw_wr_stream_t  *st;              // pointer to the area in the shm which stores the app context. used for frequent updates
} _ffngxshm_raw_wr_media_ctx_t;

/**
 * Per read channel the reader has to maintain a state so it can determine at any point of time what
 * should be the next frame to read from that channel
 */
typedef struct {
	ngx_uint_t                 last_ix;          // the index of the last returned frame. if set to NGX_SHM_UNSET_UINT it means
	                                             // we are out of sync and the next read should go to the end of the buffer (most recent)
	ngx_shm_seq                last_sq;          // the sequence number of the last returned frame
} _ffngxshm_raw_rd_media_chn_ctx_t;

typedef struct {
	ngx_stream_shm                     shm;              // nginx shm context
	ngx_pool_t                         *pool;            // memory pool to be used by this context only (destroyed when the ctx is closed)
	_ffngxshm_raw_wr_stream_t          *st;              // pointer to the area in the shm which stores the app context. used for frequent updates

	ngx_buf_t                          buf;              // buffer to be used for reading video and audio data from shm

	_ffngxshm_raw_rd_media_chn_ctx_t   chn_ctx[FFNGXSHM_MAX_NUM_CHANNELS]; // maintains the read state per channel
} _ffngxshm_raw_rd_media_ctx_t;


/******************************************************************************
 *
 *                           RAW CHUNK HEADERS
 *
 *****************************************************************************/
typedef struct {
    AVFrame              frame;
    uint32_t             plane_size[AV_NUM_DATA_POINTERS];   // in order to avoid re-calculate the planes sizes by readers we cache it in the chunk header
} _ffngxshm_raw_chunk_hd;


static int ffngxshm_write_raw_frame(ffngxshm_raw_wr_ctx_t *ctx, unsigned int chn, AVFrame *frame, int is_video, int unique);


#define ffngxshm_raw_media_get_shm(ctx) &(ctx)->shm
#define ffngxshm_raw_media_get_stream_name(ctx) ngx_shm_get_stream_name(ffngxshm_raw_media_get_shm(ctx))
#define ffngxshm_raw_media_null_ctx ((_ffngxshm_raw_rd_media_ctx_t*)0)

// the stream app context which is stored in the shared memory
#define ffngxshm_raw_media_get_stream(_ctx) \
    offset_to_shm_addr((ffngxshm_raw_media_get_shm(_ctx))->header->ctx,ffngxshm_raw_media_get_shm(_ctx),_ffngxshm_raw_wr_stream_t)

#define FFNGXSHM_RAW_LOG_INFO(ctx,str,...)                                             \
        ffngxshm_log(FFNGXSHM_LOG_LEVEL_INFO, "- ffshm - %s %d - %s - " str,     \
                __FUNCTION__,__LINE__,                                            \
                ((ctx && ffngxshm_raw_media_get_stream_name(ctx))? ffngxshm_raw_media_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define FFNGXSHM_RAW_LOG_ERR(ctx,str,...)                                             \
		ffngxshm_log(FFNGXSHM_LOG_LEVEL_ERR, "- ffshm - %s %d - %s - " str,     \
				__FUNCTION__,__LINE__,                                            \
				((ctx && ffngxshm_raw_media_get_stream_name(ctx))? ffngxshm_raw_media_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define FFNGXSHM_RAW_LOG_WARN(ctx,str,...)                                             \
		ffngxshm_log(FFNGXSHM_LOG_LEVEL_WARN, "- ffshm - %s %d - %s - " str,     \
				__FUNCTION__,__LINE__,                                            \
				((ctx && ffngxshm_raw_media_get_stream_name(ctx))? ffngxshm_raw_media_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define FFNGXSHM_RAW_LOG_DEBUG(ctx,str,...)                                            \
		ffngxshm_log(FFNGXSHM_LOG_LEVEL_DEBUG, "- ffshm - %s %d - %s - " str,    \
				__FUNCTION__,__LINE__,                                             \
				((ctx && ffngxshm_raw_media_get_stream_name(ctx))? ffngxshm_raw_media_get_stream_name(ctx):"na"),##__VA_ARGS__)

#define FFNGXSHM_RAW_LOG_TRACE(ctx,str,...)                                            \
		ffngxshm_log(FFNGXSHM_LOG_LEVEL_DEBUG, "- ffshm - %s %d - %s - " str,    \
				__FUNCTION__,__LINE__,                                             \
				((ctx && ffngxshm_raw_media_get_stream_name(ctx))? ffngxshm_raw_media_get_stream_name(ctx):"na"),##__VA_ARGS__)


/*
 * Copies the content of an input chain to a shared memory chunk
 */
ngx_int_t
ffngxshm_raw_wr_copy_to_chunk(
		_ffngxshm_raw_wr_media_ctx_t *_ctx,
        ngx_stream_shm_chnk *chunk,
        ngx_uint_t chn,
        uint8_t *buf,
		size_t in_len){

	ngx_stream_shm                 *shm;
    size_t                         blk_offset;
    size_t                         len;
    ngx_int_t                      total_size;
    ngx_stream_shm_block           *blk;

    if(!in_len)
        return FFNGXSHM_OK;

    shm = ffngxshm_raw_media_get_shm(_ctx);
    total_size = 0;

    if(chunk->last){
    	blk_offset = chunk->last;
        // converts the offset to actual pointer in memory
        blk = ngx_stream_shm_get_block(blk_offset, shm);

        // determines how much space we have in the block
        len = blk->end - blk->last;

        // in case the space we have is more than the current input buffer
        // decrease len
        if(in_len < len){
            len = in_len;
        }

        // copy
        ngx_memcpy(offset_to_shm_addr(blk->last,shm,void), buf, len);

        blk->last += len;
        buf += len;
        total_size += len;
        in_len -= len;
    }

	blk_offset = 0;
	blk = NULL;

	// if there is more to copy after we consume the space in the current block
	// we allocate more blocks for this chunk
    while(in_len){
        if(blk_offset == 0){
            // get an offset of a free block for channel chn
            blk_offset = ngx_stream_shm_get_shm_buffer (shm, chn);

            // we fail to get a free block for the specified channel
            if(blk_offset == 0){
            	FFNGXSHM_RAW_LOG_ERR(_ctx, "CRITICAL: fail to allocate buffer. chn=%uD", chn);

                return FFNGXSHM_ERR;
            }
            // converts the offset to actual pointer in memory
            blk = ngx_stream_shm_get_block(blk_offset, shm);
        }

        // determines how much space we have in the block
        len = blk->end - blk->last;

        // in case the space we have is more than the current input buffer
        // decrease len
        if(in_len < len){
            len = in_len;
        }

        // copy
        ngx_memcpy(offset_to_shm_addr(blk->last,shm,void), buf, len);

        blk->last += len;
        buf += len;
        total_size += len;
        in_len -= len;

        // if there is no more free space in the current block
        // add it to the chunk and request a new block
        if(blk->last >= blk->end){
            ngx_stream_shm_chunk_add_block(shm,chunk,blk_offset);
            blk_offset = 0;
        }
    }

    if(blk_offset != 0){
        ngx_stream_shm_chunk_add_block(shm,chunk,blk_offset);
    }

    return total_size;
}


/**
 * Takes as input an ffngxshm_shm_conf_t which is used as an abstraction to the shm real conf structure
 * and fill in the shm make it ready for calling ngx_stream_shm_cre
 */
static int
ffngxshm_raw_wr_create_shm_conf(ffngxshm_raw_init *init, ngx_stream_shm_conf_t *shm_conf)
{
	ngx_uint_t               chn;
	char                     *stream_name;
	ffngxshm_shm_conf_t      *conf;


	stream_name = init->stream_name;
	conf = &init->conf;

	for(chn = 0;chn < FFNGXSHM_MAX_NUM_CHANNELS;chn++)
	{
	    if(conf->channels[chn].video && conf->channels[chn].audio){
	    	FFNGXSHM_RAW_LOG_ERR(ffngxshm_raw_media_null_ctx, "interleave channel not allowed. chn=%d name=%s",
	    			chn, stream_name);

	    	return FFNGXSHM_ERR;
	    }

	    if(!conf->channels[chn].video && !conf->channels[chn].audio){
	        FFNGXSHM_RAW_LOG_TRACE(ffngxshm_raw_media_null_ctx, "skipping channel %d", chn);
	        continue;
	    }

	    // chunk header size
	    shm_conf->chncf[chn].shm_chk_header_size = sizeof(_ffngxshm_raw_chunk_hd);

	    // channel header size
	    shm_conf->chncf[chn].shm_chn_header_size = sizeof(_ffngxshm_raw_wr_chn_header_t);

	    // number of chunks (i.e. frames)
	    shm_conf->chncf[chn].shm_num_chks = conf->channels[chn].num_frames;

	    // number of data blocks allocated for this channel
	    // for raw video there should be no variation in picture size therefore we assume 1 block per chunk
	    shm_conf->chncf[chn].shm_num_blks = shm_conf->chncf[chn].shm_num_chks;

	    // size of each data block.
	    shm_conf->chncf[chn].shm_blk_size = ngx_stream_shm_block_size_memalign(conf->channels[chn].frame_size);

        FFNGXSHM_RAW_LOG_TRACE(ffngxshm_raw_media_null_ctx, "configure channel %d "
                "video=%d audio=%d num_frames=%d frame_size=%d "
                "chk_hd_sz=%uL chn_hd_sz=%uL num_chk=%uL num_blk=%uL blk_sz=%uL",
                chn, conf->channels[chn].video, conf->channels[chn].audio,
                conf->channels[chn].num_frames, conf->channels[chn].frame_size,
                shm_conf->chncf[chn].shm_chk_header_size, shm_conf->chncf[chn].shm_chn_header_size,
                shm_conf->chncf[chn].shm_num_chks, shm_conf->chncf[chn].shm_num_blks, shm_conf->chncf[chn].shm_blk_size);
	}

	return FFNGXSHM_OK;
}


/**
 * Creates a raw media writer context, open the specified shared memory for writing and associate the
 * shm context with the writer context. If the function succeeds it returns the allocated
 * context in ctx_out, otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using ffngxshm_close_raw_writer.
 */
int
ffngxshm_open_raw_writer(ffngxshm_raw_init *init, ffngxshm_raw_wr_ctx_t **ctx_out)
{
	int                            rc;
	ngx_stream_shm_conf_t          shm_conf;
    ngx_log_t                      *log;
    ngx_pool_t                     *pool;
    _ffngxshm_raw_wr_media_ctx_t   *_ctx;
    ngx_int_t                      errno_out, i;
    _ffngxshm_raw_wr_chn_header_t  *chn_hd;
    ngx_str_t                      stream_name;

    _ctx = NULL;
    pool = NULL;

    // takes the configuration provided by the caller and convert it to shared memory configuration
    // we do it in this way to avoid exposing ngx implementation to the caller
	memset(&shm_conf, 0, sizeof(shm_conf));
	rc = ffngxshm_raw_wr_create_shm_conf(init, &shm_conf);

	if(rc < 0){
		return rc;
	}

    log = ffngxshm_get_log();

    pool = ngx_create_pool(4096, log);

    if(!pool){
    	FFNGXSHM_RAW_LOG_ERR(ffngxshm_raw_media_null_ctx, "out of memory");
    	return FFNGXSHM_ERR;
    }

    // ffngxshm_raw_wr_ctx_t hides the implementation behind the real context
    // we do it in this way to avoid exposing ngx implementation to the caller
    *ctx_out = ngx_pcalloc(pool, sizeof(ffngxshm_raw_wr_ctx_t));
    if(!ctx_out){
    	FFNGXSHM_RAW_LOG_ERR(ffngxshm_raw_media_null_ctx, "out of memory");
    	goto fail;
    }

    // allocate the real context which coupled with nginx code
    (*ctx_out)->wr_ctx = _ctx = ngx_pcalloc(pool, sizeof(_ffngxshm_raw_wr_media_ctx_t));
    if(!_ctx){
    	FFNGXSHM_RAW_LOG_ERR(ffngxshm_raw_media_null_ctx, "out of memory");
    	goto fail;
    }

    _ctx->pool = pool;

    // set up the required memory block size for the stream meta-data
	shm_conf.app_ctx_size = sizeof(_ffngxshm_raw_wr_stream_t);

	errno_out = 0;

	stream_name.data = (u_char*)init->stream_name;
	stream_name.len = strlen(init->stream_name);

    // Creating a shared memory segment and acquiring a write lock
    rc = ngx_stream_shm_cre(
            log,
			ffngxshm_raw_media_get_shm(_ctx),
			stream_name,
            &shm_conf,
            ngx_shm_get_global_shm_registry(),
            raw, &errno_out);

    if(rc < 0){
        goto fail;
    }

    _ctx->st = ffngxshm_raw_media_get_stream(_ctx);
    _ctx->st->starttime = ((uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec);
    _ctx->st->highest_act_v_chn_ix = 0xFF;

    // setup the channels
    for(i = 0;i < STREAM_SHM_MAX_CHANNELS;i++){
        // in case this is an empty channel, skip it
        if(!shm_conf.chncf[i].shm_num_chks){
        	continue;
        }
        chn_hd = ngx_stream_shm_get_chn_header(i, ffngxshm_raw_media_get_shm(_ctx));

        chn_hd->width = init->conf.channels[i].width;
        chn_hd->height = init->conf.channels[i].height;
        chn_hd->video = init->conf.channels[i].video;
        chn_hd->audio = init->conf.channels[i].audio;

        chn_hd->last_srv_time = ((uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec);
        ngx_shm_kpi_ma_win_init(&chn_hd->bitrate, init->win_size);
        ngx_shm_kpi_ma_win_init(&chn_hd->interarrival_tm, init->win_size);

        FFNGXSHM_RAW_LOG_DEBUG(_ctx,
        		"successfully added channel. "
        		"chn=%ui vid=%ui aud=%ui width=%ui height=%ui",
				i, chn_hd->video, chn_hd->audio, chn_hd->width, chn_hd->height);

    }

    return FFNGXSHM_OK;

fail:
	if(_ctx && ngx_stream_shm_is_writer_ready(ffngxshm_raw_media_get_shm(_ctx))){
		ngx_stream_shm_close(ffngxshm_raw_media_get_shm(_ctx));
	}

	if(pool){
		ngx_destroy_pool(pool);
	}

    return FFNGXSHM_ERR;
}

int
ffngxshm_close_raw_writer(ffngxshm_raw_wr_ctx_t *ctx, int time_wait)
{
	_ffngxshm_raw_wr_media_ctx_t         *_ctx;

	_ctx = ctx->wr_ctx;

	if(_ctx && ngx_stream_shm_is_writer_ready(ffngxshm_raw_media_get_shm(_ctx))){
        if(time_wait){
            ngx_stream_shm_mark_as_time_wait(ffngxshm_raw_media_get_shm(_ctx));
        }
		ngx_stream_shm_close(ffngxshm_raw_media_get_shm(_ctx));
	}

	if(_ctx->pool){
		ngx_destroy_pool(_ctx->pool);
	}

	return FFNGXSHM_OK;
}

/**
 * Creates a raw media reader context, open the specified shared memory for reading and associate the
 * shm context with the reader context. If the function succeeds it returns the allocated
 * context in ctx_out, otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using ffngxshm_close_raw_reader.
 * The function get the stream name form the given init and fill the conf structure in the given init
 * with the configuration of the shared memory
 */
int
ffngxshm_open_raw_reader(ffngxshm_raw_init *init, ffngxshm_raw_rd_ctx_t **ctx_out)
{
	int                            rc;
    ngx_log_t                      *log;
    ngx_pool_t                     *pool;
    _ffngxshm_raw_rd_media_ctx_t   *_ctx;
    ngx_int_t                      errno_out;
    int                            i;
    _ffngxshm_raw_wr_chn_header_t  *chn_hd;
    ngx_str_t                      stream_name;

    _ctx = NULL;
    pool = NULL;

    // if we successfully manage to open the shm for reading we return the channel configuration
    // in this structure
	memset(&init->conf, 0, sizeof(init->conf));

    log = ffngxshm_get_log();

    pool = ngx_create_pool(4096, log);

    if(!pool){
    	FFNGXSHM_RAW_LOG_ERR(ffngxshm_raw_media_null_ctx, "out of memory");
    	return FFNGXSHM_ERR;
    }

    // ffngxshm_raw_rd_ctx_t hides the implementation behind the real context
    // we do it in this way to avoid exposing ngx implementation to the caller
    *ctx_out = ngx_pcalloc(pool, sizeof(ffngxshm_raw_rd_ctx_t));
    if(!ctx_out){
    	FFNGXSHM_RAW_LOG_ERR(ffngxshm_raw_media_null_ctx, "out of memory");
    	goto fail;
    }

    // allocate the real context which coupled with nginx code
    (*ctx_out)->rd_ctx = _ctx = ngx_pcalloc(pool, sizeof(_ffngxshm_raw_rd_media_ctx_t));
    if(!_ctx){
    	FFNGXSHM_RAW_LOG_ERR(ffngxshm_raw_media_null_ctx, "out of memory");
    	goto fail;
    }

    _ctx->pool = pool;

    // input buffer for reading data from shared memory
    // we are adding  sizeof(uint64_t) just in case the reader reads dwords instead of byte by byte
    _ctx->buf.start = ngx_pcalloc(pool, FFNGXSHM_DEFAULT_BUF_SZ + sizeof(uint64_t));
    if(!_ctx->buf.start){
    	FFNGXSHM_RAW_LOG_ERR(_ctx, "out of memory");
    	goto fail;
    }
    _ctx->buf.pos = _ctx->buf.last = _ctx->buf.start;
    _ctx->buf.end = _ctx->buf.start + FFNGXSHM_DEFAULT_BUF_SZ;

	errno_out = 0;

	stream_name.data = (u_char*)init->stream_name;
	stream_name.len = strlen(init->stream_name);

    // opening the shared memory segment for reading
    rc = ngx_stream_shm_reader(
            log,
			ffngxshm_raw_media_get_shm(_ctx),
			stream_name,
            ngx_shm_get_global_shm_registry(),
            raw);

    if(rc < 0){
        goto fail;
    }

    _ctx->st = ffngxshm_raw_media_get_stream(_ctx);

    init->conf.raw_data = 1;

    // copy channels configuration to the init object
    for(i = 0;i < STREAM_SHM_MAX_CHANNELS;i++){
    	// this channel was not set, skip it
    	if(ngx_stream_shm_get_chn_num_chk(i, ffngxshm_raw_media_get_shm(_ctx)) == 0){
    		continue;
    	}

        chn_hd = ngx_stream_shm_get_chn_header(i, ffngxshm_raw_media_get_shm(_ctx));

        init->conf.channels[i].width  = chn_hd->width;
        init->conf.channels[i].height = chn_hd->height;
        init->conf.channels[i].video  = chn_hd->video;
        init->conf.channels[i].audio  = chn_hd->audio;

        _ctx->chn_ctx[i].last_ix = NGX_SHM_UNSET_UINT; // marks out of sync


        FFNGXSHM_RAW_LOG_DEBUG(_ctx,
        		"successfully read channel. "
        		"chn=%ui vid=%ui aud=%ui width=%ui height=%ui",
				i, chn_hd->video, chn_hd->audio, chn_hd->width, chn_hd->height);

    }

    return FFNGXSHM_OK;

fail:
	if(_ctx && ngx_stream_shm_is_reader_ready(ffngxshm_raw_media_get_shm(_ctx))){
		ngx_stream_shm_close(ffngxshm_raw_media_get_shm(_ctx));
	}

	if(pool){
		ngx_destroy_pool(pool);
	}

    return FFNGXSHM_ERR;
}


int
ffngxshm_close_raw_reader(ffngxshm_raw_rd_ctx_t *ctx)
{
	_ffngxshm_raw_rd_media_ctx_t         *_ctx;

	_ctx = ctx->rd_ctx;

	if(_ctx && ngx_stream_shm_is_reader_ready(ffngxshm_raw_media_get_shm(_ctx))){
		ngx_stream_shm_close(ffngxshm_raw_media_get_shm(_ctx));
	}

	if(_ctx->pool){
		ngx_destroy_pool(_ctx->pool);
	}

	return FFNGXSHM_OK;
}

/**
 * Based on the pixel format of the given frame, determine if it is supported by transcode and
 * if it is returns the size in bytes of each plane.
 *
 * Return FFNGXSHM_OK if succeeded, otherwise FFNGXSHM_ERROR
 */
static inline int
ffngxshm_validate_pixel_format(AVFrame *frame, uint32_t *plane_sizes, int num_plane_sizes)
{
    const AVPixFmtDescriptor     *desc;
    int                          i, h;

    desc = av_pix_fmt_desc_get(frame->format);

    if(!desc){
        return FFNGXSHM_ERR;
    }

    if (!desc || (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) || (desc->flags & AV_PIX_FMT_FLAG_PAL) )
        return FFNGXSHM_ERR;

    memset(plane_sizes, 0, sizeof(*plane_sizes) * num_plane_sizes);

    for (i = 0; i < num_plane_sizes; i++) {
        if(!frame->data[i] || !frame->linesize[i]){
            return FFNGXSHM_OK;
        }

        h = frame->height;

        if (i == 1 || i == 2) {
            h = AV_CEIL_RSHIFT(frame->height, desc->log2_chroma_h);
        }

        plane_sizes[i] = h * frame->linesize[i];
    }

    return FFNGXSHM_OK;
}


/**
 * write the given raw frame (audio or video) to the specified channel in the given shm.
 * The function clones the given frame (it doesn't ref it)
 */
static inline int
ffngxshm_write_raw_frame(ffngxshm_raw_wr_ctx_t *ctx, unsigned int chn, AVFrame *frame, int is_video, int unique)
{
    int                             rc;
    ngx_stream_shm_chnk             *chunk;
    _ffngxshm_raw_chunk_hd          *chk_hd;
    AVFrame                         *avframe_chk_hd;
    _ffngxshm_raw_wr_media_ctx_t    *_ctx;
    int                             i;
    _ffngxshm_raw_wr_chn_header_t   *chn_hd;
    size_t                          ttl_frame_size;
    uint32_t                        *plane_sizes;

    _ctx = ctx->wr_ctx;

    // start a new chunk. This function advance the index
    // BUT NOT the sequence number. The sequence number is
    // advanced only when the chunk is ready
    chunk = ngx_stream_shm_open_chunk(ffngxshm_raw_media_get_shm(_ctx), chn);
    chk_hd = ngx_stream_shm_get_chunk_header(chunk, ffngxshm_raw_media_get_shm(_ctx));
    avframe_chk_hd = &chk_hd->frame;

    // in case of audio in planer mode if the number of channels is greater than AV_NUM_DATA_POINTERS
    // then the additional buffers are allocated under AVFrae::extended_data
    // since we don't want to deal with memory allocation for returned frames we limit the number of
    // audio channels to extended_data.
    // if that become a serious limitation we can move from planer mode to interleave mode
    if(!is_video){
        if(frame->channels > AV_NUM_DATA_POINTERS){
            FFNGXSHM_RAW_LOG_ERR(_ctx, "audio number of channels exceeds the limit. nb_chn=%d limit=%d",
                    frame->channels, AV_NUM_DATA_POINTERS);

            return FFNGXSHM_ERR;
        }
    }
    // in case of video we must know the pixel format in order to determine the frame size in bytes
    else{
        plane_sizes = chk_hd->plane_size;
        if(ffngxshm_validate_pixel_format(frame, plane_sizes, AV_NUM_DATA_POINTERS) < 0){
            FFNGXSHM_RAW_LOG_ERR(_ctx, "unsupported pixel format %d", frame->format);
            return FFNGXSHM_ERR;
        }
    }

#if NGX_DEBUG
    if(avframe_chk_hd->nb_side_data){
        FFNGXSHM_RAW_LOG_ERR(_ctx, "unexpected value for decoded frame nb_side_data. nb_side_data=%ui", avframe_chk_hd->nb_side_data);
    }
#endif


    // copy the header. we make a shallow copy of the AVFrame and nullify any pointer
    // since it will be invalid
    *avframe_chk_hd = *frame;
    memset(avframe_chk_hd->buf, 0, sizeof(avframe_chk_hd->buf));
    memset(avframe_chk_hd->data, 0, sizeof(avframe_chk_hd->data));
    avframe_chk_hd->extended_buf  = NULL;
    avframe_chk_hd->extended_data = NULL;
    avframe_chk_hd->hw_frames_ctx = NULL;
    avframe_chk_hd->metadata      = NULL;
    avframe_chk_hd->opaque        = NULL;
    avframe_chk_hd->opaque_ref    = NULL;
    avframe_chk_hd->private_ref   = NULL;
    avframe_chk_hd->side_data     = NULL;
    avframe_chk_hd->nb_side_data  = 0;

    ttl_frame_size = 0;

	// copy the actual data to the chunk body.  Note that we don't have to set any
	// size value in the chunk header since this information is stored in the chunk header
	// in the form of AVFrame
    if(is_video){
    	for(i = 0;i < AV_NUM_DATA_POINTERS;i++){
    		if(plane_sizes[i]){
    			ttl_frame_size += plane_sizes[i];

    			rc = ffngxshm_raw_wr_copy_to_chunk(_ctx, chunk, chn, frame->data[i], plane_sizes[i]);

    			if(rc < 0){
    				return FFNGXSHM_ERR;
    			}

    			if((size_t)rc != plane_sizes[i]){
    				FFNGXSHM_RAW_LOG_ERR(_ctx, "failed to write video data to chunk. "
    						"data_sz=%d rc=%d", plane_sizes[i], rc);
    				return FFNGXSHM_ERR;
    			}
    		}
    		else{
    			break;
    		}
    	}

    	// video channels in the shm are stored in descending order i.e. highest resolution at the
    	// lowest index. The decoder allocate static number of variants
    	// e.g. chn[0] - 1080p, chn[1] - 720p, chn[2] - 480p ...
    	// at runtime channels are populated based on the input source. For example if the source
    	// is 1080p, then all variants will be populated however if the source is 480p then only
    	// channel 2 and above will be populated. The encoder has to know at any point of time what is
    	// the highest resolution available. In order to make the selection more efficient, we store the
    	// index of the highest available resolution in the stream's context
      if(chn < _ctx->st->highest_act_v_chn_ix && frame->pts >= _ctx->st->highest_act_v_chn_pts){
    		_ctx->st->highest_act_v_chn_ix = chn;
    		_ctx->st->highest_act_v_chn_pts = frame->pts;
    	}

      FFNGXSHM_RAW_LOG_TRACE(_ctx, "writing video. sz=%d", ttl_frame_size);
    }
    // audio
    else{

    	// planar means that each audio channel is store in its own buffer. All channels are of equal byte size
    	if(av_sample_fmt_is_planar(frame->format)){
        	for(i = 0;i < frame->channels;i++){
        		if(frame->data[i]){

        		    ttl_frame_size += frame->linesize[0];

        			rc = ffngxshm_raw_wr_copy_to_chunk(_ctx, chunk, chn, frame->data[i], frame->linesize[0]);

        			if(rc < 0){
        				return FFNGXSHM_ERR;
        			}

        			if(rc != frame->linesize[0]){
        				FFNGXSHM_RAW_LOG_ERR(_ctx, "failed to write audio data to chunk. "
        						"frame->linesize[0]=%d rc=%d", frame->linesize[0], rc);
        				return FFNGXSHM_ERR;
        			}
        		}
        		else{
        			return FFNGXSHM_ERR;
        		}
        	}

        	FFNGXSHM_RAW_LOG_TRACE(_ctx, "writing planar audio. sz=%d", ttl_frame_size);
    	}
    	else{
    	    ttl_frame_size = frame->linesize[0];
			rc = ffngxshm_raw_wr_copy_to_chunk(_ctx, chunk, chn, frame->data[0], frame->linesize[0]);

			if(rc < 0){
				return FFNGXSHM_ERR;
			}

			if(rc != frame->linesize[0]){
				FFNGXSHM_RAW_LOG_ERR(_ctx, "failed to write data to chunk. "
						"frame->linesize[0]=%d rc=%d", frame->linesize[0], rc);
				return FFNGXSHM_ERR;
			}

			FFNGXSHM_RAW_LOG_TRACE(_ctx, "writing non-planar audio. sz=%d", ttl_frame_size);
    	}
    }

    if(frame->pts > _ctx->st->last_pts){
    	_ctx->st->last_pts = frame->pts;
    }

    _ctx->st->last_srv_time = ((uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec);

    // the decoder may inject frames in case the source underflow. The control process should be able to
    // determine if the decoded stream progress based on source input or just injected fake data. Therefore
    // we maintain two different timestamps in the stream header
    if(unique){
        _ctx->st->last_dec_srv_time = _ctx->st->last_srv_time;
    }


    chn_hd = ngx_stream_shm_get_chn_header(chn, ffngxshm_raw_media_get_shm(_ctx));
    ngx_shm_kpi_ma_win_add(&chn_hd->interarrival_tm, (_ctx->st->last_srv_time - chn_hd->last_srv_time));
    ngx_shm_kpi_ma_win_add(&chn_hd->bitrate, ttl_frame_size);
    chn_hd->last_srv_time = _ctx->st->last_srv_time;
    chn_hd->last_pts = frame->pts;
    chn_hd->total_bytes += ttl_frame_size;

    // in case the source stream contains the encoded FPS (e.g.time_tick in SPS/VUI)
    // the decoder may set the frame->sample_rate based on that value
    if(is_video && frame->sample_rate){
        chn_hd->enc_fps = frame->sample_rate;
    }


    // sets the chunk sequence number to let the readers know that it is
    // ready
    __ngx_shm_mem_barrier__
    ngx_stream_shm_set_chk_seq(ffngxshm_raw_media_get_shm(_ctx), chn, chunk);

    FFNGXSHM_RAW_LOG_TRACE(_ctx,
    		"successfully wrote to chunk. chn=%ui ix=%d sq=%uL pts=%uL is_video=%d",
			chn, ngx_stream_shm_get_chn_cur_index(ffngxshm_raw_media_get_shm(_ctx), chn),
			chunk->seq_num, frame->pts, is_video);

    return FFNGXSHM_OK;
}


int
ffngxshm_write_raw_dup_prev_video_frame(ffngxshm_raw_wr_ctx_t *ctx, unsigned int chn, uint64_t pts, int keyframe)
{
    _ffngxshm_raw_wr_media_ctx_t         *_ctx;
    ngx_stream_shm                       *shm;
    ngx_stream_shm_chnk                  *src_chunk;
    AVFrame                              *src_avframe_hd;
    ngx_shm_seq                          seq;
    ngx_stream_shm_chnk                  *chunk;
    _ffngxshm_raw_chunk_hd               *chk_hd, *src_chk_hd;
    AVFrame                              *avframe_chk_hd;
    _ffngxshm_raw_wr_chn_header_t        *chn_hd;
    int                                  n;


    _ctx = ctx->wr_ctx;
    shm = ffngxshm_raw_media_get_shm(_ctx);

    // this channel was not set
    if(ngx_stream_shm_get_chn_num_chk(chn, shm) == 0){
        FFNGXSHM_RAW_LOG_ERR(_ctx, "fail to read chunk. channel was not set. chn=%d", chn);
        return FFNGXSHM_CHAN_NOT_SET;
    }

    src_chunk = ngx_stream_shm_get_chunk(chn, ngx_stream_shm_get_chn_cur_index(shm, chn), shm);
    seq = src_chunk->seq_num;

    if(!seq || seq == NGX_SHM_UNSET_UINT){
        FFNGXSHM_RAW_LOG_ERR(_ctx, "failed to duplicate frame. previous chunk is not ready. "
                "chn=%d sq=%uL pts=%uL", chn, seq, pts);
        return FFNGXSHM_ERR;
    }

    // start a new chunk. This function advance the index
    // BUT NOT the sequence number. The sequence number is
    // advanced only when the chunk is ready
    chunk = ngx_stream_shm_open_chunk(shm, chn);

    // clone the data
    if((n = ngx_stream_shm_clone_chunk(shm, chn, src_chunk, chunk)) < 0){
        FFNGXSHM_RAW_LOG_ERR(_ctx, "failed to duplicate frame. "
                "chn=%d sq=%uL pts=%uL", chn, seq, pts);
        return FFNGXSHM_ERR;
    }

    // get the destination header
    chk_hd = ngx_stream_shm_get_chunk_header(chunk, shm);
    avframe_chk_hd = &chk_hd->frame;

    // get the header of the source chunk
    src_chk_hd = ngx_stream_shm_get_chunk_header(src_chunk, shm);
    src_avframe_hd = &src_chk_hd->frame;

    // make a shallow copy of the header
    *avframe_chk_hd = *src_avframe_hd;

    // sets the pts
    avframe_chk_hd->pts = avframe_chk_hd->pkt_dts = pts;

    // set the keyframe status
    if (keyframe) {
        avframe_chk_hd->key_frame = keyframe;
        avframe_chk_hd->pict_type = AV_PICTURE_TYPE_I;
    } else {
        avframe_chk_hd->key_frame = 0;
        avframe_chk_hd->pict_type = AV_PICTURE_TYPE_NONE;
    }

    // video channels in the shm are stored in descending order i.e. highest resolution at the
    // lowest index. The decoder allocate static number of variants
    // e.g. chn[0] - 1080p, chn[1] - 720p, chn[2] - 480p ...
    // at runtime channels are populated based on the input source. For example if the source
    // is 1080p, then all variants will be populated however if the source is 480p then only
    // channel 2 and above will be populated. The encoder has to know at any point of time what is
    // the highest resolution available. In order to make the selection more efficient, we store the
    // index of the highest available resolution in the stream's context.
    if(chn < _ctx->st->highest_act_v_chn_ix && pts >= _ctx->st->highest_act_v_chn_pts){
		  _ctx->st->highest_act_v_chn_ix = chn;
      _ctx->st->highest_act_v_chn_pts = pts;
    }

    if(pts > _ctx->st->last_pts){
        _ctx->st->last_pts = pts;
    }

    _ctx->st->last_srv_time = ((uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec);

    chn_hd = ngx_stream_shm_get_chn_header(chn, ffngxshm_raw_media_get_shm(_ctx));
    ngx_shm_kpi_ma_win_add(&chn_hd->interarrival_tm, (_ctx->st->last_srv_time - chn_hd->last_srv_time));
    ngx_shm_kpi_ma_win_add(&chn_hd->bitrate, n);
    chn_hd->last_srv_time = _ctx->st->last_srv_time;
    chn_hd->last_pts = pts;
    chn_hd->total_bytes += n;

    // sets the chunk sequence number to let the readers know that it is
    // ready
    __ngx_shm_mem_barrier__
    ngx_stream_shm_set_chk_seq(ffngxshm_raw_media_get_shm(_ctx), chn, chunk);

    FFNGXSHM_RAW_LOG_TRACE(_ctx,
            "successfully duplicated chunk. ix=%d sq=%uL pts=%uL",
            ngx_stream_shm_get_chn_cur_index(shm, chn), chunk->seq_num, pts);

    return FFNGXSHM_OK;
}

/**
 * Attempt to write the given raw video picture to the associated shm in the specified channel.
 * The function clones the given frame (it doesn't ref it)
 */
int
ffngxshm_write_raw_video(ffngxshm_raw_wr_ctx_t *ctx, unsigned int chn, AVFrame *frame, int unique)
{
	return ffngxshm_write_raw_frame(ctx, chn, frame, 1, unique);
}

/**
 * Attempt to write the given raw audio frame to the associated shm in the specified channel.
 * The function clones the given frame (it doesn't ref it)
 */
int
ffngxshm_write_raw_audio(ffngxshm_raw_wr_ctx_t *ctx, unsigned int chn, AVFrame *frame, int unique)
{
	return ffngxshm_write_raw_frame(ctx, chn, frame, 0, unique);
}


int
ffngxshm_get_raw_channel_stats(ffngxshm_raw_rd_ctx_t *ctx, unsigned int chn, ffngxshm_raw_chn_stats *out)
{
    _ffngxshm_raw_wr_chn_header_t        *chn_hd;
    _ffngxshm_raw_rd_media_ctx_t         *_ctx;
    ngx_stream_shm                       *shm;
    ngx_shm_kpi_ma_stats                 ma;
	ngx_int_t                            chn_idx;
	uint64_t                             max_seq;                // the current sequence number being written - max value across all video channels
	uint64_t                             last_srv_time;          // timestamp of when the last raw video frame being written - max value across all video channels

    _ctx = ctx->rd_ctx;
    shm = ffngxshm_raw_media_get_shm(_ctx);

    // this channel was not set
    if(ngx_stream_shm_get_chn_num_chk(chn, shm) == 0){
        FFNGXSHM_RAW_LOG_ERR(_ctx, "fail to read channel stats. channel was not set. chn=%d", chn);
        return FFNGXSHM_CHAN_NOT_SET;
    }

    chn_hd = ngx_stream_shm_get_chn_header(chn, shm);

    if(ngx_shm_kpi_ma_get_stats_no_stdev(&chn_hd->interarrival_tm, &ma) >= 0){
        out->mv_avg_interarrival_tm = ma.avg;
    }
    else{
        out->mv_avg_interarrival_tm = 0;
    }

    out->last_pts = chn_hd->last_pts;
    out->num_pending = ngx_stream_shm_get_chn_cur_seq(shm, chn) - _ctx->chn_ctx[chn].last_sq;

    // we assume there is only one audio channel therefore we can calculate fps based on
    // this channel alone
    if(chn_hd->audio){
        out->avg_fps = ngx_stream_shm_get_chn_cur_seq(shm,chn) * 1000 / (chn_hd->last_srv_time - _ctx->st->starttime + 1);
        out->enc_fps = 0; // for audio we don't have encoder FPS
    }
    // for video we have to loop over all video channels and calculate fps based on
    // the most recent active channel
    else{
        out->avg_fps = 0;
        out->enc_fps = 0;
        max_seq = 0;
        last_srv_time = _ctx->st->starttime;

        for (chn_idx = 1;chn_idx < FFNGXSHM_MAX_NUM_CHANNELS;chn_idx++) {
            // this channel was not set
            if (ngx_stream_shm_get_chn_num_chk(chn_idx, shm) == 0)
                continue;

            chn_hd = ngx_stream_shm_get_chn_header(chn_idx, shm);

            if(!chn_hd->video)
                continue;

            if (chn_hd->last_srv_time > last_srv_time)
                last_srv_time = chn_hd->last_srv_time;

            if (ngx_stream_shm_get_chn_cur_seq(shm,chn_idx) > max_seq)
                max_seq = ngx_stream_shm_get_chn_cur_seq(shm,chn_idx);

            if(chn_hd->enc_fps > out->enc_fps){
                out->enc_fps = chn_hd->enc_fps;
            }
        }

        out->avg_fps = max_seq * 1000 / (last_srv_time - _ctx->st->starttime + 1);
    }


    return FFNGXSHM_OK;
}


/**
 * fill up the given array with information about the channels in the shm
 */
int
ffngxshm_get_raw_channel_layout(
        ffngxshm_raw_rd_ctx_t *ctx, ffngxshm_raw_chn_hd_info *chn_hd_out)
{
    _ffngxshm_raw_wr_chn_header_t        *chn_hd;
    _ffngxshm_raw_rd_media_ctx_t         *_ctx;
    ngx_stream_shm                       *shm;
    ngx_int_t                            chn;

    _ctx = ctx->rd_ctx;
    shm = ffngxshm_raw_media_get_shm(_ctx);

    ngx_memzero(chn_hd_out, sizeof(ffngxshm_raw_chn_hd_info) * STREAM_SHM_MAX_CHANNELS);

    if(!ngx_stream_shm_is_reader_ready(shm)){
        FFNGXSHM_RAW_LOG_ERR(_ctx, "reader not ready\n");
        return FFNGXSHM_ERR;
    }

    for(chn = 0;chn < STREAM_SHM_MAX_CHANNELS;chn++){
        // this channel was not set
        if(ngx_stream_shm_get_chn_num_chk(chn, shm) == 0){
            continue;
        }

        chn_hd = ngx_stream_shm_get_chn_header(chn, shm);
        chn_hd_out[chn].video = chn_hd->video;
        chn_hd_out[chn].audio = chn_hd->audio;
        chn_hd_out[chn].width = chn_hd->width;
        chn_hd_out[chn].height = chn_hd->height;

    }

    return FFNGXSHM_OK;
}


void _ffngxshm_raw_channels_snapshot(ngx_stream_shm *shm, u_char *str, int sz) {
	_ffngxshm_raw_wr_chn_header_t        *chn_hd;
	ngx_shm_kpi_ma_stats                 ma;
	ngx_int_t                            chn;
	u_char                               *p, *q;

	p = str;
	q = p + sz - 1;
	ngx_memzero(str, sz);

	p = ngx_slprintf(p, q, "channels snapshot:");

	for(chn = 0;chn < FFNGXSHM_MAX_NUM_CHANNELS;chn++){
			// this channel was not set
			if(ngx_stream_shm_get_chn_num_chk(chn, shm) == 0){
					continue;
			}

			chn_hd = ngx_stream_shm_get_chn_header(chn, shm);
			p = ngx_slprintf(p, q, " chn: %d - %uL - %uL", chn, chn_hd->last_pts, chn_hd->last_srv_time);
	}
}

void
ffngxshm_raw_channels_snapshot(ffngxshm_raw_wr_ctx_t *ctx)
{
	_ffngxshm_raw_wr_media_ctx_t *_ctx;
	u_char                       str[1024];
	ngx_stream_shm               *shm;

	_ctx = ctx->wr_ctx;
	shm = ffngxshm_raw_media_get_shm(_ctx);
	_ffngxshm_raw_channels_snapshot(shm, str, sizeof(str));
	FFNGXSHM_RAW_LOG_DEBUG(_ctx, "%s", str);
}

void
ffngxshm_raw_channels_snapshot2(ffngxshm_raw_rd_ctx_t *ctx)
{
	_ffngxshm_raw_rd_media_ctx_t *_ctx;
	u_char                       str[1024];
	ngx_stream_shm               *shm;

	_ctx = ctx->rd_ctx;
	shm = ffngxshm_raw_media_get_shm(_ctx);
	_ffngxshm_raw_channels_snapshot(shm, str, sizeof(str));
	FFNGXSHM_RAW_LOG_DEBUG(_ctx, "%s", str);
}

int ffngxshm_get_highest_video_active_chn(ffngxshm_raw_rd_ctx_t *ctx)
{
    _ffngxshm_raw_rd_media_ctx_t         *_ctx;

    _ctx = ctx->rd_ctx;

    return (_ctx->st->highest_act_v_chn_ix < 0xFF)? _ctx->st->highest_act_v_chn_ix : FFNGXSHM_AGAIN;
}


int
ffngxshm_read_next_raw_frame(ffngxshm_raw_rd_ctx_t *ctx, unsigned int chn, int *num_pending, uint64_t *next_dts)
{
	_ffngxshm_raw_rd_media_ctx_t         *_ctx;
	ngx_stream_shm_chnk                  *chunk, *nxt_chk;
	_ffngxshm_raw_chunk_hd               *chk_hd, *nxt_frm_hd;
	AVFrame                              *avframe_chk_hd, *avframe_nxt_frm_hd;
	uint32_t                             plane_size[AV_NUM_DATA_POINTERS];
	ngx_int_t                            ix, i, rc, start_ix;
	ngx_shm_seq                          seq, nxt_seq;
	ngx_stream_shm                       *shm;
	_ffngxshm_raw_wr_chn_header_t        *chn_hd;
	size_t                               chunk_sz;
	u_char                               *p;
	ngx_stream_shm_chk_rd_ctx            rd_ctx;

	_ctx = ctx->rd_ctx;
	shm = ffngxshm_raw_media_get_shm(_ctx);

	// this shm is closed and we are waiting for the broadcast to reconnect
	// and start a new one
    if(ngx_stream_shm_is_time_wait(shm)){
        return FFNGXSHM_TIME_WAIT;
    }


    if(ngx_stream_shm_is_closing(shm)){
        return FFNGXSHM_CLOSING;
    }

	// this channel was not set
	if(ngx_stream_shm_get_chn_num_chk(chn, shm) == 0){
		FFNGXSHM_RAW_LOG_ERR(_ctx, "fail to read chunk. channel was not set. chn=%d", chn);
		return FFNGXSHM_CHAN_NOT_SET;
	}

	// we already delivered the most recent frame or the stream hasn't started yet
	if(ngx_stream_shm_get_chn_cur_seq(shm, chn) <= _ctx->chn_ctx[chn].last_sq){
		FFNGXSHM_RAW_LOG_DEBUG(_ctx, "content not ready. chn=%d last_sq=%uL last_ix=%ui",
				chn, _ctx->chn_ctx[chn].last_sq, _ctx->chn_ctx[chn].last_ix);
		return FFNGXSHM_EOF;
	}

	// the read context for this channel is out of sync, move to the most recent chunk
	if(_ctx->chn_ctx[chn].last_ix == NGX_SHM_UNSET_UINT){
		start_ix = ix = ngx_stream_shm_get_chn_cur_index(shm, chn);
		chunk = ngx_stream_shm_get_chunk(chn, ix, shm);
		seq = chunk->seq_num;

		for(;;){
			// we found a sync point
			if(seq && seq != NGX_SHM_UNSET_UINT && seq > _ctx->chn_ctx[chn].last_sq){
				break;
			}

			ix = ngx_stream_shm_adjust_chn_index(shm, ix - 1, chn);

			if(ix == start_ix){
				FFNGXSHM_RAW_LOG_DEBUG(_ctx, "content not ready. chn=%d last_sq=%uL last_ix=%ui ix=%d",
						chn, _ctx->chn_ctx[chn].last_sq, _ctx->chn_ctx[chn].last_ix, ix);
				return FFNGXSHM_EOF;
			}

			chunk = ngx_stream_shm_get_chunk(chn, ix, shm);
			seq = chunk->seq_num;
		}
	}
	else{
		// examine the next chunk
		ix = ngx_stream_shm_adjust_chn_index(shm, _ctx->chn_ctx[chn].last_ix + 1, chn);
		chunk = ngx_stream_shm_get_chunk(chn, ix, shm);
		seq = chunk->seq_num;

		if(!seq || seq == NGX_SHM_UNSET_UINT){
			FFNGXSHM_RAW_LOG_DEBUG(_ctx, "content not ready. chn=%d last_sq=%uL last_ix=%ui ix=%d sq=%uL",
					chn, _ctx->chn_ctx[chn].last_sq, _ctx->chn_ctx[chn].last_ix, ix, seq);
			return FFNGXSHM_EOF;
		}

	    // the reader is falling behind
	    if(_ctx->chn_ctx[chn].last_sq + 1 < seq){
	    	FFNGXSHM_RAW_LOG_ERR(_ctx,
	    			"reader is falling behind. "
	    			"chn=%ui ix=%d sq=%uL last_sq=%uL",
					chn, ix, seq, _ctx->chn_ctx[chn].last_sq);

			// mark that we are out of sync
	    	_ctx->chn_ctx[chn].last_ix = NGX_SHM_UNSET_UINT;

	    	return FFNGXSHM_AGAIN;
	    }
	}

	chn_hd = ngx_stream_shm_get_chn_header(chn, shm);

	chk_hd = ngx_stream_shm_get_chunk_header(chunk, shm);
	avframe_chk_hd = &chk_hd->frame;
	// we must make a local copy to make sure the writer doesn't override it during the copy process
	ngx_memcpy(plane_size, chk_hd->plane_size, sizeof(chk_hd->plane_size));

	// make a shallow copy of the AVFrame
	ctx->frame_out = *avframe_chk_hd;

	// determine the chunk size
	if(chn_hd->video){
		chunk_sz = 0;
    	for(i = 0;i < AV_NUM_DATA_POINTERS;i++){
    		chunk_sz += plane_size[i];
    	}
	}
	else{
		chunk_sz = av_sample_fmt_is_planar(ctx->frame_out.format)?
				ctx->frame_out.channels * ctx->frame_out.linesize[0] : ctx->frame_out.linesize[0];
	}

	ngx_stream_shm_init_rd_ctx(shm, chunk, &rd_ctx);

    // make sure the complier will not try to optimize the sequence read from chunk
    // the sequence number is served as a lazy lock
    __ngx_shm_mem_barrier__

    // we must make sure the chunk size that we calculated is based on valid data from shared memory
    // i.e. that the writer didnt override it as we read from chk_hd->plane_size
    if(!ngx_stream_shm_cmp_seq(seq, chunk->seq_num)){
        FFNGXSHM_RAW_LOG_ERR(_ctx,
                "reader is behind. chn=%d sq=%uL ix=%d", chn, seq, ix);
        // mark that we are out of sync and return
        goto out_of_sync;
    }


	// in case the chunk size exceed the currently allocated buffer size we have to re-allocate
	if(_ctx->buf.start + chunk_sz > _ctx->buf.end){
		if(chunk_sz > FFNGXSHM_DEFAULT_BUF_MAX_SZ){
			FFNGXSHM_RAW_LOG_ERR(_ctx, "CRITICAL: chunk size exceeds max buf size. chn=%d sq=%uL ix=%d sz=%uL",
					chn, seq, ix, chunk_sz);
			goto out_of_sync;
		}
		else{
			FFNGXSHM_RAW_LOG_INFO(_ctx,
					"reallocating input buffer. sz=%uL", chunk_sz);

			p = ngx_palloc(_ctx->pool, chunk_sz);
			if(!p){
				FFNGXSHM_RAW_LOG_ERR(_ctx, "out of memory");
				goto out_of_sync;
			}

			ngx_pfree(_ctx->pool, _ctx->buf.start);
			_ctx->buf.start = p;
			_ctx->buf.end = _ctx->buf.start + chunk_sz;
		}
	}

	_ctx->buf.pos = _ctx->buf.last = _ctx->buf.start;



	// for video each plane is stored in its own buffer and may be of different size
	// normally plane zero is the luma and it is 4 times bigger than the other chroma
	// planes
	if(chn_hd->video){
		for(i = 0;i < AV_NUM_DATA_POINTERS;i++){
			if(plane_size[i]){
				ctx->frame_out.data[i] = _ctx->buf.last;
				rc = ngx_stream_shm_blk_cpy_bytes(shm, _ctx->buf.last, &rd_ctx, plane_size[i]);
				if(rc < 0){
					FFNGXSHM_RAW_LOG_ERR(_ctx,
							"failed to read video plane to output buffer. not enough data. chn=%d sq=%uL ix=%d chk_sz=%uL pln_sz=%uL",
							chn, seq, ix, chunk_sz, plane_size[i]);
					goto out_of_sync;
				}
				_ctx->buf.last += plane_size[i];
			}
		}
		ctx->frame_out.extended_data = ctx->frame_out.data;
	}
	// audio
	else{
		// in planar format each audio channel is stored in a separate buffer of equal size
		if(av_sample_fmt_is_planar(ctx->frame_out.format)){
			for(i = 0;i < ctx->frame_out.channels;i++){
				ctx->frame_out.data[i] = _ctx->buf.last;
				rc = ngx_stream_shm_blk_cpy_bytes(shm, _ctx->buf.last, &rd_ctx, ctx->frame_out.linesize[0]);
				if(rc < 0){
					FFNGXSHM_RAW_LOG_ERR(_ctx,
							"failed to read audio plane to output buffer. not enough data. chn=%d sq=%uL ix=%d chk_sz=%uL pln_sz=%uL",
							chn, seq, ix, chunk_sz, ctx->frame_out.linesize[0]);
					goto out_of_sync;
				}
				_ctx->buf.last += ctx->frame_out.linesize[0];
			}
			ctx->frame_out.extended_data = ctx->frame_out.data;
		}
		// in interleave mode all audio samples from all audio channels are stored in one buffer
		else{
			ctx->frame_out.data[0] = _ctx->buf.last;
			rc = ngx_stream_shm_blk_cpy_bytes(shm, _ctx->buf.last, &rd_ctx, ctx->frame_out.linesize[0]);
			if(rc < 0){
				FFNGXSHM_RAW_LOG_ERR(_ctx,
						"failed to read audio to output buffer. not enough data. chn=%d sq=%uL ix=%d chk_sz=%uL sz=%uL",
						chn, seq, ix, chunk_sz, ctx->frame_out.linesize[0]);
				goto out_of_sync;
			}
			_ctx->buf.last += ctx->frame_out.linesize[0];
			ctx->frame_out.extended_data = ctx->frame_out.data;
		}
	}


    // retrieving the number of pending frames in the buffer as well as the DTS of the next frame
    // if there is one.
    // NOTE: it should be done before we re-check the lazy lock
    nxt_seq = ngx_stream_shm_get_chn_cur_seq(shm, chn);
    if(nxt_seq != NGX_SHM_UNSET_UINT && nxt_seq > seq){
    	*num_pending = nxt_seq - seq;

    	nxt_chk = ngx_stream_shm_get_chunk(chn, ngx_stream_shm_adjust_chn_index(shm, ix + 1, chn), shm);
    	nxt_frm_hd = ngx_stream_shm_get_chunk_header(chunk, shm);
    	avframe_nxt_frm_hd = &nxt_frm_hd->frame;

    	*next_dts = avframe_nxt_frm_hd->pts;
    }
    else{
    	*num_pending = 0;
    	*next_dts = 0;
    }


    // in case the writer overridden the chunk as we read we have
    // to re-sync
    __ngx_shm_mem_barrier__
    if(!ngx_stream_shm_cmp_seq(seq, chunk->seq_num)){
    	FFNGXSHM_RAW_LOG_ERR(_ctx,
                "reader is behind. chn=%d sq=%uL ix=%d", chn, seq, ix);
		// mark that we are out of sync and return
		goto out_of_sync;
    }

    _ctx->chn_ctx[chn].last_ix  = ix;
    _ctx->chn_ctx[chn].last_sq  = seq;

    FFNGXSHM_RAW_LOG_TRACE(_ctx, "successfully read frame. chn=%d ix=%ui sq=%uL pts=%uL nxt_dts=%uL is_video=%d",
    		chn, ix, seq, ctx->frame_out.pts, *next_dts, chn_hd->video);

    return FFNGXSHM_OK;

out_of_sync:
	_ctx->chn_ctx[chn].last_ix = NGX_SHM_UNSET_UINT; // marks that we are out of sync
	return FFNGXSHM_AGAIN;
}

/**
 * Access params is an opque field that is set by the application in the shared memory and
 * helps to control the access to the stream i.e. determine if the access is public or require
 * authorization. The transcoder copy the value of the access parameters as is from the source all
 * the way out to the encoded stream. This makes it easier to enforce stream acceess accross all
 * variants and replication by simply copy the value from the source all the way to the border
 */
int
ffngxshm_get_raw_acess_params(ffngxshm_raw_rd_ctx_t *ctx, ffngxshm_access_param *out)
{
    _ffngxshm_raw_rd_media_ctx_t         *_ctx;
    _ctx = ctx->rd_ctx;
    *out = (ffngxshm_access_param) ngx_stream_shm_get_acc_param(ffngxshm_raw_media_get_shm(_ctx));
    return FFNGXSHM_OK;
}

int
ffngxshm_set_raw_acess_params(ffngxshm_raw_wr_ctx_t *ctx, ffngxshm_access_param access_param)
{
    _ffngxshm_raw_wr_media_ctx_t *_ctx;
    _ctx = ctx->wr_ctx;
    ngx_stream_shm_set_acc_param(ffngxshm_raw_media_get_shm(_ctx), access_param);
    return FFNGXSHM_OK;
}
