/*
 * ffngxshm_log_header.h
 *
 *  Created on: May 4, 2018
 *      Author: Amir Pauker
 */

#ifndef INCLUDE_FFNGXSHM_LOG_HEADER_H_
#define INCLUDE_FFNGXSHM_LOG_HEADER_H_

#define FFNGXSHM_LOG_LEVEL_ERR          4
#define FFNGXSHM_LOG_LEVEL_WARN         5
#define FFNGXSHM_LOG_LEVEL_INFO         7
#define FFNGXSHM_LOG_LEVEL_DEBUG        8
#define FFNGXSHM_LOG_LEVEL_TRACE        9

void ffngxshm_change_log_level(unsigned int level);
void _ffngxshm_log(unsigned int level, const char* str, ...);
void ffngxshm_log_reopen();

#endif /* INCLUDE_FFNGXSHM_LOG_HEADER_H_ */
