/*
 * ffngxshm.h
 *
 *  Created on: May 2, 2018
 *      Author: Amir Pauker
 */

#include <stdint.h>
#include <stdlib.h>

#ifndef INCLUDE_FFNGXSHM_H_
#define INCLUDE_FFNGXSHM_H_

// we don't use NGX code macros for obvious reasons
// however the codes should follow NGX error codes (see NGX_OK, NGX_ERROR, etc)
#define FFNGXSHM_OK                 0
#define FFNGXSHM_ERR               -1
#define FFNGXSHM_AGAIN             -2
#define FFNGXSHM_EOF              -110
#define FFNGXSHM_ENC_PARAM_ERR    -112
#define FFNGXSHM_OUT_OF_SYNC      -113
#define FFNGXSHM_CHAN_NOT_SET     -114
#define FFNGXSHM_CLOSING          -115
#define FFNGXSHM_TIME_WAIT        -116


#define FFNGXSHM_PTS_UNSET ((uint64_t)-1)

typedef struct {
	const char          *log_file_name;         // full path for the log file
	unsigned int        log_level;              // default log level
	unsigned            redirect_sdtio:1;      // if set, stdout and stderr will be redirected to the log file
} ffngxshm_lib_init_t;


// must match the value of STREAM_SHM_MAX_CHANNELS in ngx_stream_shm.h
#ifndef FFNGXSHM_MAX_NUM_CHANNELS
#define FFNGXSHM_MAX_NUM_CHANNELS 3
#endif

// must be no more than SHM_NAME_LEN minus the prefix which is about 20 characters
// see NGX_SHM_NAME_PREFIX in ngx_stream_shm.h
#define FFNGXSHM_MAX_STREAM_NAME_LEN 60


// buffer size use for reading data from shm into process memory space
#define FFNGXSHM_DEFAULT_BUF_SZ        3*1024*1024
#define FFNGXSHM_DEFAULT_BUF_MAX_SZ    40*1024*1024

/**
 * Access param is an opaque flag which is set by the application in the shared memory
 * in applications that enforce stream authorization this field help to enforce the access
 * to the stream by carrying the access mask inside the stream
 *    *** IMPORTANT NOTE  ***
 * the type is defined in ngx_stream_shm.h but since we don't import this
 * file in order to avoid strong coupling we redefine it here. The type MUST match the one
 * that is defined in ngx_stream_shm.h
 */
typedef unsigned char ffngxshm_access_param;

/**
 * Used when opening a new shared memory for writing to setup the channels.
 * It us also used by reader to query the structure of the shm e.g. when encoder reads from raw data shm
 */
typedef struct{
	// based on the target number of seconds to store and estimated bitrate we derive the size of
	// the array that stores the frames in memory. The array of of fixed size and once it is full
	// the memory get recycled
    union{
        unsigned int     target_buf_ms;      // the target number of milliseconds to store in shared memory
        unsigned int     num_frames;         // for raw shm this is the explicit number of pictures/audio frames to store
    };
    union{
        unsigned int     target_kbps;        // the estimated bitrate of the stream
        unsigned int     frame_size;         // for raw shm this is the explicit frame size in bytes
    };
	unsigned int     target_fps;         // the expected frames per sec
	uint16_t         width;              // in case this channel contains video, the width of the picture in pixels
	uint16_t         height;             // in case this channel contains video, the height of the picture in pixels
	unsigned         video:1;            // true if this channel contains video data
	unsigned         audio:1;            // true if this channel contains audio data
} ffngxshm_chn_conf_t;

/**
 * Used when opening a new shared memory for writing.
 * It us also used by reader to query the structure of the shm e.g. when encoder reads from raw data shm
 */
typedef struct{
	ffngxshm_chn_conf_t        channels[FFNGXSHM_MAX_NUM_CHANNELS]; // configuration parameter per channel
	unsigned int               raw_data;                            // set to true if this shm will store raw data (not encoded)
} ffngxshm_shm_conf_t;

/**
 *
 */
typedef struct {
	char                *log_file_name;         // full path for the log file
	unsigned int        log_level;              // default log level
} ffngxshm_raw_chn_meta_t;


/**
 * Must be called once when the program starts to initialize the library
 */
void ffngxshm_init(ffngxshm_lib_init_t *init);

/**
 * Updates cache timestamps that are used for instance by log as well as
 * tracking stream progression based on serer's timestamp.
 * This function must be called periodically
 */
void ffngxshm_time_update();

/**
 * returned the cached unix timestamp in milliseconds. should be used instead of making repeated system calls
 */
uint64_t ffngxshm_get_cur_timestamp();

/**
 * returned the cached time string in the format YYYY/MM/DD HH:mm:ss
 * the time is specified in UTC
 * The output string DOES NOT include null terminator. the len field is set with the string length in bytes
 */
void ffngxshm_get_cur_utc_time_str(uint8_t **data, size_t *len);



/**
 * utility function for testing purposes.
 * It convert a stream name to its full path under /dev/shm
 */
int ffngxshm_stream_name_to_shm(char *stream_name, char *out, size_t buf_size);


#endif /* INCLUDE_FFNGXSHM_H_ */
