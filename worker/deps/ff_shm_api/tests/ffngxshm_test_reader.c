/*
 * ffngxshm_test_reader.c
 *
 *  Created on: May 3, 2018
 *      Author: Amir Pauker
 */

#include <ffngxshm_av_media.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ffngxshm.h"
#include "ffngxshm_log_header.h"
#include "ffngxshm_raw_media.h"

#define FFNGXSHM_TEST_ACTIONS_QUEUE_SIZE 100

#define FFNGXSHM_TEST_LOG(str, ...) \
        fprintf(stderr, "- %s - %d : " str "\n", \
                __FUNCTION__,__LINE__, \
                ##__VA_ARGS__)

static char      src_stream_name[1024] = {'t','e','s','t','_','r','e','a','d','e','r',0};
static char      dst_raw_name[1024]    = {'t','e','s','t','_','r','a','w','_','w','r','i','t','e','r',0};
static char      dst_stream_name[1024] = {'t','e','s','t','_','w','r','i','t','e','r',0};
static char      *jitter_params;

static int need_setup = 0;

static ffngxshm_av_rd_ctx_t      *av_src_rd_ctx;
static ffngxshm_raw_wr_ctx_t     *raw_wr_ctx;
static ffngxshm_raw_rd_ctx_t     *raw_rd_ctx;
static ffngxshm_av_wr_ctx_t      *av_dst_wr_ctx;


static struct AVCodecParameters  v_codec_par;
static char                      v_codec_par_buf[1024];
static AVCodec                   *v_dec_codec;
static AVCodecContext            *v_dec_ctx;


static struct AVCodecParameters  a_codec_par;
static char                      a_codec_par_buf[1024];
static AVCodec                   *a_dec_codec;
static AVCodecContext            *a_dec_ctx;


static AVCodec                   *v_enc_codec;
static AVCodecContext            *v_enc_ctx;
static uint64_t                  last_dec_video_pts = (uint64_t)-1;

static AVCodec                   *a_enc_codec;
static AVCodecContext            *a_enc_ctx;

static uint64_t                  poll_interval = 10000;

static int                       extra_raw_video_ch_count = 0;

#define FFNGXSHM_TEST_VIDEO_CHN_ID 0
#define FFNGXSHM_TEST_AUDIO_CHN_ID 1

typedef enum{
    ffngxshm_av_tests_transcoding,
    ffngxshm_av_tests_jitter
} ffngxshm_av_tests;


typedef struct{
    ffngxshm_av_flow_ctl_action  action;
    uint32_t                     frame_dur;
} ffngxshm_av_actions_queue;

static ffngxshm_av_tests what_should_I_test = ffngxshm_av_tests_transcoding;

/***************************************************************************
 *
 *                      PROCESS COMMAND LINE ARGUMENT
 *
 ***************************************************************************/
static int
process_opt(int argc, char *argv[])
{
    int c;
    int i;

    jitter_params = NULL;

    static char* usage =
            "usage1: %s [ -s stream_name ] [ -r raw_shm_name ] [ -d out_stream_name ] [ -n]\n"
            "usage2: %s [ -s stream_name ] -m jitter <stats_win_size>,<fc_decay_fact>,<hist_update_intr>,<trgt_percentile_uf>,<trg_buf_fact>,<uf_hist_step>,<uf_hist_num_buk>\n"
            "-s stream_name      - the name of the source stream. default test_reader\n"
            "-r raw_shm_name     - the name of the raw shared memory to use. default test_raw_writer\n"
            "-d out_stream_name  - the name of the destination shared memory to use. default test_writer\n"
            "-n                  - if present remove existing shms and set up test data\n"
            "-e extra_video_chn  - additional number of extra video channels with raw data, maximum FFNGXSHM_MAX_NUM_CHANNELS - 2\n"
            "-m test_mode        - the name of the test to run. jitter for jitter buffer only test. transcoding for transcoding test\n"
            " stats_win_size     - the jitter buffer tracks stream's inter-arrival time using moving average. \n"
            "                      this parameters determine the size of the window in samples\n"
            " fc_decay_fact      - the jitter buffer maintains histogram  of underflow duration. This parameter determines how fast to decay the information\n"
            "                      in the histogram i.e. all values are multiplied by this factor per histogram update\n"
            " hist_update_intr   - how frequent to update the jitter buffer histogram\n"
            " trgt_percentile_uf - the jitter buffer determines the target buffer duration based on the underflow histogram and the\n"
            "                      specified percentile i.e. if this value is set to 90 and the 90 percentil of underflow duration is 200 ms\n"
            "                      then whenever the buffer length exceeds 200ms the jitter buffer will return drop picture command\n"
            " trg_buf_fact       - a factor to apply to the target buffer duration based on the underflow histogram\n"
            " uf_hist_step       - underflow histogram step duration in milliseconds. e.g. if set to 30 then the histogram will measure underflow distibution in steps of 30 ms\n"
            " uf_hist_num_buk    - underflow histogram number of buckets (note that there is a hard coded max limit)\n";


    while ((c = getopt (argc, argv, "s:r:d:m:e:n:")) != -1)
    {
        switch(c){
        case 's':
            if(strlen(optarg) > sizeof(src_stream_name)){
                printf("stream name too long. should be less than %zu\n", sizeof(src_stream_name));
                exit(1);
            }

            strcpy(src_stream_name, optarg);
            break;
        case 'r':
            if(strlen(optarg) > sizeof(src_stream_name)){
                printf("raw shm name too long. should be less than %zu\n", sizeof(dst_raw_name));
                exit(1);
            }

            strcpy(dst_raw_name, optarg);
            break;
        case 'd':
            if(strlen(optarg) > sizeof(src_stream_name)){
                printf("destination shm name too long. should be less than %zu\n", sizeof(dst_stream_name));
                exit(1);
            }

            strcpy(dst_stream_name, optarg);
            break;

        case 'n':
            need_setup = 1;
            break;

        case 'm':
            if(strcmp(optarg, "jitter") == 0){
                what_should_I_test = ffngxshm_av_tests_jitter;
            }
            break;

        case 'e':
            i = atoi(optarg);
            if (i >= 1 && i <= FFNGXSHM_MAX_NUM_CHANNELS - 2) {
                extra_raw_video_ch_count = i;
            }
            break;

        case '?':
            printf(usage, argv[0],argv[0]);
            exit(0);
        }
    }

    if(optind < argc){
        jitter_params = argv[optind];
    }
}

/***************************************************************************
 *
 * 		     SET UP A SHM BASED ON PRE-GENERATED SNAPSHOT
 *
 * i.e. copy stored snapshot uner /dev/shm
 *
 **************************************************************************/
static void
setup_test_shm()
{
    ffngxshm_av_init          rd_init;
    char                      dev_shm_name[2048];
    char                      cmd[2048];
    int                       rc;


    if (!need_setup)
        return;

    // remove any old shm
    rc = system("rm -f /dev/shm/nginx_0000_*");
    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to delete old shm segments. err=%s", strerror(errno));
        exit(1);
    }

    rd_init.stream_name = src_stream_name;
    rc = ffngxshm_stream_name_to_shm(rd_init.stream_name, dev_shm_name, sizeof(dev_shm_name));

    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to create /dev/shm name");
        exit(1);
    }

    sprintf( cmd, "cp %s %s", "../resources/test_reader_shm", dev_shm_name );
    rc = system(cmd);
    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to copy test_reader_shm to %s. err=%s",
                dev_shm_name, strerror(errno));
        exit(1);
    }
}

