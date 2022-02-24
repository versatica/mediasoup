#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <fnmatch.h>
#include <dirent.h>
#include <sys/stat.h>
#include <math.h>


#define MAX_LABEL_LEN   200
#define MAX_DIR         2
#define MAX_FILES       3
#define MAX_TIME_ALIGN  10

//////// Defines and structs copied from LivelyBinLogs.hpp in mediasoup and must stay in sync with source


typedef struct {
  uint16_t            epoch_len;             // epoch duration in milliseconds
  uint16_t            packets_count;         // RTP packets per epoch
  uint16_t            packets_lost;
  uint16_t            packets_discarded;
  uint16_t            packets_retransmitted;
  uint16_t            packets_repaired;
  uint16_t            nack_count;            // number of NACK requests
  uint16_t            nack_pkt_count;        // number of NACK packets requested
  uint16_t            kf_count;              // Requests for key frame, PLI or FIR, see RequestKeyFrame()
  uint16_t            rtt;
  uint32_t            max_pts;
  uint32_t            bytes_count;
} call_stats_sample_t;


// Maximum number of samples in call_stats_record_t
#define CALL_STATS_BIN_LOG_RECORDS_NUM 30
// Length in bytes we need to skip over and start reading call_stats_sample_t
#define CALL_STATS_HEADER_LEN 120


typedef struct {
  uint64_t            start_tm;        // the record start timestamp in milliseconds
  char                call_id    [36]; // call id: uuid4() without \0
  char                object_id  [36]; // uuid4() no \0, producer or consumer id
  char                producer_id[36]; // uuid4() no \0, ZERO_UUID if source is producer, or consumer's corresponding producer id
  uint8_t             source;          // producer=0, consumer=1
  uint8_t             mime;            // mime type: unset=0, audio=1, video=2
  uint16_t            filled;          // number of filled records in the array below
  call_stats_sample_t samples[CALL_STATS_BIN_LOG_RECORDS_NUM];
} call_stats_record_t;


static const char *mime_string[3] = {
  "u",
  "a",
  "v"
};


static const char *source_string[2] = {
  "p",
  "c"
};


static const char *header[][3] = {
  {"call_id                             ", "Call ID",     "ID of a call"},
  {"object_id                           ", "Object ID",   "ID of a producer or consumer object"},
  {"producer_id                         ", "Producer ID", "ID of a consumer's producer or empty field"}, 
  {"start_ts",  "Start Time",            "The statime of the epoch (HH:MM:SS,sss)"},
  {"type ",     "Stream Type",           "0 - producer, 1 - consumer"},
  {"mime ",     "Audio/Video",           "0 - undefined, 1 - audio, 2 - video"},
  {"pkt_cnt ",  "Packets Count",         "Packets received or sent during epoch"},
  {"pkt_lost ", "Packets Lost",          "Packets lost during epoch"},
  {"pkt_disc ", "Packets Discarded",     "Packets discarded during epoch"},
  {"pkt_rtx ",  "Packets Retransmitted", "Packets Retransmitted during epoch"},
  {"pkt_rep ",  "Packets Repaired",      "Packets repaired during epoch"},
  {"nack_cnt ",  "NACK Count",            "NACKs during epoch"},
  {"nack_pkt ",  "NACK Packets",          "Number of NACK packets requested"},
  {"kf ",        "Keyframe Requests",     "Keyframe requests during epoch"},
  {"rtt ",       "RTT",                   "RTT in milliseconds"},
  {"max_pts ",   "Maximum PTS",           "The most recent PTS in a stream"},
  {"bytes_cnt ", "Bytes Count",           "Bytes received or sent during epoch"},
  NULL
};


typedef struct {
  char*       filename;   // input file name
  int         format;     //1 - CSV with comma, 2 - tab with headers line;
  uint16_t    time_align; // interval in milliseconds for time alignment i.e. convert variable length epoch to fixed length
} ms_binlog_config;


static void
print_headers()
{
    int i;
    for (i = 0; *header[i]; i++)
    {
      printf("%s\t", header[i][0]);
    }
    printf("\n");
}


