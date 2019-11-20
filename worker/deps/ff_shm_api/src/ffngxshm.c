/*
 * ffngxshm.c
 *
 *  Created on: May 2, 2018
 *      Author: Amir Pauker
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_log.h>
#include <ngx_shm_av.h>

#include "ffngxshm.h"
#include "ffngxshm_log.h"

/*
 * in order to avoid exposing nginx code directly to the transcode code we don't use
 * STREAM_SHM_MAX_CHANNELS directly in header files of this library and use instead
 * FFNGXSHM_MAX_NUM_CHANNELS. However these two macros MUST have the same values otherwise
 * the defined shared memory will be corrupted
 */
#if STREAM_SHM_MAX_CHANNELS != FFNGXSHM_MAX_NUM_CHANNELS
#error macro for max channels in ngx_stream_shm.h is not the same as in ffngxshm.h
#endif

void
ffngxshm_init(ffngxshm_lib_init_t *init)
{
	ngx_time_init();
	ngx_time_update();

	ngx_pid = getpid();

	ffngxshm_log_init(init->log_file_name, init->log_level, init->redirect_sdtio);
}

/**
 * Updates cache timestamps that are used for instance by log as well as
 * tracking stream progression based on serer's timestamp.
 * This function must be called periodically
 */
void
ffngxshm_time_update()
{
    ngx_time_update();
}


/**
 * utility function for testing purposes.
 * It convert a stream name to its full path under /dev/shm
 */
int
ffngxshm_stream_name_to_shm(char *stream_name, char *out, size_t buf_size)
{
	ngx_str_t         name;

	if(buf_size < SHM_NAME_LEN + sizeof("/dev/shm")){
		return FFNGXSHM_ERR;
	}

	memcpy(out, "/dev/shm", sizeof("/dev/shm") - 1);

	out += sizeof("/dev/shm") - 1;

	name.data = (u_char*)stream_name;
	name.len = strlen(stream_name);

	ngx_stream_shm_str_t_to_shm_name(name, ffngxshm_get_log(), out, rtmp);

	return FFNGXSHM_OK;
}


/**
 * returned the cached unix timestamp in milliseconds. should be used instead of making repeated system calls
 */
uint64_t ffngxshm_get_cur_timestamp()
{
	return ((uint64_t) (ngx_timeofday())->sec * 1000 + (ngx_timeofday())->msec);
}

/**
 * returned the cached time string in the format YYYY/MM/DD HH:mm:ss
 * the time is specified in UTC
 * The output string DOES NOT include null terminator. the len field is set with the string length in bytes
 */
void ffngxshm_get_cur_utc_time_str(uint8_t **data, size_t *len)
{
	*data = (uint8_t*)ngx_cached_err_log_time.data;
	*len = ngx_cached_err_log_time.len;
}