static void
init_shm_lib()
{
    ffngxshm_lib_init_t       lib_init;

    lib_init.log_file_name = "/var/log/sg/nginx/test_reader.log";
    lib_init.log_level = FFNGXSHM_LOG_LEVEL_TRACE;

    ffngxshm_init(&lib_init);
}

/**
 * initializes all the readers and writers context from/to the shm
 * i.e. read context from source shm, write context to raw shm
 * read from raw shm and write to dest shm
 */
static void
init_read_write_shm_ctx()
{
    ffngxshm_av_init          av_init;
    ffngxshm_raw_init         raw_init, raw_init_rd;
    int                       rc;
    int                       idx;

    memset(&av_init, 0, sizeof(av_init));
    memset(&raw_init, 0, sizeof(raw_init));
    memset(&raw_init_rd, 0, sizeof(raw_init_rd));

    av_init.stream_name        = src_stream_name;
    av_init.stats_win_size     = 500;
    av_init.trgt_num_pending   = 4;


    // open the reader for the source
    // ------------------------------------------------------------------------

    rc = ffngxshm_open_av_reader(&av_init, &av_src_rd_ctx);
    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to open source shm");
        exit(1);
    }


    // open raw writer
    // ------------------------------------------------------------------------
    raw_init.conf.raw_data = 1;
    raw_init.conf.channels[0].video          = 1;
    raw_init.conf.channels[0].height         = 480;
    raw_init.conf.channels[0].width          = 852;
    raw_init.conf.channels[0].num_frames     = 5;
    raw_init.conf.channels[0].frame_size     = 480*852*3 + 1024;

    raw_init.conf.channels[1].audio          = 1;
    raw_init.conf.channels[1].num_frames     = 20;
    raw_init.conf.channels[1].frame_size     = 8192;

    for(idx = 0; idx < extra_raw_video_ch_count; idx++) {
        raw_init.conf.channels[2+idx].video          = 1;
        raw_init.conf.channels[2+idx].height         = 480;
        raw_init.conf.channels[2+idx].width          = 852;
        raw_init.conf.channels[2+idx].num_frames     = 5;
        raw_init.conf.channels[2+idx].frame_size     = 480*852*3 + 1024;
    }

    raw_init.stream_name = dst_raw_name;
    raw_init.win_size    = 500;

    rc = ffngxshm_open_raw_writer(&raw_init, &raw_wr_ctx);

    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to create raw writer context");
        exit(1);
    }

    // open raw reader
    // ------------------------------------------------------------------------


    raw_init_rd.stream_name = dst_raw_name;
    rc = ffngxshm_open_raw_reader(&raw_init_rd, &raw_rd_ctx);

    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to create raw reader context");
        exit(1);
    }

    // make sure whatever we get back from the open reader match what we set for the writer