int
time_alignment(
    uint64_t record_tm,
    uint16_t time_align,
		call_stats_sample_t *sample_in,
		call_stats_sample_t *sample_out,
		int out_len)
{
	call_stats_sample_t sample;
	int                 i;
	uint64_t            start_tm, end_tm, rec_tm;

#define TIME_ALIGNMENT(metric) \
		sample.metric = sample_in->metric * time_align / sample_in->epoch_len

#define TIME_ALIGNMENT_ACCURATE(metric) \
		sample.metric = round((double)sample_in->metric * (double)time_align / (double)sample_in->epoch_len)

	TIME_ALIGNMENT(packets_count);
	TIME_ALIGNMENT(packets_lost);
	TIME_ALIGNMENT(packets_discarded);
	TIME_ALIGNMENT(packets_retransmitted);
	TIME_ALIGNMENT(packets_repaired);
	TIME_ALIGNMENT(nack_count);
	TIME_ALIGNMENT(nack_pkt_count);
	TIME_ALIGNMENT(kf_count);
	TIME_ALIGNMENT(rtt);
	TIME_ALIGNMENT(bytes_count);
	sample.max_pts   = sample_in->max_pts;
	sample.epoch_len = time_align;
  rec_tm = record_tm + sample_in->epoch_len;

	start_tm = (rec_tm % time_align) ?
			(rec_tm / time_align + 1) * time_align : rec_tm;
	end_tm   = rec_tm + sample_in->epoch_len;

	for (i = 0; i < out_len && start_tm < end_tm; i++, start_tm += time_align)
  {
		sample_out[i]           = sample;
		sample_out[i].epoch_len = time_align * i;
	}

	return i;
}


int
format_csv(FILE* fd, ms_binlog_config *conf)
{
    char                        buf[sizeof(call_stats_record_t)*1000];
    size_t                      len;

    int                         n, m, k, i, num_rec, num_tm_align_rec;
    char                        *first;
    call_stats_record_t         *rec;
    char                        *samples_pos;
    call_stats_record_t         rec_tm_align[MAX_TIME_ALIGN];
    call_stats_sample_t         *sample;
    call_stats_sample_t         sample_tm_align[MAX_TIME_ALIGN];

    uint64_t                    record_start_ts;
    uint64_t                    sample_start_tm;
    uint16_t                    prev_sample_epoch_len, curr_sample_epoch_len;
    
    char                        label_buf[MAX_LABEL_LEN];
    size_t                      label_len;

    char                        call_id[37];
    char                        object_id[37]; 
    char                        producer_id[37]; 

    memset(call_id, sizeof(call_id), 0); 
    memset(object_id, sizeof(object_id), 0); 
    memset(producer_id, sizeof(producer_id), 0); 

    first = buf;
    len = sizeof(buf);
    num_tm_align_rec = 1;

    if (2 == conf->format)
      print_headers();

    while( (n = fread(first, sizeof(call_stats_record_t), len, fd)) > 0) //read(fd, first, len)) > 0 )
    {
      num_rec = n; // / sizeof(call_stats_record_t);
      //printf("n=%d recsize=%u num_rec=%d\n", n, sizeof(call_stats_record_t), num_rec);

      // go over all the records currently in the buffer (read from file)
      for (m = 0; m < num_rec; m++)
      {
        rec = &((call_stats_record_t*)buf)[m];
        //printf("\nCallStatsRecord filled=%d start_tm=%u\n", rec->filled, rec->start_tm);
        if (rec->filled > CALL_STATS_BIN_LOG_RECORDS_NUM)
          continue;

        memcpy(call_id, rec->call_id, 36);
        memcpy(object_id, rec->object_id, 36);
        memcpy(producer_id, rec->producer_id, 36);

        record_start_ts = rec->start_tm;
    
        samples_pos = ((char*)rec) + CALL_STATS_HEADER_LEN;
        prev_sample_epoch_len = curr_sample_epoch_len = 0; // we need delta between current and previous epoch len to time alignment function

        for (k = 0; k < rec->filled; k++)
        {
          sample = &((call_stats_sample_t*)samples_pos)[k];
          //printf("call_stats_sample_t epoch_len=%u count=%u lost=%u rtt=%u max_pts=%u bytes=%u\n", sample->epoch_len, sample->packets_count, sample->packets_lost, sample->rtt, sample->max_pts, sample->bytes_count);

          curr_sample_epoch_len = sample->epoch_len;

          sample_start_tm = rec->start_tm + curr_sample_epoch_len; // calculate sample's timestamp for output recording
          sample->epoch_len = curr_sample_epoch_len - prev_sample_epoch_len; // now it is delta from a previous sample

          prev_sample_epoch_len = curr_sample_epoch_len;

          if (conf->time_align)
          {
            num_tm_align_rec = time_alignment(rec->start_tm, conf->time_align, sample, sample_tm_align, MAX_TIME_ALIGN);
            sample = sample_tm_align;
          }

          for (i = 0; i < num_tm_align_rec; i++)
          {
            if (1 == conf->format)
              fprintf(stdout, 
                "%s"
                "%s,%s,%s"
                ",%"PRIu64
                ",%s,%s,%"PRIu16
                ",%"PRIu16",%"PRIu16",%"PRIu16
                ",%"PRIu16",%"PRIu16",%"PRIu16
                ",%"PRIu16",%"PRIu16",%"PRIu32",%"PRIu32"\n",
                label_buf, 
                call_id, object_id, producer_id,
                sample_start_tm + sample[i].epoch_len,
                source_string[rec->source], mime_string[rec->mime], sample[i].packets_count,
                sample[i].packets_lost, sample[i].packets_discarded, sample[i].packets_retransmitted,
                sample[i].packets_repaired, sample[i].nack_count, sample[i].nack_pkt_count,
                sample[i].kf_count, sample[i].rtt, sample[i].max_pts, sample[i].bytes_count);
            else if (2 == conf->format)
              fprintf(stdout, 
                "%s"
                "%s\t%s\t%s"
                "\t%"PRIu64
                "\t%s\t%s\t%"PRIu16
                "\t%"PRIu16"\t%"PRIu16"\t%"PRIu16
                "\t%"PRIu16"\t%"PRIu16"\t%"PRIu16
                "\t%"PRIu16"\t%"PRIu16"\t%"PRIu32"\t%"PRIu32"\n",
                label_buf, 
                call_id, object_id, producer_id,
                sample_start_tm + sample[i].epoch_len,
                source_string[rec->source], mime_string[rec->mime], sample[i].packets_count,
                sample[i].packets_lost, sample[i].packets_discarded, sample[i].packets_retransmitted,
                sample[i].packets_repaired, sample[i].nack_count, sample[i].nack_pkt_count,
                sample[i].kf_count, sample[i].rtt, sample[i].max_pts, sample[i].bytes_count);
          }
        }
      }

      if(n % sizeof(call_stats_record_t))
      {
          memmove(buf, buf + sizeof(call_stats_record_t) * num_rec, n % sizeof(call_stats_record_t));
          first = buf + (n % sizeof(call_stats_record_t));
          len = sizeof(buf) - (n % sizeof(call_stats_record_t));
      }
      else
      {
          first = buf;
          len = sizeof(buf);
      }
    }

    return 0;  
}


