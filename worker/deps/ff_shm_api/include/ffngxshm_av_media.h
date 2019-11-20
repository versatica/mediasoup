/*
 * ffngxshm_av_media.h
 *
 *  Created on: May 3, 2018
 *      Author: Amir Pauker
 *
 *
 * read and write encoded media from / to shared memory
 */

#include <libavcodec/avcodec.h>
#include "ffngxshm.h"

#ifndef INCLUDE_FFNGXSHM_AV_MEDIA_H_
#define INCLUDE_FFNGXSHM_AV_MEDIA_H_

// we hide the implementation details behind this context from clients of this library
typedef struct{
	void            *rd_ctx;                   // the internal read context which is used for reading data from shm

	AVPacket        pkt_out;                   // whenever a call is made to read next video or read next audio this
	                                           // packet get filled with data. The caller must not attempt to unref it
	                                           // the content of this packet is overridden whenever the next frame is read (video or audio)
} ffngxshm_av_rd_ctx_t;

// we hide the implementation details behind this context from clients of this library
typedef struct{
	void            *wr_ctx;                   // the internal write context which is used for reading data from shm
} ffngxshm_av_wr_ctx_t;

/**
 * in case the reader uses the API flow control mechanism in order to
 * smooth fluctuation of the input stream, the read API will return per frame
 * one of these actions instructing the reader what to do with the returned frame
 */
typedef enum {
    ffngxshm_av_fc_none,             // some error occurred while reading, reader should try again without taking any action
    ffngxshm_av_fc_use_frame,        // reader should use the returned frame
    ffngxshm_av_fc_discard_frame,    // overflow - reader should discard this frame
    ffngxshm_av_fc_dup_prev_frame    // underflow - reader should duplicate the previous frame
} ffngxshm_av_flow_ctl_action;

extern const char *ffngxshm_av_flow_ctl_action_names[];

typedef struct {
    ffngxshm_shm_conf_t   conf;                 // shared memory configuration

    char                  *stream_name;         // the name of the stream to open for reading
    unsigned int          stats_win_size;       // the writer continuously collects stats about the stream such as fps and bitrate
                                                // it collects it as moving average. this parameter determines the window size in number of samples

    uint16_t              trgt_num_pending;     // the target number for pending frames in the buffer. frames over that threshold should be discard
} ffngxshm_av_init;

// struct that is used for returning av channel stats
typedef struct{
	double        mv_avg_interarrival_tm;       // moving average of frames inter-arrival time in milliseconds. Avg FPS = 1000 / Avg inter-arrival time
	double        mv_avg_frame_sz;              // moving average of frame size in bytes
	uint32_t      cur_avg_video_frame_dur;      // the current average video frame duration based on the PTS of oldest frame, PTS of most recent frame and
	                                            // the number of frames. This average is based on encoder clock
} ffngxshm_av_chn_stats;

/**
 * Creates an av reader (for encoded data) context, open the specified shared memory for reading and associate the
 * shm context with the reader context. If the function succeeds it returns the allocated
 * context in ctx_out, otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using ffngxshm_close_reader.
 */
int ffngxshm_open_av_reader(ffngxshm_av_init *init, ffngxshm_av_rd_ctx_t **ctx_out);
int ffngxshm_close_av_reader(ffngxshm_av_rd_ctx_t *rd_ctx);

// output parameters from ffngxshm_read_next_av_video_with_flow_ctl
typedef struct {
    uint64_t                      poll_interval;       // output the number of milliseconds the reader should sleep until the next poll
    int                           num_pending;         // number of pictures pending in the encoder buffer
    uint64_t                      most_recent_pts;
    uint64_t                      frame_dur;           // the source's current average frame duration, should be used in case the reader
                                                       // duplicate the previous frame to determine the PTS of the duplicated frame
    ffngxshm_av_flow_ctl_action   flow_ctl_action;     // the action the reader should do with the returned frame
    uint16_t                      video_rotation;      // the orientation of the source video i.e. 0, 90, 180 or 270
} ffngxshm_av_frame_info_t;