#define FFNGXSHM_CMP_PROP(prop) \
        if(raw_init.conf.prop != raw_init_rd.conf.prop){ \
            FFNGXSHM_TEST_LOG(#prop " mismatch - %d - %d ", raw_init.conf.prop, raw_init_rd.conf.prop); \
            exit(1); \
        }

    FFNGXSHM_CMP_PROP(raw_data)
    FFNGXSHM_CMP_PROP(channels[0].audio)
    FFNGXSHM_CMP_PROP(channels[0].video)
    FFNGXSHM_CMP_PROP(channels[0].height)
    FFNGXSHM_CMP_PROP(channels[0].width)

    FFNGXSHM_CMP_PROP(channels[1].audio)
    FFNGXSHM_CMP_PROP(channels[1].video)
    FFNGXSHM_CMP_PROP(channels[1].height)
    FFNGXSHM_CMP_PROP(channels[1].width)


    // open destination writer
    // ------------------------------------------------------------------------
    memset(&av_init, 0, sizeof(av_init));
    av_init.stream_name = dst_stream_name;

    av_init.conf.raw_data = 0;
    av_init.conf.channels[0].video          = 1;
    av_init.conf.channels[0].target_buf_ms  = 20000;
    av_init.conf.channels[0].target_fps     = 30;
    av_init.conf.channels[0].target_kbps    = 2500;

    av_init.conf.channels[1].audio          = 1;
    av_init.conf.channels[1].target_buf_ms  = 20000;
    av_init.conf.channels[1].target_fps     = 44;
    av_init.conf.channels[1].target_kbps    = 128;


    rc = ffngxshm_open_av_writer(&av_init, &av_dst_wr_ctx);

    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to create dst writer context");
        exit(1);
    }
}


static void
init_video_decoder_ctx()
{
    int                       rc;

    memset(&v_codec_par, 0, sizeof(v_codec_par));
    v_codec_par.extradata_size = sizeof(v_codec_par_buf);
    v_codec_par.extradata = v_codec_par_buf;
    rc = ffngxshm_get_video_avcodec_parameters(av_src_rd_ctx, &v_codec_par);
    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to read video codec parameters");
        exit(1);
    }

    v_dec_codec = avcodec_find_decoder(v_codec_par.codec_id);
    if(v_dec_codec==NULL) {
        FFNGXSHM_TEST_LOG("Unsupported video decoder %d!", v_codec_par.codec_id);
        exit(1);
    }

    v_dec_ctx = avcodec_alloc_context3(v_dec_codec);
    if(v_dec_ctx == NULL) {
        FFNGXSHM_TEST_LOG("out of memory!\n");
        exit(1);
    }

    if (avcodec_parameters_to_context(v_dec_ctx, &v_codec_par) < 0) {
        FFNGXSHM_TEST_LOG("Error initializing the video decoder context.\n");
        exit(1);
    }

    // Open codec
    if(avcodec_open2(v_dec_ctx, v_dec_codec, NULL)<0){
        FFNGXSHM_TEST_LOG("Error open the video decoder context.\n");
        exit(1);
    }
}


static void
init_audio_decoder_ctx()
{
    int   rc;

    memset(&a_codec_par, 0, sizeof(a_codec_par));
    a_codec_par.extradata_size = sizeof(a_codec_par_buf);
    a_codec_par.extradata = a_codec_par_buf;
    rc = ffngxshm_get_audio_avcodec_parameters(av_src_rd_ctx, &a_codec_par);
    if(rc < 0){
        FFNGXSHM_TEST_LOG("failed to read audio codec parameters");
        exit(1);
    }

    a_dec_codec = avcodec_find_decoder(a_codec_par.codec_id);
    if(a_dec_codec==NULL) {
        FFNGXSHM_TEST_LOG("Unsupported audio decoder! (%d)\n", a_codec_par.codec_id);
        exit(1);
    }

    a_dec_ctx = avcodec_alloc_context3(a_dec_codec);
    if(a_dec_ctx == NULL) {
        FFNGXSHM_TEST_LOG("out of memory!\n");
        exit(1);
    }

    if (avcodec_parameters_to_context(a_dec_ctx, &a_codec_par) < 0) {
        FFNGXSHM_TEST_LOG("Error initializing the audio decoder context.\n");
        exit(1);
    }

    // Open codec
    if(avcodec_open2(a_dec_ctx, a_dec_codec, NULL)<0){
        FFNGXSHM_TEST_LOG("Error open the audio decoder context.\n");
        exit(1);
    }
}


static void
init_video_encoder_ctx()
{
    int         rc;

    v_enc_codec = avcodec_find_encoder(AV_CODEC_ID_H264);

    if(v_enc_codec==NULL) {
        FFNGXSHM_TEST_LOG("Unsupported video encoder h264!");
        exit(1);
    }

    v_enc_ctx = avcodec_alloc_context3(v_enc_codec);
    if(v_enc_ctx == NULL) {
        FFNGXSHM_TEST_LOG("out of memory!");
        exit(1);
    }

    v_enc_ctx->height = v_dec_ctx->height;
    v_enc_ctx->width  = v_dec_ctx->width;

    //    v_enc_ctx->sample_aspect_ratio = v_dec_ctx->sample_aspect_ratio;

    /* take first format from list of supported formats */
    if (v_enc_codec->pix_fmts)
        v_enc_ctx->pix_fmt = v_enc_codec->pix_fmts[0];
    else
        v_enc_ctx->pix_fmt = v_dec_ctx->pix_fmt;
    /* video time_base can be set to whatever is handy and supported by encoder */
    v_enc_ctx->time_base.den = 30;
    v_enc_ctx->time_base.num = 1;
    v_enc_ctx->max_b_frames  = 0;
    v_enc_ctx->gop_size = 15;

    //preset: ultrafast, superfast, veryfast, faster, fast,
    //medium, slow, slower, veryslow, placebo
    av_opt_set(v_enc_ctx->priv_data,"preset","veryfast",0);
    //tune: film, animation, grain, stillimage, psnr,
    //ssim, fastdecode, zerolatency
    av_opt_set(v_enc_ctx->priv_data,"tune","zerolatency",0);
    //profile: baseline, main, high, high10, high422, high444
    av_opt_set(v_enc_ctx->priv_data,"profile","main",0);


    rc = avcodec_open2(v_enc_ctx, v_enc_codec, NULL);
    if (rc < 0){
        FFNGXSHM_TEST_LOG("failed to open encoder. err=%d.", rc);
        exit(1);
    }

    v_enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}


