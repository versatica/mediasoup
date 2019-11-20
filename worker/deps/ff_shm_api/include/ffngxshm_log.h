/*
 * ffngxshm_log.h
 *
 *  Created on: May 2, 2018
 *      Author: Amir Pauker
 *
 * Provide log facility for this library
 * The internal implementation is based on nginx log mechanism
 *
 *
 * ****   IMPORTANT NOTE ****
 * THIS IS THE INTERNAL HEADER FILE. FOR EXTERNAL USES, IMPORT ffngxshm_log_header.h
 * DO NOT IMPORT IT BY EXTERNAL FILES
 * IMPORT THIS FILE ONLY BY .c FILES
 *
 */

#include "ffngxshm_log_header.h"

#ifndef INCLUDE_FFNGXSHM_LOG_H_
#define INCLUDE_FFNGXSHM_LOG_H_

extern ngx_log_t global_log;

#define ffngxshm_log(level, str, ...) \
	if(global_log.log_level >= level) \
		_ffngxshm_log(level, str, __VA_ARGS__)

void ffngxshm_log_init(const char* filename, unsigned int level, unsigned int redirect_stdio);
void* ffngxshm_get_log();

#endif /* INCLUDE_FFNGXSHM_LOG_H_ */

