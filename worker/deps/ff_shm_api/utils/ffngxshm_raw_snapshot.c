/*
 * ffngxshm_raw_snapshot.c
 *
 *  Created on: Aug 6, 2018
 *      Author: Amir Pauker
 *
 * small stand alone utility that can be used using command line arguments and
 * stdin commands to connect to raw video shared memory and save pictures into
 * bmp files
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <getopt.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avconfig.h>
#include <libavutil/imgutils.h>

#include "ngx_stream_shm.h"
#include "ngx_shm_av.h"
#include "ngx_shm_raw.h"
#include "ffngxshm_raw_media.h"


#define FFNGXSHM_SNAPSHOT_LOG_INFO(log,str,...)                                             \
        ngx_log_error(NGX_LOG_INFO,log,0,"- snap - %s %d - " str,     \
                __FUNCTION__,__LINE__,##__VA_ARGS__)

#define FFNGXSHM_SNAPSHOT_LOG_ERR(log,str,...)                                             \
        ngx_log_error(NGX_LOG_ERR,log,0,"- snap - %s %d - " str,     \
                __FUNCTION__,__LINE__,##__VA_ARGS__)

#define FFNGXSHM_SNAPSHOT_LOG_DEBUG(log,str,...)                                            \
        ngx_log_error(NGX_LOG_DEBUG,log,0,"- snap - %s %d - " str,    \
                __FUNCTION__,__LINE__,##__VA_ARGS__)


#define FFNGXSHM_SNAPSHOT_OK     0
#define FFNGXSHM_SNAPSHOT_ERROR -1

#define FFNGXSHM_SNAPSHOT_BUF_SIZE    1024*1024
#define FFNGXSHM_SNAPSHOT_LINE_SIZE   1024
#define FFNGXSHM_SNAPSHOT_HISTORY     1

#define FFNGXSHM_SNAPSHOT_INDEX_NOT_SET -1

// buffer into which the functions write their output before flashing it to the output file descriptor
// the last pointer point to the location of the next write in the output buffer
static u_char ffngxshm_snap_out_buf[FFNGXSHM_SNAPSHOT_BUF_SIZE];
static u_char *ffngxshm_snap_out_last = ffngxshm_snap_out_buf;
static u_char *ffngxshm_snap_out_end = ffngxshm_snap_out_buf + sizeof(ffngxshm_snap_out_buf) - 1; // make sure we have room for null terminator

#define ffngxshm_snap_init_out(ctx) ffngxshm_snap_out_last = ffngxshm_snap_out_buf

#define ffngxshm_snap_printf(ctx, str, ...) \
        ffngxshm_snap_out_last = ngx_slprintf(ffngxshm_snap_out_last, ffngxshm_snap_out_end, str, ##__VA_ARGS__)

#define ffngxshm_snap_dump(ctx) \
        *ffngxshm_snap_out_last = 0; \
        fwrite(ffngxshm_snap_out_buf, sizeof(u_char), (ffngxshm_snap_out_last - ffngxshm_snap_out_buf + 1), stdout); \
        fflush(stdout)

typedef struct {
    ffngxshm_raw_rd_ctx_t          *rd_ctx;

    // libav BMP encoder and software scaler that used for converting raw picture in
    // YUV pixel format to RGB then save into a file as BMP
    AVCodec                        *bmp_codec;
    AVCodecContext                 *bmp_ctx;
    struct SwsContext              *sws_ctx;
    uint8_t                        *rgb24_buf;
    size_t                         rgb24_buf_sz;


    ngx_log_t                      *log;                              // required by the av_shm and shm stream library
    int                            json_format;                       // set to 1 if the client requests output formatted as json object
} ffngxshm_snap_ctx_t;


static void ffngxshm_snap_print_help(ffngxshm_snap_ctx_t *ctx);


/*****************************************************************************
 *
 *                               FORMAT FUNCTION
 *
 * **************************************************************************/



/*
 * converts unix timestamp to date into the output buffer
 */