static void
init_audio_encoder_ctx()
{
    int      rc;

    /* Find the encoder to be used by its name. */
    if (!(a_enc_codec = avcodec_find_encoder(AV_CODEC_ID_AAC))) {
        FFNGXSHM_TEST_LOG("Could not find an AAC encoder.");
        exit(1);
    }

    a_enc_ctx = avcodec_alloc_context3(a_enc_codec);
    if (!a_enc_ctx) {
        FFNGXSHM_TEST_LOG( "Could not allocate an encoding context");
        exit(1);
    }

    /* Set the basic encoder parameters.
     * The input file's sample rate is used to avoid a sample rate conversion. */
    a_enc_ctx->channels       = 2;
    a_enc_ctx->channel_layout = av_get_default_channel_layout(2);
    a_enc_ctx->sample_rate    = 44100;
    a_enc_ctx->sample_fmt     = a_enc_codec->sample_fmts[0];
    a_enc_ctx->bit_rate       = 96000;

    /* Allow the use of the experimental AAC encoder. */
    a_enc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    if ((rc = avcodec_open2(a_enc_ctx, a_enc_codec, NULL)) < 0) {
        FFNGXSHM_TEST_LOG( "Could not open output codec (error '%s')",
                av_err2str(rc));
        exit(1);
    }
}