/**
 * Attempt to read a video frame from the associated shm.
 * This function implement Jitter buffer which aim to smooth input stream fluctuations.
 * If succeeded, the function will return the content of the frame in the rd_ctx->pkt_out
 * Note that the rd_ctx->pkt_out is reused i.e. the function unref it before reading the next
 * frame.
 * The caller should not unref rd_ctx->pkt_out
 *
 * In case the stream has meta-data e.g. SPS/PPS for h.264, the function will adds the meta-data
 * as side data to the packet. At the next read however if the meta-data hasn't change the side
 * data will be cleared. This behavior allows the decoder to determine whether the encoding parameters
 * has changed
 *
 * Return:
 *   FFNGXSHM_OK    - read successful
 *   FFNGXSHM_ERR   - fatal error (e.g. out of memory). abort
 *   FFNGXSHM_AGAIN - the reader is falling behind and should attempt again without delay
 *   FFNGXSHM_EOF   - content is not ready
 */
int ffngxshm_read_next_av_video_with_flow_ctl(ffngxshm_av_rd_ctx_t *rd_ctx, ffngxshm_av_frame_info_t *frame_info);

/**
 * allows the client to alter the target num pending threshold of the jitter buffer at run time
 */
int ffngxshm_set_trgt_num_pending(ffngxshm_av_rd_ctx_t *rd_ctx, uint16_t trgt_num_pending);

/**
 * Attempt to read a audio frame from the associated shm.
 * If succeeded, the function will return the content of the frame in the rd_ctx->pkt_out
 * Note that the rd_ctx->pkt_out is reused i.e. the function unref it before reading the next
 * frame.
 * The caller should not unref rd_ctx->pkt_out
 *
 * max_pts - the API will not return audio frames that are more recent than the specified max_pts
 *           this is used for aligning the video and audio channels
 *
 * Return:
 *   FFNGXSHM_OK    - read successful
 *   FFNGXSHM_ERR   - fatal error (e.g. out of memory). abort
 *   FFNGXSHM_AGAIN - the reader is falling behind and should attempt again without delay
 *   FFNGXSHM_EOF   - content is not ready
 *
 */
int ffngxshm_read_next_av_audio(ffngxshm_av_rd_ctx_t *rd_ctx, uint64_t max_pts);



/**
 * fills up the given AVCodecParameters with parameters from the video channel
 */
int ffngxshm_get_video_avcodec_parameters(ffngxshm_av_rd_ctx_t *rd_ctx, struct AVCodecParameters *out);

/**
 * fills up the given AVCodecParameters with parameters from the audio channel
 */
int ffngxshm_get_audio_avcodec_parameters(ffngxshm_av_rd_ctx_t *rd_ctx, struct AVCodecParameters *out);

/**
 * Each video and audio channel contains a moving average of frames inter-arrival time in milliseconds and
 * frame size in bytes. This function can be used to retrieve those stats from the specified channel.
 * in case the function returns FFNGXSHM_AGAIN it means the writer is currently updating the stats and the
 * reader should attempt to read it again
 */
int ffngxshm_get_av_chn_stats(ffngxshm_av_rd_ctx_t *rd_ctx, int chn, ffngxshm_av_chn_stats *out);

/**
 * Creates an av writer (for encoded data) context, open the specified shared memory for writing and associate the
 * shm context with the writer context. If the function succeeds it returns the allocated
 * context in ctx_out, otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using ffngxshm_close_av_writer.
 */
int ffngxshm_open_av_writer(ffngxshm_av_init *init, ffngxshm_av_wr_ctx_t **ctx_out);
int ffngxshm_close_av_writer(ffngxshm_av_wr_ctx_t *ctx, int time_wait);


/**
 * write the given encoded video frame to the associated shm in the specified channel.
 * The function clones the given packet (it doesn't ref it)
 */
int ffngxshm_write_av_video(ffngxshm_av_wr_ctx_t *ctx, unsigned int chn, AVPacket *pkt);

/**
 * write the given encoded audio frame to the associated shm in the specified channel.
 * The function clones the given packet (it doesn't ref it)
 */
int ffngxshm_write_av_audio(ffngxshm_av_wr_ctx_t *ctx, unsigned int chn, enum AVCodecID, AVPacket *pkt);

/**
 * Access params is an opque field that is set by the application in the shared memory and
 * helps to control the access to the stream i.e. determine if the access is public or require
 * authorization. The transcoder copy the value of the access parameters as is from the source all
 * the way out to the encoded stream. This makes it easier to enforce stream acceess accross all
 * variants and replication by simply copy the value from the source all the way to the border
 */
int ffngxshm_get_av_acess_params(ffngxshm_av_rd_ctx_t *ctx, ffngxshm_access_param *out);
int ffngxshm_set_av_acess_params(ffngxshm_av_wr_ctx_t *ctx, ffngxshm_access_param access_param);


#endif /* INCLUDE_FFNGXSHM_AV_MEDIA_H_ */