int main(int argc, char* argv[])
{
  int              c;
  FILE*            fd   = 0;
  int              rc   = 0;
  ms_binlog_config conf = {0};

  static char* usage =
      "usage: %s [ -f <format> ] [ -t <interval ms> ] log_name\n"
      "    log_name           - full path to the log file to parse or wildcard pattern in case -a is used\n"
      "    -f <format>        - format:\n"
      "                         1 = CSV with no headers, comma separated (default)\n"
      "                         2 = CSV with column headers, tab separated \n"
      "    -t <interval>      - optional align timestamps to fixed intervals in milliseconds\n"
      ;

  conf.time_align = 0; 
  conf.format     = 1; // CSV without headers, comma separated
  
  while ((c = getopt (argc, argv, "f:t:")) != -1)
  {
    switch(c)
    {
      case 'f':
        conf.format = atoi(optarg);
        break;
      case 't':
        conf.time_align = atoi(optarg);
        break;
      case '?':
        printf(usage, argv[0]);
        exit(0);
    }
  }

  if ((optind+1) > argc)
  {
    fprintf(stderr, "please specify file name\n");
    exit(1);
  }

  conf.filename = argv[optind];

  fd = fopen(conf.filename, "r+");
  if (!fd)
  {
      fprintf(stderr, "failed to open %s. err=%s\n", conf.filename, strerror(errno));
      exit(1);
  }

  rc = format_csv(fd, &conf);
  fclose(fd);
  
  return rc;
}