static void
process_video()
{
    int                          rc;
    AVFrame                      *frame;
    AVPacket                     enc_pkt;
    int                          i;
    int                          ch;
    uint64_t                     pts;
    ffngxshm_av_chn_stats        av_stats;
    ffngxshm_av_frame_info_t     frame_info;
    static int                   frames_cnt;


    ffngxshm_av_actions_queue    src_actions[FFNGXSHM_TEST_ACTIONS_QUEUE_SIZE];
    int                          src_act_tail, src_act_head, src_act_cnt;

    double                      action_hist[4];
    double                      actions_cnt;

    actions_cnt = 0;
    action_hist[0] = 0;
    action_hist[1] = 0;
    action_hist[2] = 0;
    action_hist[3] = 0;

    rc = 0;
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    src_act_tail = src_act_head = src_act_cnt = 0;


    rc = ffngxshm_get_av_chn_stats(av_src_rd_ctx, FFNGXSHM_TEST_VIDEO_CHN_ID, &av_stats);

    if(rc >= 0){
        FFNGXSHM_TEST_LOG("video channel stats: inter-arv-tm: avg=%.2f  frame-sz: avg=%.2f",
                av_stats.mv_avg_interarrival_tm, av_stats.mv_avg_frame_sz);
    }

    // reading next encoded frame from shm
    // --------------------------------------------------------------------
    //rc = ffngxshm_read_next_av_video(av_src_rd_ctx, &num_pending, &nxt_dts);
    rc = ffngxshm_read_next_av_video_with_flow_ctl(av_src_rd_ctx, &frame_info);
    poll_interval = frame_info.poll_interval;


    // updating actions histogram
    // --------------------------------------------------------------------
    action_hist[frame_info.flow_ctl_action] += 1;
    // applying decay factor
    action_hist[0] *= 0.99;
    action_hist[1] *= 0.99;
    action_hist[2] *= 0.99;
    action_hist[3] *= 0.99;
    actions_cnt = action_hist[0] + action_hist[1] + action_hist[2] + action_hist[3];

#define ffngxshm_actions_hist_pct(__action_enum__) ((int)((double)action_hist[__action_enum__]*100.0/actions_cnt))

    FFNGXSHM_TEST_LOG(
            "frame: act=%s pct_none=%d pct_use=%d pct_disc=%d pct_dup=%d",
            ffngxshm_av_flow_ctl_action_names[frame_info.flow_ctl_action],
            ffngxshm_actions_hist_pct(ffngxshm_av_fc_none),
            ffngxshm_actions_hist_pct(ffngxshm_av_fc_use_frame),
            ffngxshm_actions_hist_pct(ffngxshm_av_fc_discard_frame),
            ffngxshm_actions_hist_pct(ffngxshm_av_fc_dup_prev_frame));


    if(rc < 0 || ffngxshm_av_fc_none == frame_info.flow_ctl_action){
        FFNGXSHM_TEST_LOG("ffngxshm_next_video returned rc=%d. act=%s\n",
                rc, ffngxshm_av_flow_ctl_action_names[frame_info.flow_ctl_action]);
        return;
    }

    // push next action to the queue
    if(src_act_cnt >= FFNGXSHM_TEST_ACTIONS_QUEUE_SIZE){
        FFNGXSHM_TEST_LOG("actions queue too small");
        exit(1);
    }
    src_actions[src_act_tail].action = frame_info.flow_ctl_action;
    src_actions[src_act_tail].frame_dur = frame_info.frame_dur;
    src_act_tail = (src_act_tail + 1) % FFNGXSHM_TEST_ACTIONS_QUEUE_SIZE; // cyclic queue
    src_act_cnt++;


    FFNGXSHM_TEST_LOG("read encoded video packet. dts=%"PRId64" pts=%"PRId64" sz=%d poll_intr=%d action=%s",
            av_src_rd_ctx->pkt_out.dts, av_src_rd_ctx->pkt_out.pts, av_src_rd_ctx->pkt_out.size,
            poll_interval, ffngxshm_av_flow_ctl_action_names[frame_info.flow_ctl_action]);

    // pushing next encoded frame into decoder
    // --------------------------------------------------------------------
    // in case the action is duplicate previous it means that there is no available frame
    if(ffngxshm_av_fc_dup_prev_frame != frame_info.flow_ctl_action){
        rc = avcodec_send_packet(v_dec_ctx, &av_src_rd_ctx->pkt_out);

        if (rc == AVERROR_EOF){
            FFNGXSHM_TEST_LOG("video avcodec_send_packet EOF\n");
            return;
        }
        else if (rc < 0){
            FFNGXSHM_TEST_LOG("video avcodec_send_packet err=%d\n", rc);
            return;
        }
    }

    // This is not the best way to do it.  *** DO NOT COPY!!! ***
    frame = av_frame_alloc();

    if (!frame) {
        FFNGXSHM_TEST_LOG( "out of memory.\n");
        exit(1);
    }

    while (rc >= 0 && src_act_cnt) {

        // pulling decoded frame from decoder
        // --------------------------------------------------------------------

        // in case the current action is either use or discard it means we have
        // to consume a picture out of the decoder context. In the case of
        // duplicate previous we don't have to consume a picture
        if(src_actions[src_act_head].action != ffngxshm_av_fc_dup_prev_frame){

            rc = avcodec_receive_frame(v_dec_ctx, frame);

            if(rc == AVERROR(EAGAIN)){
                FFNGXSHM_TEST_LOG("video avcodec_receive_frame AGAIN\n");
                break;
            }

            if (rc < 0) {
                FFNGXSHM_TEST_LOG("video avcodec_receive_frame err=%d\n", rc);
                exit(1);
            }

            FFNGXSHM_TEST_LOG("successfully decoded video frame. width=%d height=%d dts=%"PRId64" pts=%"PRId64" sz=%d",
                    frame->width, frame->height, frame->pkt_dts, frame->pts, frame->pkt_size);

            // once we got a picture from the decoder context, we pop the action from the queue
            frame_info.flow_ctl_action = src_actions[src_act_head].action;
            src_act_cnt--;
            src_act_head = (src_act_head + 1) % FFNGXSHM_TEST_ACTIONS_QUEUE_SIZE; // cyclic queue


            // we need to discard this picture
            if(frame_info.flow_ctl_action == ffngxshm_av_fc_discard_frame){
                FFNGXSHM_TEST_LOG("discard frame with pts=%"PRId64, frame->pts);
                continue;
            }

            // we previously injected a frame and the current frame has pts which
            // precede the one we injected, we have to discard it
            if(last_dec_video_pts != (uint64_t)-1 && frame->pts <= last_dec_video_pts){
                FFNGXSHM_TEST_LOG("source frame precede injected frame. discarding "
                        "pts=%"PRId64" last_dec_pts=%"PRId64, frame->pts, last_dec_video_pts);
                continue;
            }

            // we set the last decoded pts in case we have to duplicate this frame and
            // fake the pts
            last_dec_video_pts = frame->pts;
        }
        // we have to duplicate the previous frame
        else{
            // pop the action from the queue
            frame_info.frame_dur = src_actions[src_act_head].frame_dur;
            src_act_cnt--;
            src_act_head = (src_act_head + 1) % FFNGXSHM_TEST_ACTIONS_QUEUE_SIZE; // cyclic queue

            // we havn't used any decded picture yet, just continue
            if(last_dec_video_pts == (uint64_t)-1){
                FFNGXSHM_TEST_LOG("got duplicate command by there are no pictures in the buffer");
                continue;
            }

            last_dec_video_pts += frame_info.frame_dur;

            rc = ffngxshm_write_raw_dup_prev_video_frame(raw_wr_ctx, FFNGXSHM_TEST_VIDEO_CHN_ID, last_dec_video_pts, 0);

            if(rc < 0){
                FFNGXSHM_TEST_LOG("fail to duplicate frame. rc=%d", rc);
                exit(1);
            }

            for(ch = 0; ch < extra_raw_video_ch_count; ch++) {
                rc = ffngxshm_write_raw_dup_prev_video_frame(raw_wr_ctx, 2+ch, last_dec_video_pts, 0);

                if(rc < 0){
                    FFNGXSHM_TEST_LOG("fail to duplicate frame. rc=%d ch=%d", rc, ch);
                    exit(1);
                }
            }

            continue;
        }

        // writing decoded frame into raw shm
        // --------------------------------------------------------------------

        rc = ffngxshm_write_raw_video(raw_wr_ctx, FFNGXSHM_TEST_VIDEO_CHN_ID, frame, 1);

        if(rc < 0){
            FFNGXSHM_TEST_LOG("failed to write raw video frame to shm\n");
            exit(1);
        }

        for(ch = 0; ch < extra_raw_video_ch_count; ch++) {
            rc = ffngxshm_write_raw_video(raw_wr_ctx, 2+ch, frame, 1);

            if(rc < 0){
                FFNGXSHM_TEST_LOG("failed to write raw video frame to shm rc=%d ch=%d\n", rc, ch);
                exit(1);
            }
        }
    }

    av_frame_free(&frame);

    encode:
    for(;;){
        // reading decoded frame from raw shm
        // --------------------------------------------------------------------
        rc = ffngxshm_read_next_raw_frame(raw_rd_ctx, FFNGXSHM_TEST_VIDEO_CHN_ID, &frame_info.num_pending, &frame_info.nxt_dts);

        if(rc < 0){
            if(rc == FFNGXSHM_AGAIN){
                FFNGXSHM_TEST_LOG("failed to read raw video frame to shm. rc=again");
                continue;
            }

            if(rc != FFNGXSHM_EOF){
                FFNGXSHM_TEST_LOG("failed to read raw video frame to shm. rc=%d", rc);
            }
            break;
        }

        FFNGXSHM_TEST_LOG("read raw video packet. pkt_dts=%"PRId64" pts=%"PRId64" pending=%d nxt_dts=%"PRId64,
                raw_rd_ctx->frame_out.pkt_dts, raw_rd_ctx->frame_out.pts, num_pending, nxt_dts);


        // encode the frame
        // --------------------------------------------------------------------
        raw_rd_ctx->frame_out.pict_type = AV_PICTURE_TYPE_NONE;
        pts = raw_rd_ctx->frame_out.pts;
        raw_rd_ctx->frame_out.pts = raw_rd_ctx->frame_out.pts * 90;
        //raw_rd_ctx->frame_out.pts = ((double)frames_cnt++) * (1.0 / 30.0) * 90000.0;

        FFNGXSHM_TEST_LOG("pushing video frame to encoder. width=%d height=%d dts=%"PRId64" pts=%"PRId64" (%"PRId64" 90khz)",
                raw_rd_ctx->frame_out.width, raw_rd_ctx->frame_out.height, raw_rd_ctx->frame_out.pkt_dts, pts, raw_rd_ctx->frame_out.pts);


        rc = avcodec_send_frame(v_enc_ctx, &raw_rd_ctx->frame_out);

        if (rc < 0){
            FFNGXSHM_TEST_LOG("encoder send frame failed err=%d", rc);
            exit(1);
        }

        for(;rc >= 0;){
            // read encoded frame from encoder context
            // --------------------------------------------------------------------
            rc = avcodec_receive_packet(v_enc_ctx, &enc_pkt);

            if(rc == AVERROR(EAGAIN)){
                FFNGXSHM_TEST_LOG("video encoder returned again");
                av_packet_unref(&enc_pkt);
                break;
            }

            if(rc < 0){
                FFNGXSHM_TEST_LOG("encoder receive frame failed err=%d", rc);
                exit(1);
            }

            // write the packet to destination shm (final stream)
            // --------------------------------------------------------------------

            // convert 90khz to milliseconds as expected by the shm API
            enc_pkt.dts /= 90;
            enc_pkt.pts /= 90;

            rc = ffngxshm_write_av_video(av_dst_wr_ctx, FFNGXSHM_TEST_VIDEO_CHN_ID, &enc_pkt);


            if (rc < 0){
                FFNGXSHM_TEST_LOG("failed to write encoded video to shm");
                exit(1);
            }

            FFNGXSHM_TEST_LOG("successfully wrote encoded video frame. dts=%"PRId64" pts=%"PRId64" sz=%d",
                    enc_pkt.dts, enc_pkt.pts, enc_pkt.size);

            av_packet_unref(&enc_pkt);
        }
    }

}