u_char *
ffngxshm_snap_dump_timestamp(
        uint64_t timestamp, u_char *buf, size_t size)
{
    time_t       t;
    struct tm    lt;
    int          n;

    t = timestamp / 1000; // remove ms part

    (void) localtime_r(&t, &lt);

    if((n = strftime(buf, size, "%D %T", &lt)) == 0){
        return buf;
    }

    buf += n;
    size -= n;

    return ngx_snprintf(buf, size - n, ",%uL", (unsigned int)(timestamp % 1000));
}

/*****************************************************************************/
/*****************************************************************************/

static int
ffngxshm_snap_connect(ffngxshm_snap_ctx_t *ctx, char *stream_name)
{
    ffngxshm_raw_init rd_init;

    // Init raw shm reader
    rd_init.stream_name = stream_name;

    if (ffngxshm_open_raw_reader(&rd_init, &ctx->rd_ctx) != FFNGXSHM_OK) {
        FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "failed to open stream\n");
        ctx->rd_ctx = NULL;
        return FFNGXSHM_SNAPSHOT_ERROR;
    }

    return FFNGXSHM_SNAPSHOT_OK;
}


static void
ffngxshm_snap_disconnect(ffngxshm_snap_ctx_t *ctx)
{
    ffngxshm_close_raw_reader(ctx->rd_ctx);
    ctx->rd_ctx = NULL;
}


static int
ffngxshm_snap_list_channels(ffngxshm_snap_ctx_t *ctx)
{
    ffngxshm_raw_chn_hd_info chn_hd[STREAM_SHM_MAX_CHANNELS];
    char                     delim;
    int                      chn;

    if(ffngxshm_get_raw_channel_layout(ctx->rd_ctx, chn_hd) != FFNGXSHM_OK){
        FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "failed to read shm channels layout\n");
        return FFNGXSHM_SNAPSHOT_ERROR;
    }

    if(ctx->json_format){
        delim = '[';
        for(chn = 0;chn < STREAM_SHM_MAX_CHANNELS;chn++){
            if(chn_hd[chn].video){
                ffngxshm_snap_printf(ctx,
                        "%c{"
                          "channel: %d,"
                          "width: %d,"
                          "height: %d"
                        "}",
                        delim, chn, chn_hd[chn].width, chn_hd[chn].height);
                delim = ',';
            }
        }
        ffngxshm_snap_printf(ctx,"]\n");
    }
    else{
        for(chn = 0;chn < STREAM_SHM_MAX_CHANNELS;chn++){
            if(chn_hd[chn].video){
                ffngxshm_snap_printf(ctx,"%d %d %d\n", chn, chn_hd[chn].width, chn_hd[chn].height);
            }
        }
    }

    return FFNGXSHM_SNAPSHOT_OK;
}

