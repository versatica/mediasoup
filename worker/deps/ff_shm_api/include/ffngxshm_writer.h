/*
 * ffngxshm_writer.h
 *
 *  Created on: May 15, 2018
 *      Author: Amir Pauker
 */

#include <libavcodec/avcodec.h>
#include "ffngxshm.h"

#ifndef INCLUDE_FFNGXSHM_WRITER_H_
#define INCLUDE_FFNGXSHM_WRITER_H_



// we hide the implementation details behind this context from clients of this library
typedef struct{
	void            *wr_ctx;                   // the internal write context which is used for writing data to shm
} ffngxshm_wr_ctx_t;

typedef struct {
	char                 *stream_name;         // the name of the stream to open for writing
	ffngxshm_shm_conf_t   conf;                 // shared memory configuration
} ffngxshm_writer_init;

/**
 * Creates a writer context, open the specified shared memory for writing and associate the
 * shm context with the writer context. If the function succeeds it returns the allocated
 * context in ctx_out, otherwise it returns an error and destroys all allocated memory.
 * The allocated context must be closed using ffngxshm_close_writer.
 */
int ffngxshm_open_writer(ffngxshm_writer_init *init, ffngxshm_wr_ctx_t **ctx_out);
int ffngxshm_close_writer(ffngxshm_wr_ctx_t *wr_ctx);

/**
 * Attempt to write the given raw video picture to the associated shm in the specified channel.
 * The function clones the given frame (it doesn't ref it)
 */
int ffngxshm_write_raw_video(ffngxshm_wr_ctx_t *wr_ctx, unsigned int chn, AVFrame *frame);

/**
 * Attempt to write the given raw audio frame to the associated shm in the specified channel.
 * The function clones the given frame (it doesn't ref it)
 */
int ffngxshm_write_raw_audio(ffngxshm_wr_ctx_t *wr_ctx, unsigned int chn, AVFrame *frame);



#endif /* INCLUDE_FFNGXSHM_WRITER_H_ */