static void
process_audio()
{
    int                       rc;
    AVFrame                   *frame;
    AVPacket                  enc_pkt;
    size_t                    size;
    int                       i;
    uint64_t                  pts, nxt_dts;
    int                       num_pending;


    rc = 0;
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);


    for(;rc >= 0;){

        // reading next encoded frame from shm
        // --------------------------------------------------------------------
        rc = ffngxshm_read_next_av_audio(av_src_rd_ctx, last_dec_video_pts);

        if(rc < 0){
            //FFNGXSHM_TEST_LOG("ffngxshm_next_audio returned %d.\n", rc);
            break;
        }

        FFNGXSHM_TEST_LOG("read encoded audio packet. dts=%"PRId64" pts=%"PRId64" sz=%d last_dec_video_pts=%"PRId64,
                av_src_rd_ctx->pkt_out.dts, av_src_rd_ctx->pkt_out.pts, av_src_rd_ctx->pkt_out.size, last_dec_video_pts);

        // pushing next encoded frame into decoder
        // --------------------------------------------------------------------
        rc = avcodec_send_packet(a_dec_ctx, &av_src_rd_ctx->pkt_out);

        if (rc == AVERROR_EOF){
            FFNGXSHM_TEST_LOG("audio avcodec_send_packet EOF\n");
            break;
        }
        else if (rc < 0){
            FFNGXSHM_TEST_LOG("audio avcodec_send_packet err=%d\n", rc);
            break;
        }

        while (rc >= 0) {


            // pulling decoded frame from decoder
            // --------------------------------------------------------------------

            frame = av_frame_alloc();
            if (!frame) {
                FFNGXSHM_TEST_LOG( "out of memory.\n");
                exit(1);
            }

            rc = avcodec_receive_frame(a_dec_ctx, frame);

            if(rc == AVERROR(EAGAIN)){
                FFNGXSHM_TEST_LOG("audio avcodec_receive_frame AGAIN\n");
                av_frame_unref(frame);
                break;
            }

            if (rc < 0) {
                FFNGXSHM_TEST_LOG("audio avcodec_receive_frame err=%d\n", rc);
                exit(1);
            }

            FFNGXSHM_TEST_LOG("successfully decoded audio frame. dts=%"PRId64" pts=%"PRId64" sz=%d",
                    frame->pkt_dts, frame->pts, frame->pkt_size);


            // writing decoded frame into raw shm
            // --------------------------------------------------------------------

            rc = ffngxshm_write_raw_audio(raw_wr_ctx, FFNGXSHM_TEST_AUDIO_CHN_ID, frame, 1);

            if(rc < 0){
                FFNGXSHM_TEST_LOG("failed to write raw audio frame to shm\n");
                exit(1);
            }

            // reading decoded frame from raw shm
            // --------------------------------------------------------------------
            rc = ffngxshm_read_next_raw_frame(raw_rd_ctx, FFNGXSHM_TEST_AUDIO_CHN_ID, &num_pending, &nxt_dts);

            if(rc < 0){
                FFNGXSHM_TEST_LOG("failed to read raw audio frame to shm\n");
                exit(1);
            }

            FFNGXSHM_TEST_LOG("read raw audio packet. pkt_dts=%"PRId64" pts=%"PRId64" pending=%d nxt_dts=%"PRId64,
                    raw_rd_ctx->frame_out.pkt_dts, raw_rd_ctx->frame_out.pts, num_pending, nxt_dts);

            av_frame_unref(frame);

            // encode the frame
            // --------------------------------------------------------------------
            pts = raw_rd_ctx->frame_out.pts;
            raw_rd_ctx->frame_out.pts = ((double)raw_rd_ctx->frame_out.pts / 1000) * 44100;
            size = raw_rd_ctx->frame_out.linesize[0] * raw_rd_ctx->frame_out.channels;

            FFNGXSHM_TEST_LOG("pushing audio frame to encoder. dts=%"PRId64" pts=%"PRId64" (%"PRId64" 44.1Khz) sz=%zu",
                    raw_rd_ctx->frame_out.pkt_dts, pts, raw_rd_ctx->frame_out.pts, size);


            rc = avcodec_send_frame(a_enc_ctx, &raw_rd_ctx->frame_out);

            if (rc < 0){
                FFNGXSHM_TEST_LOG("encoder send frame failed err=%d", rc);
                exit(1);
            }

            for(;rc >= 0;){
                // read encoded frame from encoder context
                // --------------------------------------------------------------------
                rc = avcodec_receive_packet(a_enc_ctx, &enc_pkt);

                if(rc == AVERROR(EAGAIN)){
                    FFNGXSHM_TEST_LOG("audio encoder returned again");
                    av_packet_unref(&enc_pkt);
                    break;
                }

                if(rc < 0){
                    FFNGXSHM_TEST_LOG("encoder receive frame failed err=%d", rc);
                    exit(1);
                }

                // write the packet to destination shm (final stream)
                // --------------------------------------------------------------------

                // convert 44.1khz to milliseconds as expected by the shm API
                enc_pkt.dts = ((double)enc_pkt.dts / 44100) * 1000;
                enc_pkt.pts = ((double)enc_pkt.pts / 44100) * 1000;

                rc = ffngxshm_write_av_audio(av_dst_wr_ctx, FFNGXSHM_TEST_AUDIO_CHN_ID, AV_CODEC_ID_AAC, &enc_pkt);


                if (rc < 0){
                    FFNGXSHM_TEST_LOG("failed to write encoded audio to shm");
                    exit(1);
                }

                FFNGXSHM_TEST_LOG("successfully wrote encoded audio frame. dts=%"PRId64" pts=%"PRId64" sz=%d",
                        enc_pkt.dts, enc_pkt.pts, enc_pkt.size);

                av_packet_unref(&enc_pkt);
            }
        }
    }
}