static int
ffngxshm_snap_save_picture(ffngxshm_snap_ctx_t *ctx, ngx_uint_t chn, char *output_path)
{
    AVFrame                   frame_rgb24;
    AVPacket                  bmp_pkt;
    size_t                    rgb24_buf_sz;
    int                       num_pending;
    uint64_t                  next_dts;
    int                       rc;
    FILE                      *out;

    rc = ffngxshm_read_next_raw_frame(ctx->rd_ctx, chn, &num_pending, &next_dts);

    if(rc != FFNGXSHM_OK){
        if(rc == FFNGXSHM_EOF || rc == FFNGXSHM_AGAIN){
            FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "picture not available\n");
            return FFNGXSHM_SNAPSHOT_OK;
        }
        FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "failed to read next picture\n");
        return FFNGXSHM_SNAPSHOT_ERROR;
    }

    if(!ctx->bmp_codec){
        ctx->bmp_codec = avcodec_find_encoder(AV_CODEC_ID_BMP);
        if(ctx->bmp_codec==NULL) {
            FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "Unsupported bmp encoder\n");
            return FFNGXSHM_SNAPSHOT_ERROR;
        }
    }

    if(!ctx->bmp_ctx){
        ctx->bmp_ctx = avcodec_alloc_context3(ctx->bmp_codec);
        if(ctx->bmp_ctx == NULL) {
            FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "out of memory!\n");
            return FFNGXSHM_SNAPSHOT_ERROR;
        }

        ctx->bmp_ctx->pix_fmt = AV_PIX_FMT_BGR24;
        ctx->bmp_ctx->time_base.den = 30;
        ctx->bmp_ctx->time_base.num = 1;
        ctx->bmp_ctx->height = ctx->rd_ctx->frame_out.height;
        ctx->bmp_ctx->width = ctx->rd_ctx->frame_out.width;
        ctx->bmp_ctx->sample_aspect_ratio = ctx->rd_ctx->frame_out.sample_aspect_ratio;
        if (avcodec_open2(ctx->bmp_ctx, ctx->bmp_codec, NULL) < 0){
            avcodec_free_context(&ctx->bmp_ctx);
            ctx->bmp_ctx = NULL;

            FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "failed to open bmp encoder!\n");
            return FFNGXSHM_SNAPSHOT_ERROR;
        }
    }

    if(!ctx->sws_ctx){
        ctx->sws_ctx = sws_getContext(
                ctx->rd_ctx->frame_out.width,
                ctx->rd_ctx->frame_out.height,
                ctx->rd_ctx->frame_out.format,
                ctx->rd_ctx->frame_out.width,
                ctx->rd_ctx->frame_out.height,
                AV_PIX_FMT_BGR24,
                SWS_BILINEAR,
                NULL,
                NULL,
                NULL
            );

        if(!ctx->sws_ctx){
            FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "failed to get software scaler\n");
            return FFNGXSHM_SNAPSHOT_ERROR;
        }
    }

    rgb24_buf_sz = av_image_get_buffer_size(
            AV_PIX_FMT_BGR24,
            ctx->rd_ctx->frame_out.width,
            ctx->rd_ctx->frame_out.height, 1);

    if(rgb24_buf_sz > ctx->rgb24_buf_sz){
        if(ctx->rgb24_buf){
            av_free(ctx->rgb24_buf);
            ctx->rgb24_buf_sz = 0;
        }

        ctx->rgb24_buf = (uint8_t *)av_malloc(rgb24_buf_sz*sizeof(uint8_t));
        if(!ctx->rgb24_buf){
            FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "out of memory\n");
            return FFNGXSHM_SNAPSHOT_ERROR;
        }

        ctx->rgb24_buf_sz = rgb24_buf_sz;
    }

    memset(&frame_rgb24, 0, sizeof(frame_rgb24));

    av_image_fill_arrays(frame_rgb24.data, frame_rgb24.linesize,
            ctx->rgb24_buf, AV_PIX_FMT_BGR24, ctx->rd_ctx->frame_out.width, ctx->rd_ctx->frame_out.height, 1);

    sws_scale(ctx->sws_ctx, (uint8_t const * const *)ctx->rd_ctx->frame_out.data,
            ctx->rd_ctx->frame_out.linesize, 0, ctx->rd_ctx->frame_out.height, frame_rgb24.data, frame_rgb24.linesize);

    bmp_pkt.data = NULL;
    bmp_pkt.size = 0;
    av_init_packet(&bmp_pkt);

    frame_rgb24.width  = ctx->rd_ctx->frame_out.width;
    frame_rgb24.height = ctx->rd_ctx->frame_out.height;

    if (avcodec_send_frame(ctx->bmp_ctx, &frame_rgb24) < 0){
        FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "failed to send frame to bmp encoder\n");
        return FFNGXSHM_SNAPSHOT_ERROR;
    }

    if (avcodec_receive_packet(ctx->bmp_ctx, &bmp_pkt) < 0){
        FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "bmp encoder receive frame failed\n");
        return FFNGXSHM_SNAPSHOT_ERROR;
    }

    out = fopen(output_path, "w");
    if(!out){
        FFNGXSHM_SNAPSHOT_LOG_ERR(ctx->log, "failed to open output file\n");
        return FFNGXSHM_SNAPSHOT_ERROR;
    }

    fwrite(bmp_pkt.data, 1, bmp_pkt.size, out);
    fclose(out);

    av_packet_unref(&bmp_pkt);

    return FFNGXSHM_SNAPSHOT_OK;
}




