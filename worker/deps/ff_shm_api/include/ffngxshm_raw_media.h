/*
 * ffngxshm_raw_media.h
 *
 *  Created on: May 16, 2018
 *      Author: Amir Pauker
 */

#include "ffngxshm.h"
#include <libavutil/frame.h>

#ifndef INCLUDE_FFNGXSHM_RAW_MEDIA_H_
#define INCLUDE_FFNGXSHM_RAW_MEDIA_H_

// we hide the implementation details behind this context from clients of this library
typedef struct{
	void            *wr_ctx;                    // the internal context which is used for writing data to shm
} ffngxshm_raw_wr_ctx_t;


// we hide the implementation details behind this context from clients of this library
typedef struct{
	void            *rd_ctx;                    // the internal context which is used for reading data from shm
	AVFrame         frame_out;                  // whenever the reader calls the read next frame, the content of the next frame is
	                                            // stored here. The previous content will be discard. The reader MUST NOT ref this frame!!
} ffngxshm_raw_rd_ctx_t;

typedef struct {
	char                 *stream_name;         // the name of the stream to open for writing
    ffngxshm_shm_conf_t  conf;                 // shared memory configuration
	uint16_t             win_size;             // stats moving average window size
} ffngxshm_raw_init;


// struct that is used for returning av channel stats
typedef struct{
    double        mv_avg_interarrival_tm;       // moving average of frames inter-arrival time in milliseconds. Avg FPS = 1000 / Avg inter-arrival time
    uint64_t      last_pts;                     // the PTS of the most recent frame
    uint64_t      num_pending;                  // number of frames available for reading
	uint64_t      avg_fps;                      // estimated fps based on stream start ts, last write time and max seq number of raw video frames being written, across all video channels
	uint64_t      enc_fps;                      // in case the encoder sets the FPS in the encoding parameters e.g. SPS VUI

} ffngxshm_raw_chn_stats;

// struct that is used for returning channels static info
typedef struct{
    uint16_t             width;               // in case this channel contains video, the width of the picture in pixels
    uint16_t             height;              // in case this channel contains video, the height of the picture in pixels
    unsigned             video:1;             // this channel contains video
    unsigned             audio:1;             // this channel contains audio
} ffngxshm_raw_chn_hd_info;



/**
 * Creates a raw media writer context, open the specified shared memory for writing and associate the
 * shm context with the writer context. If the function succeeds it returns the allocated
 * context in ctx_out, otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using ffngxshm_close_raw_writer.
 */
int ffngxshm_open_raw_writer(ffngxshm_raw_init *init, ffngxshm_raw_wr_ctx_t **ctx_out);
int ffngxshm_close_raw_writer(ffngxshm_raw_wr_ctx_t *ctx, int time_wait);


/**
 * Creates a raw media reader context, open the specified shared memory for reading and associate the
 * shm context with the reader context. If the function succeeds it returns the allocated
 * context in ctx_out, otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using ffngxshm_close_raw_reader.
 * The function get the stream name form the given init and fill the conf structure in the given init
 * with the configuration of the shared memory
 */
int ffngxshm_open_raw_reader(ffngxshm_raw_init *init, ffngxshm_raw_rd_ctx_t **ctx_out);
int ffngxshm_close_raw_reader(ffngxshm_raw_rd_ctx_t *ctx);


/**
 * write the given raw video picture to the associated shm in the specified channel.
 * The function clones the given frame (it doesn't ref it)
 *
 * unique - true only if the frame is not injected/duplicated by the decoder
 */
int ffngxshm_write_raw_video(ffngxshm_raw_wr_ctx_t *ctx, unsigned int chn, AVFrame *frame, int unique);

/**
 * duplicate the most recent video frame and set the pts of the new frame to the specified pts
 * this is used in case of underflow in the source stream
 */
int ffngxshm_write_raw_dup_prev_video_frame(ffngxshm_raw_wr_ctx_t *ctx, unsigned int chn, uint64_t pts, int keyframe);


/**
 * write the given raw audio frame to the associated shm in the specified channel.
 * The function clones the given frame (it doesn't ref it)
 *
 * unique - true only if the frame is not injected/duplicated by the decoder
 */
int ffngxshm_write_raw_audio(ffngxshm_raw_wr_ctx_t *ctx, unsigned int chn, AVFrame *frame, int unique);


/**
 * Reads the next frame from the specified channel into the AVFrame that is in the read context
 * The function discard the previously stored frame in the context.
 * In case the specified channel is inactive, the function returns an error
 * In case the channel doesn't contain any new data the function returns again
 * In case the next frame was overridden the function returns the most recent frame
 *
 * num_pending - output the number of frames pending in the buffer after reading this frame
 * nex_dts     - in case num_pending greater than zero, output the DTS of the next frame (return AVFrame::pts)
 *
 * Return:
 *   FFNGXSHM_OK    - read successful
 *   FFNGXSHM_ERR   - fatal error (e.g. out of memory). abort
 *   FFNGXSHM_AGAIN - the reader is falling behind and should attempt again without delay
 *   FFNGXSHM_EOF   - content is not ready
 *
 */
int ffngxshm_read_next_raw_frame(ffngxshm_raw_rd_ctx_t *ctx, unsigned int chn, int *num_pending, uint64_t *next_dts);

/**
 * returns the index of the currently active video channel with the highest frame dimensions
 * it is used by the encoder to determine which channel to read data from in case the designated channel is inactive
 *
 * returns FFNGXSHM_AGAIN in case of a failure
 */
int ffngxshm_get_highest_video_active_chn(ffngxshm_raw_rd_ctx_t *ctx);


/**
 * returns the current stats of the specified channel
 */
int ffngxshm_get_raw_channel_stats(ffngxshm_raw_rd_ctx_t *ctx, unsigned int chn, ffngxshm_raw_chn_stats *out);


/**
 * fill up the given array with information about the channels in the shm
 */
int ffngxshm_get_raw_channel_layout(ffngxshm_raw_rd_ctx_t *ctx, ffngxshm_raw_chn_hd_info *chn_hd_out);


/**
 * for debug purposes this function print out to the log a line with the last pts of every channel
 * it is used for making sure all channels are aligned
 */
void ffngxshm_raw_channels_snapshot(ffngxshm_raw_wr_ctx_t *ctx);
void ffngxshm_raw_channels_snapshot2(ffngxshm_raw_rd_ctx_t *ctx);


/**
 * Access params is an opque field that is set by the application in the shared memory and
 * helps to control the access to the stream i.e. determine if the access is public or require
 * authorization. The transcoder copy the value of the access parameters as is from the source all
 * the way out to the encoded stream. This makes it easier to enforce stream acceess accross all
 * variants and replication by simply copy the value from the source all the way to the border
 */
int ffngxshm_get_raw_acess_params(ffngxshm_raw_rd_ctx_t *ctx, ffngxshm_access_param *out);
int ffngxshm_set_raw_acess_params(ffngxshm_raw_wr_ctx_t *ctx, ffngxshm_access_param access_param);

#endif /* INCLUDE_FFNGXSHM_RAW_MEDIA_H_ */