/**
 * continuously read from the Jitter buffer and print out the sleep time and command
 * to stdout. The function may optionally get parameters as a string in the format
 * <stats_win_size>,<fc_decay_fact>,<hist_update_intr>,<trgt_percentile_uf>,<trg_buf_fact>,<uf_hist_step>,<uf_hist_num_buk>
 */
static int
test_jitter_buffer(char *jitter_params)
{
    ffngxshm_av_init            av_init;
    int                         rc;
    uint64_t                    poll_interval;
    uint64_t                    frame_dur;
    ffngxshm_av_flow_ctl_action flow_ctl_action;
    char                        buf[4096];
    char                        *p, *q;
    uint64_t                    cur_ts, prev_ts, delta_tm;
    int                         num_pending;
    uint64_t                    most_recent_pts;


    double                      action_hist[4];
    double                      actions_cnt;

    ffngxshm_time_update();

    memset(&av_init, 0, sizeof(av_init));

    actions_cnt    = 0;
    action_hist[0] = 0;
    action_hist[1] = 0;
    action_hist[2] = 0;
    action_hist[3] = 0;

    av_init.stream_name        = src_stream_name;
    av_init.stats_win_size     = 300;
    av_init.trgt_num_pending   = 4;

    // in case we are provided with jitter buffer parameters from command line
    // then parse whatever we can
    if(jitter_params){
        q = p = jitter_params;
        // stats_win_size
        for(;*q && *q != ',';q++);

        if(*q == ','){
            *q = 0;
            av_init.stats_win_size = atoi(p);
            p = ++q;
        }
        else{
            goto parse_params_done;
        }

        // fc_decay_fact
        for(;*q && *q != ',';q++);

        if(*q == ','){
            *q = 0;
            av_init.trgt_num_pending = atoi(p);
            p = ++q;
        }
        else{
            goto parse_params_done;
        }
    }
parse_params_done:

    printf("jitter buffer test: stream_name=%s stats_win_size=%d trgt_num_pending=%ui\n",
            av_init.stream_name, av_init.stats_win_size, av_init.trgt_num_pending);

    // open the reader for the source
    // ------------------------------------------------------------------------
    rc = ffngxshm_open_av_reader(&av_init, &av_src_rd_ctx);
    if(rc < 0){
        printf("failed to open source shm\n");
        exit(1);
    }

    prev_ts = ffngxshm_get_cur_timestamp();

    for(;;){
        ffngxshm_time_update();

        cur_ts = ffngxshm_get_cur_timestamp();
        delta_tm = cur_ts - prev_ts;
        prev_ts = cur_ts;


        // reading next encoded frame from shm
        // --------------------------------------------------------------------
        rc = ffngxshm_read_next_av_video_with_flow_ctl(av_src_rd_ctx, &poll_interval, &num_pending, &most_recent_pts, &frame_dur, &flow_ctl_action);

        action_hist[flow_ctl_action] += 1;

        // applying decay factor
        action_hist[0] *= 0.9;
        action_hist[1] *= 0.9;
        action_hist[2] *= 0.9;
        action_hist[3] *= 0.9;
        actions_cnt = action_hist[0] + action_hist[1] + action_hist[2] + action_hist[3];

#define ffngxshm_actions_hist_pct(__action_enum__) ((double)action_hist[__action_enum__]*100.0/actions_cnt)

        printf(
                "frame: act=%s pct_none=%.2f pct_use=%.2f pct_disc=%.2f pct_dup=%.2f "
                "poll_intr=%u frame_dur=%u srv_delta_tm=%u pts=%"PRId64"\n",
                ffngxshm_av_flow_ctl_action_names[flow_ctl_action],
                ffngxshm_actions_hist_pct(ffngxshm_av_fc_none),
                ffngxshm_actions_hist_pct(ffngxshm_av_fc_use_frame),
                ffngxshm_actions_hist_pct(ffngxshm_av_fc_discard_frame),
                ffngxshm_actions_hist_pct(ffngxshm_av_fc_dup_prev_frame),
                poll_interval, frame_dur, delta_tm, av_src_rd_ctx->pkt_out.pts);

        usleep(poll_interval);
    }

}