static void
ffngxshm_snap_print_help(ffngxshm_snap_ctx_t *ctx)
{
    ffngxshm_snap_printf(ctx,
           "shm_snapshot [-i <stream name> -n <channel id> -o <full path> [ -r <interval milliseconds> ] [ -c count ]]\n"
           "options:\n"
           " when input and output options are used, the tool will connect to the specified stream, read frame(s)\n"
           " from the specified channel and save them as bmp files to the specified output path. It can optionally\n"
           " loop in fixed time intervals and save the most recent picture each time to the specified file\n"
           "  -i           - specify a stream name and a channel id to read from\n"
           "  -n           - specify the channel index to read from\n"
           "  -o           - full path to the file into which snapshot frames should be stored.\n"
           "  -r           - optional, save most recent frame to the specified output file in fixed time intervals\n"
           "  -c           - optional, max number of intervals\n"
           "  -h           - print help\n"
           "  -l           - optional way to setup the log level for stderr\n"
           "  -j           - change the output format to json objects\n"

           "\n"
           "online mode (command line arguments not specified, reading commands from stdin):\n"
           "  h or help\n"
           "    print this menu\n"
           "\n"
           "  c or connect <name>\n"
           "    opens the shm stream specified by name for reading and waiting for further instructions\n"
           "    e.g. c b4a314fc-21cb-4408-a1a5-c083465c44f1_124_256x144_56\n"
           "\n"
           "  d or disconnect\n"
           "    closes the currently open shm\n"
           "\n"
           "  lc \n"
           "    lists channels in the open shm. output format: <channel id> <width> <height>\n"
           "\n"
           "  sc \n"
           "    set the channel to read from\n"
           "\n"
           "  s <full path>\n"
           "    save the content of the most recent frame to the specified file as bmp format\n"
           "\n"
           "  q or quit\n"
           "    exit the program"
           "\n"
    );

    ffngxshm_snap_dump(ctx);
}

