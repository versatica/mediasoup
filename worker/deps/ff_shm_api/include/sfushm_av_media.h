/*
* sfushm_av_media.h
*
* Created on: Oct 29, 2019
*         By: Maria Tverdostup 
*/

#include <stdlib.h>
#include <inttypes.h>

#ifndef INCLUDE_SFUSHM_AV_H_
#define INCLUDE_SFUSHM_AV_H_

#ifdef __cplusplus
extern "C" {
#endif


// codec codes (initially based on FLV codes, but some are non standard e.g. VP8 and OPUS)
// DO NOT ALTER THE ENUM VALUES. THOSE VALUES MUST MATCH THE CODEC VALUES in ngx_shm_av.h
#define SFUSHM_AV_VIDEO_CODEC_H264            7
#define SFUSHM_AV_VIDEO_CODEC_VP8            10
#define SFUSHM_AV_VIDEO_CODEC_MAX_ID         11

#define SFUSHM_AV_AUDIO_CODEC_OPUS            9
#define SFUSHM_AV_AUDIO_CODEC_AAC            10
#define SFUSHM_AV_AUDIO_CODEC_MAX_ID         12

#define SFUSHM_AV_LOG_LEVEL_ERR          4
#define SFUSHM_AV_LOG_LEVEL_WARN         5
#define SFUSHM_AV_LOG_LEVEL_INFO         7
#define SFUSHM_AV_LOG_LEVEL_DEBUG        8
#define SFUSHM_AV_LOG_LEVEL_TRACE        9

/////////////////////////////


// we don't use NGX code macros for obvious reasons
// however the codes should follow NGX error codes (see NGX_OK, NGX_ERROR, etc)
#define SFUSHM_AV_OK                 0
#define SFUSHM_AV_ERR               -1
#define SFUSHM_AV_AGAIN             -2
#define SFUSHM_AV_INVALID_SEQ       -3


#define SFUSHM_AV_PTS_UNSET ((uint64_t)-1)


// must match the value of STREAM_SHM_MAX_CHANNELS in ngx_stream_shm.h
#define SFUSHM_AV_MAX_NUM_CHANNELS 3


typedef struct {
	uint32_t          target_buf_ms; // target number of milliseconds to store in shared memory for this channel
	uint32_t          target_kbps;   // expected birtate in kbps
	uint32_t          ssrc;          // source RTP SSRC. Used for correlating with RTCP messages
	uint32_t          sample_rate;   // clock sample rate for the this channel
	uint8_t           num_chn;       // number of audio channels
	uint8_t           codec_id;      // codec id as one of SFUSHM_AV_VIDEO_CODEC_XXX constants
	unsigned          video:1;       // if set the channel contains video
	unsigned          audio:1;       // if set the channel contains audio
} sfushm_av_chn_conf_t;

typedef struct {
	const char             *log_file_name;      // full path for the log file
	unsigned int           log_level;           // default log level. one of SFUSHM_AV_LOG_LEVEL_XXX
	sfushm_av_chn_conf_t   channels[SFUSHM_AV_MAX_NUM_CHANNELS]; // channels configuration
	unsigned               redirect_stdio:1;    // if set log output will be redirected to stdout
} sfushm_av_conf_t;

// we hide the implementation details behind this context from clients of this library
typedef struct{
	void            *wr_ctx;
} sfushm_av_wr_ctx_t;

typedef struct {
	sfushm_av_conf_t    conf;               // context configuration
	char                *stream_name;       // the name of the stream to open for writing
    unsigned int        stats_win_size;     // the writer continuously collects stats about the stream such as fps and bitrate
                                            // it collects it as moving average. this parameter determines the window size in number of samples
} sfushm_av_writer_init_t;


typedef struct{
	const uint8_t          *data;           // pointer to the buffer that contains the data
	size_t                 len;             // number of bytes to copy from the buffer
	uint64_t               rtp_time;        // the frame RTP timestamp (in source time base)
	uint32_t               first_rtp_seq;   // the first RTP sequence number in this fragment
	uint32_t               last_rtp_seq;    // the last RTP sequence number in this fragment
	unsigned               begin:1;         // set when this fragment contains the start of the frame
	unsigned               end:1;           // set when this fragment is the last of this frame
	unsigned               complete:1;      // set if we received all the fragments of this frame
} sfushm_av_frame_frag_t;

// allocates, opens and initializes new SHM writer context.
// on success returns a new writer context that is ready for writing video and audio data
// on failure, returns non OK code and ctx_out is set to NULL
int sfushm_av_open_writer(sfushm_av_writer_init_t *init, sfushm_av_wr_ctx_t **ctx_out);

// close a context previously allocated with sfushm_av_open_writer. This function must be called
// at the end of the stream to free all allocated resources
int sfushm_av_close_writer(sfushm_av_wr_ctx_t *wr_ctx, int time_wait);

// write video / audio fragment to shared memory. These API allows the client to write fragment of access units as they arrive
// directly to shared memory. Whenever the begin flag is set, the library will start a new chunk for editing. Whenever the end flag
// is set the library will publish the chunk and make it available for readers. If the client starts a chunk but never send the end
// then starts a new chunk, the library will override the previously opened chunk
int sfushm_av_write_video(sfushm_av_wr_ctx_t *wr_ctx, sfushm_av_frame_frag_t *data);
int sfushm_av_write_audio(sfushm_av_wr_ctx_t *wr_ctx, sfushm_av_frame_frag_t *data);

// write complete RTCP message. The library need access to the RTCP stream for a/v sync as well as other
// meta data information.
int sfushm_av_write_rtcp(sfushm_av_wr_ctx_t *wr_ctx, sfushm_av_frame_frag_t *data);

// write opaque data to shared memory. this allows an external controller to set stream meta-data such as room state
// in the shared memory.
int sfushm_av_write_stream_metadata(sfushm_av_wr_ctx_t *wr_ctx, uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_SFUSHM_AV_H_ */