static int
test_transcode(){
    uint64_t   start_ts, end_ts;

    // copy a ready shm snapshot under /dev/shm for testing
    setup_test_shm();

    // initialize all the read and write shm context
    init_read_write_shm_ctx();

    // open and initialize video decoder
    init_video_decoder_ctx();

    // open and initialize audio decoder
    init_audio_decoder_ctx();

    // open and initialize video encoder
    init_video_encoder_ctx();

    // open and initialize audio encoder
    init_audio_encoder_ctx();

    for(;;){
        ffngxshm_time_update();
        start_ts = ffngxshm_get_cur_timestamp();

        FFNGXSHM_TEST_LOG("start loop time %"PRId64, start_ts);

        process_video();
        process_audio();

        ffngxshm_time_update();
        end_ts = ffngxshm_get_cur_timestamp();

        FFNGXSHM_TEST_LOG("end loop time %"PRId64" delta=%"PRId64, start_ts, (end_ts - start_ts));

        // subtract from the poll interval i.e. the sleep time the time it
        // took to decode and encode
        if(poll_interval + start_ts > end_ts){
            poll_interval -= (end_ts - start_ts);
            FFNGXSHM_TEST_LOG("sleeping for %u", poll_interval);
            usleep(poll_interval * 1000);
        }

    }
}

int
main(int argc, char *argv[])
{
    process_opt(argc, argv);

    // initialize the library that allows read / write from / to shm
    init_shm_lib();

    if(what_should_I_test == ffngxshm_av_tests_jitter){
        test_jitter_buffer(jitter_params);
    }
    else{
        test_transcode();
    }


    return 0;
}

