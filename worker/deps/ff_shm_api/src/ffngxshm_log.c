/*
 * ffngxshm_log.c
 *
 *  Created on: May 2, 2018
 *      Author: Amir Pauker
 */

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_log.h>
#include <ngx_files.h>
#include <ngx_errno.h>

static char* err_levels[] = {
    "",
    "emerg",
    "alert",
    "crit",
    "error",
    "warn",
    "notice",
    "info",
    "debug",
    "trace"
};


ngx_log_t                 global_log;
ngx_open_file_t           global_log_file;


void
ffngxshm_log_init(
        const char* filename,
        unsigned int level,
        unsigned int redirect_stdio)
{
    if(global_log_file.fd > 2){
        if(close(global_log_file.fd) < 0){
            fprintf(stderr, "ffngxshm_log_init - fail to close file. fd=%d %s\n",
                    global_log_file.fd, strerror(errno));
            exit(1);
        }
    }

    if(global_log_file.name.data){
        free(global_log_file.name.data);
    }

    memset(&global_log, 0, sizeof(global_log));
    memset(&global_log_file, 0, sizeof(global_log_file));

    global_log_file.name.len = strlen(filename) + 1;
    global_log_file.name.data = malloc(global_log_file.name.len);
    if(!global_log_file.name.data){
        fprintf(stderr, "fail to open log '%s'. out of memory. %s\n", filename, strerror(errno));
        exit(1);
    }

    strcpy(global_log_file.name.data, filename);

    global_log_file.fd = open(filename,
    		O_WRONLY|O_APPEND|O_CREAT,
    		S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);

    if(global_log_file.fd < 0){
    	fprintf(stderr, "fail to open log '%s'.  %s\n", filename, strerror(errno));
    	exit(1);
    }
    global_log.file = &global_log_file;
    global_log.log_level = level;
    global_log.disk_full_time = ngx_time() + 1;

    if(redirect_stdio){
        dup2(global_log_file.fd, 1);
        dup2(global_log_file.fd, 2);
    }
}


void
ffngxshm_log_reopen()
{
    if(global_log_file.fd <= 2 || !global_log_file.name.data){
        return;
    }

    if(close(global_log_file.fd) < 0){
        fprintf(stderr, "ffngxshm_log_init - fail to close file. fd=%d %s\n",
                global_log_file.fd, strerror(errno));
        exit(1);
    }

    global_log_file.fd = open(global_log_file.name.data,
            O_WRONLY|O_APPEND|O_CREAT,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);

    if(global_log_file.fd < 0){
        fprintf(stderr, "fail to open log '%s'.  %s\n", global_log_file.name.data, strerror(errno));
        exit(1);
    }
}


void
_ffngxshm_log(unsigned int level, const char* str, ...)
{
	va_list      args;
	ngx_log_t    *log;
    u_char       *p, *last, *msg;
    ssize_t      n;
    u_char       errstr[NGX_MAX_ERROR_STR];

    log = &global_log;

    last = errstr + NGX_MAX_ERROR_STR;

    p = ngx_cpymem(errstr, ngx_cached_err_log_time.data,
                   ngx_cached_err_log_time.len);

    p = ngx_slprintf(p, last, " [%s] ", err_levels[level]);

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

void*
ffngxshm_get_log()
{
	return &global_log;
}

void
ffngxshm_change_log_level(unsigned int level)
{
	global_log.log_level = level;
}