void
main(int argc, char *argv[])
{
    int                       i, rc;
    size_t                    size;
    ssize_t                   n;
    ffngxshm_snap_ctx_t       *ctx;
    ngx_log_t                 log;
    ngx_open_file_t           log_file;
    ngx_pool_t                *pool;
    int                       ch;
    char                      stream_name[ngx_shm_name_max_len];
    int                       interval;
    int                       count;
    u_char                    cmd[FFNGXSHM_SNAPSHOT_LINE_SIZE];
    unsigned int              chn;                                        // current channel to read from
    u_char                    output_path[FFNGXSHM_SNAPSHOT_LINE_SIZE];   // full path to output file in to which pictures should be saved



    memset(&log, 0, sizeof(log));
    memset(&log_file, 0, sizeof(log_file));

    ngx_pid = getpid();
    ngx_time_init();

    // some nginx stuff (log and pool) that are required
    // for shm and av_shm libraries
    log_file.fd        = STDERR_FILENO;
    log.file           = &log_file;
    log.disk_full_time = ngx_time() + 1;

    pool = ngx_create_pool(4096, &log);
    if(!pool){
        printf("failed to create memory pool");
        exit(1);
    }

    ctx = ngx_pcalloc(pool, sizeof(ffngxshm_snap_ctx_t));
    if(!ctx){
        printf("failed to allocate context");
        exit(1);
    }

    ctx->log = &log;
    ctx->log->log_level = NGX_LOG_ERR;

    stream_name[0] = output_path[0] = 0;
    interval = 2000;
    count = 1;
    chn = 1;

    while ((ch = getopt (argc, argv, "i:o:r:c:hjl:")) != -1)
    {
        switch(ch){
        case 'i':
            if(strlen(optarg) + 1 > ngx_shm_name_max_len){
                FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "stream name too long. should be less than %uL\n", ngx_shm_name_max_len - 1);
                exit(1);
            }

            strcpy(stream_name, optarg);
            break;

        case 'n':
            chn = atoi(optarg);
            if(chn < 0 || chn >= STREAM_SHM_MAX_CHANNELS){
                FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "invalid channel index %d\n", chn);
                exit(1);
            }
            break;

        case 'o':
            if(strlen(optarg) + 1 > sizeof(output_path)){
                FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "output path too long. should be less than %uL\n", sizeof(output_path) - 1);
                exit(1);
            }
            strcpy(output_path, optarg);
            break;

        case 'r':
            interval = atoi(optarg);
            if(interval <= 50){
                FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "invalid repeat interval %d\n", interval);
                exit(1);
            }
            break;

        case 'c':
            count = atoi(optarg);
            break;

        case 'j':
            ctx->json_format = 1;
            break;

        case 'l':
            ctx->log->log_level = atoi(optarg);
            break;

        case 'h':
            ffngxshm_snap_print_help(ctx);
            return;
        }
    }

    // in case stream name was specified in command line argument then we sample that stream and don't get into
    // interactive mode
    if(stream_name[0]){
        if(!output_path[0]){
            FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "output path was not specified\n");
            exit(1);
        }

        if(ffngxshm_snap_connect(ctx, stream_name) != FFNGXSHM_SNAPSHOT_OK){
            FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "failed to connect to stream\n");
            exit(1);
        }

        for(;count > 0;count--){
            ffngxshm_snap_save_picture(ctx, chn, output_path);
            usleep(interval * 1000);
        }

        return;
    }

    // interactive mode
    for ( ;; ){

        printf(" > ");

        // reading the command line
        // we use getchar rather than getline since we like in the future to support
        // the up key to navigate the history
        // we restrict the read to sizeof(next_cmd->line) - 1 in order to reserve space for
        // null terminator
        for(i = 0;i < sizeof(cmd) - 1;i++){
            if( (ch = getchar()) == EOF){
                FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "failed to read from stdin\n");
                exit(1);
            }

            if(ch == '\n' || ch == '\r'){
                cmd[i] = 0;
                break;
            }
            else{
                cmd[i] = (ch & 0xFF);
            }
        }

        ffngxshm_snap_init_out(ctx);
        ngx_time_update();

        switch(cmd[0]){
        case 'h':
            ffngxshm_snap_print_help(ctx);
            break;
        case 'd':
            ffngxshm_snap_disconnect(ctx);
            break;

        case 'c':
            for(i = 1;i < sizeof(cmd) && cmd[i] && cmd[i] == ' ';i++);
            if(i >= sizeof(cmd) || !cmd[i]){
                FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "please specify stream name\n");
                break;
            }

            if(ffngxshm_snap_connect(ctx, &cmd[i]) != FFNGXSHM_SNAPSHOT_OK){
                FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "failed to connect to stream %s\n", &cmd[i]);
            }
            break;

        case 'l':
            if(cmd[1] == 'c'){

                if(!ctx->rd_ctx){
                    FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "please connect to a stream first\n");
                    break;
                }

                if(ffngxshm_snap_list_channels(ctx) == FFNGXSHM_SNAPSHOT_OK){
                    ffngxshm_snap_dump(ctx);
                }
            }
            break;

        case 's':
            if(cmd[1] == 'c'){
                for(i = 2;i < sizeof(cmd) && cmd[i] && cmd[i] == ' ';i++);
                if(i >= sizeof(cmd) || !cmd[i]){
                    FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "please specify channel index\n");
                    break;
                }
                chn = atoi(&cmd[i]);
                if(chn < 0 || chn >= STREAM_SHM_MAX_CHANNELS){
                    FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "invalid channel index %d\n", chn);
                    chn = 1;
                }
            }
            else{
                for(i = 2;i < sizeof(cmd) && cmd[i] && cmd[i] == ' ';i++);
                if(i >= sizeof(cmd) || !cmd[i]){
                    FFNGXSHM_SNAPSHOT_LOG_ERR(&log, "please specify output file full path\n");
                    break;
                }
                ffngxshm_snap_save_picture(ctx, chn, &cmd[i]);
            }
            break;
        case 'q':
            exit(0);
        }
    }

}


