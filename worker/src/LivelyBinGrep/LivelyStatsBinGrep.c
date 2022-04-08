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
#include <sys/time.h>
#include <math.h>
#include <getopt.h>


#define MAX_TIME_ALIGN        10
#define MAX_RECORDS_IN_BUFFER 1000

#define FORMAT_CSV_COMMA   1
#define FORMAT_CSV_HEADERS 2
#define FORMAT_RAW         3

#define LOAD_TEST_MAX_FILES    200
#define LOAD_TEST_FILE_MAX_DUR 1800000
#define LOAD_TEST_FILE_MIN_DUR 2000
#define LOAD_TEST_MIN_SLEEP    1950

#define UUID_CHAR_LEN 36 
#define ZEROS_UUID    "00000000-0000-0000-0000-000000000000"

//////// Defines and structs copied from LivelyBinLogs.hpp in mediasoup and must stay in sync with source
//////// TBD: if this tool stays within mediasoup project (inside sfu docker) then remove duplication and include LivelyBinLogs.hpp


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
#define CALL_STATS_BIN_LOG_SAMPLING    2000

// Bytes to skip over and start reading call_stats_sample_t
#define CALL_STATS_HEADER_LEN 120

typedef struct {
  uint64_t            start_tm;                   // the record start timestamp in milliseconds
  char                call_id    [UUID_CHAR_LEN]; // call id: uuid4() without \0
  char                object_id  [UUID_CHAR_LEN]; // uuid4() no \0, producer or consumer id
  char                producer_id[UUID_CHAR_LEN]; // uuid4() no \0, ZEROS_UUID if source is producer, or consumer's corresponding producer id
  uint8_t             source;                     // producer=0, consumer=1
  uint8_t             mime;                       // mime type: unset=0, audio=1, video=2
  uint16_t            filled;                     // number of filled records in the array below
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
  char*       filename;             // input file name
  int         format;               // 1 - CSV with comma, 2 - tab with headers line; 3 - copy of original bin data
  uint16_t    time_align;           // interval in milliseconds for time alignment i.e. convert variable length epoch to fixed length
  uint64_t    start_ts;             // start and end timestamps in milliseconds
  uint64_t    dur;                  // data timespan in milliseconds
  int         no_callid;            // do not include callid into output in format 1 or 2
  int         dedup;                // remove duplicates
  uint64_t    dedup_last_ts;        // in case dedup is on, stores the last seen timestamp
  uint16_t    load_test_num_cfiles; // number of consumers stats produced for load testing, in a single file
  uint16_t    load_test_num_pfiles; // number of concurrent test files with producer stats produced by this tool for load testing
} ms_binlog_config;


uint8_t* 
fill_with_rand_hex(uint8_t *p, int cnt)
{
  static char hx[] = "0123456789abcdef";
  int i, r;

  for (i = 0; i < cnt; i++)
  {
    r = rand() % 16;
    *p++ = hx[r];
  }
  return p;
}

//Result is "00000000-0000-0000-0000-000000000000\0" i.e. 37 symbols string
void 
rand_pseudo_uuid(uint8_t *dst)
{
  uint8_t *ptr = dst;

  memset(dst, 0, UUID_CHAR_LEN + 1);
  ptr = fill_with_rand_hex(ptr, 8);
  *ptr++ = '-';
  ptr = fill_with_rand_hex(ptr, 4);
  *ptr++ = '-';
  ptr = fill_with_rand_hex(ptr, 4);
  *ptr++ = '-';
  ptr = fill_with_rand_hex(ptr, 4);
  *ptr++ = '-';
  ptr = fill_with_rand_hex(ptr, 8);
}


void
print_headers(ms_binlog_config *conf)
{
    int i;

    i = (conf->no_callid) ? 1 : 0; // assume that call_id is the first column

    for (; *header[i]; i++)
    {
      printf("%s\t", header[i][0]);
    }
    
    printf("\n");
}


#define SET_RAND_RECORD(f, s, r) rec.samples[j].f = s + rand() % r;
#define SET_RAND_VIDEO_RECORD(f, s, r) rec.samples[j].f = (rec.mime == 2) ? s + rand() % r : 0;

// Creates a single test file with data for several consumers
int
load_test_cfiles(ms_binlog_config *conf)
{
  int                 fd;
  char                filename[256];

	uint64_t            start_ts[LOAD_TEST_MAX_FILES];
  uint64_t            last_ts[LOAD_TEST_MAX_FILES];
  uint64_t            end_ts[LOAD_TEST_MAX_FILES];
	uint64_t            active[LOAD_TEST_MAX_FILES];
  uint64_t            tmp;
  uint16_t            active_consumers_cnt;

  uint8_t             object_id[LOAD_TEST_MAX_FILES][UUID_CHAR_LEN + 1]; 
  uint8_t             producer_id[LOAD_TEST_MAX_FILES][UUID_CHAR_LEN + 1];
  
  int                 i, j;
  int                 count;
  int                 source, mime;

  call_stats_record_t rec;
  call_stats_sample_t sample;
  call_stats_sample_t prev_sample[LOAD_TEST_MAX_FILES];

	struct timeval             tv;
	uint64_t                   start_tm, cur_tm;
	useconds_t                 sleep;

	memset(start_ts, 0, sizeof(start_ts));
	memset(last_ts, 0, sizeof(last_ts));
	memset(end_ts, 0, sizeof(end_ts));
	memset(active, 0, sizeof(active));
	memset(&rec, 0, sizeof(rec));
  memset(&sample, 0, sizeof(sample));
	memset(prev_sample, 0, sizeof(prev_sample));
  memset(object_id, 0, sizeof(object_id));
  memset(producer_id, 0, sizeof(producer_id));

	if (gettimeofday(&tv, NULL) < 0)
  {
		fprintf(stderr, "gettimeofday failed. err=%s\n", strerror(errno));
		exit(1);
	}
	
  start_tm = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  
  snprintf(filename, sizeof(filename), "c_%s_%lu.bin.log", conf->filename, start_tm);

  srand(tv.tv_usec);

	for (i = 0; i < conf->load_test_num_cfiles; i++)
  {
		// start and end times
		start_ts[i] = start_tm + (rand() % (5 * LOAD_TEST_FILE_MIN_DUR)); // start sooner than just wait
		end_ts[i] = start_tm + LOAD_TEST_FILE_MIN_DUR + (rand() % (LOAD_TEST_FILE_MAX_DUR - LOAD_TEST_FILE_MIN_DUR));
    if (end_ts[i] < start_ts[i])
    {
      tmp = end_ts[i]; end_ts[i] = start_ts[i]; start_ts[i] = tmp;
    }
    
    active[i] = 1;
    rand_pseudo_uuid(object_id[i]);
    rand_pseudo_uuid(producer_id[i]);
  }

  fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (fd < 0)
  {
    fprintf(stderr, "failed to open file %s. err=%s\n", filename, strerror(errno));
    exit(1);
  } 

	for (count = 0; count < 0xFFFFFF; count++)
  {
		if (gettimeofday(&tv, NULL) < 0)
    {
			fprintf(stderr, "gettimeofday failed. err=%s\n", strerror(errno));
			exit(1);
		}

		cur_tm = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		for (i = 0, active_consumers_cnt = 0; i < conf->load_test_num_cfiles; i++)
    {
      if (0 == active[i])
      {
        continue;
      }
      if (cur_tm < start_ts[i])
      {
        active_consumers_cnt++;
        continue;
      }

			rec.start_tm  = last_ts[i] ? 
          last_ts[i] + CALL_STATS_BIN_LOG_SAMPLING + rand() % 100 : 
          cur_tm;

      memset(rec.call_id, 0, UUID_CHAR_LEN); // in case input file name is shorter don't want garbage
      memcpy(rec.call_id, conf->filename, UUID_CHAR_LEN);
      memcpy(rec.object_id, object_id[i], UUID_CHAR_LEN);
      memcpy(rec.producer_id, producer_id[i], UUID_CHAR_LEN);
      rec.source = 1;
      rec.mime = 1 + i % 2; // odd i for audio, even for video
      rec.filled = CALL_STATS_BIN_LOG_RECORDS_NUM;

      prev_sample[i].max_pts = cur_tm + 500;
      prev_sample[i].epoch_len = CALL_STATS_BIN_LOG_SAMPLING;

      for (j = 0; j < CALL_STATS_BIN_LOG_RECORDS_NUM; j++)
      {
        memset(&(rec.samples[j]), 0, sizeof(call_stats_sample_t));
        
        rec.samples[j].epoch_len = CALL_STATS_BIN_LOG_SAMPLING * (j + 1);
        SET_RAND_RECORD(packets_count, 0, 2000)
        SET_RAND_RECORD(packets_lost, 0, 100)
        SET_RAND_RECORD(packets_discarded, 0, 20)
        SET_RAND_VIDEO_RECORD(packets_retransmitted, 0, 10)
        SET_RAND_VIDEO_RECORD(packets_repaired, 0, 10)
        SET_RAND_VIDEO_RECORD(nack_count, 0, 10)
        SET_RAND_VIDEO_RECORD(nack_pkt_count, 0, 10)
        SET_RAND_VIDEO_RECORD(kf_count, 0, 3)
        SET_RAND_RECORD(rtt, 50, 300)
        SET_RAND_RECORD(max_pts, prev_sample[i].max_pts, 1000)
        SET_RAND_RECORD(bytes_count, 1000, 1000000)

        prev_sample[i] = rec.samples[j];
      }

      last_ts[i] = rec.start_tm + CALL_STATS_BIN_LOG_SAMPLING * CALL_STATS_BIN_LOG_RECORDS_NUM;

			if (write(fd, &rec, sizeof(rec)) < 0)
      {
				fprintf(stderr, "failed to write to file %d. err=%s\n", fd, strerror(errno));
				exit(1);
			}

      if (cur_tm > end_ts[i])
      {
        active[i] = 0; // wrote at least one record for each consumer
      }
      active_consumers_cnt++;
		}

    if (0 == active_consumers_cnt)
    { 
      close(fd);
      return 0;
    }

    sleep = (LOAD_TEST_MIN_SLEEP + rand() % 200) * 1000;
		usleep(sleep);
	}

  close(fd);
  return 0;
}


int
load_test_pfiles(ms_binlog_config *conf)
{
  char                filename[LOAD_TEST_MAX_FILES][256];
  int                 fd[LOAD_TEST_MAX_FILES]; // set into -1 explicitly after file is closed, not to be confused with error state
  uint64_t            last_ts[LOAD_TEST_MAX_FILES];
	uint64_t            end_ts[LOAD_TEST_MAX_FILES];
  int                 fopened_count;
  
  char                object_id[LOAD_TEST_MAX_FILES][UUID_CHAR_LEN + 1]; 
  int                 source, mime;
  int                 i, j, count;

  call_stats_record_t rec;
  call_stats_sample_t sample;
  call_stats_sample_t prev_sample[LOAD_TEST_MAX_FILES];

	struct timeval             tv;
	uint64_t                   cur_tm;
	useconds_t                 sleep;

	memset(fd, 0, sizeof(fd));
  memset(last_ts, 0, sizeof(last_ts));
	memset(end_ts, 0, sizeof(end_ts));
	memset(&rec, 0, sizeof(rec));
  memset(&sample, 0, sizeof(sample));
	memset(prev_sample, 0, sizeof(prev_sample));

	if (gettimeofday(&tv, NULL) < 0)
  {
		fprintf(stderr, "gettimeofday failed. err=%s\n", strerror(errno));
		exit(1);
	}

	cur_tm = tv.tv_sec * 1000 + tv.tv_usec / 1000;

  srand(tv.tv_usec);

	for (i = 0; i < conf->load_test_num_pfiles; i++)
  {
		// end time for this file
		end_ts[i] = cur_tm +
				(rand() % (LOAD_TEST_FILE_MAX_DUR + 1 - LOAD_TEST_FILE_MIN_DUR)) +
				LOAD_TEST_FILE_MIN_DUR;

    rand_pseudo_uuid(object_id[i]);

    snprintf(filename[i], sizeof(filename) / LOAD_TEST_MAX_FILES, "p_%s_%s_%lu.bin.log", conf->filename, object_id[i], cur_tm + i);
		fd[i] = open(filename[i], O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (fd[i] < 0) {
			fprintf(stderr, "failed to open file %s. err=%s\n",
					filename[i], strerror(errno));
			exit(1);
		}
  }

	for (count = 0; count < 0xFFFFFF; count++)
  {
		if (gettimeofday(&tv, NULL) < 0) {
			fprintf(stderr, "gettimeofday failed. err=%s\n", strerror(errno));
			exit(1);
		}

		cur_tm = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		for (i = 0, fopened_count = 0; i < conf->load_test_num_pfiles; i++)
    {
      if (fd[i] < 0)
        continue;

			rec.start_tm  = last_ts[i] ? 
          last_ts[i] + CALL_STATS_BIN_LOG_SAMPLING + rand() % 100 : 
          cur_tm;

      memset(rec.call_id, 0, UUID_CHAR_LEN); // in case input file name is shorter don't want garbage
      memcpy(rec.call_id, conf->filename, UUID_CHAR_LEN);
      memcpy(rec.object_id, object_id[i], UUID_CHAR_LEN);
      memcpy(rec.producer_id, ZEROS_UUID, UUID_CHAR_LEN);
      rec.source = 0;
      rec.mime = 1 + i % 2; // odd i for audio, even for video
      rec.filled = CALL_STATS_BIN_LOG_RECORDS_NUM;

      prev_sample[i].max_pts = cur_tm + 1000;
      prev_sample[i].epoch_len = CALL_STATS_BIN_LOG_SAMPLING;

      for (j = 0; j < CALL_STATS_BIN_LOG_RECORDS_NUM; j++)
      {
        memset(&(rec.samples[j]), 0, sizeof(call_stats_sample_t));

        rec.samples[j].epoch_len = CALL_STATS_BIN_LOG_SAMPLING * (j + 1);
        SET_RAND_RECORD(packets_count, 0, 2000)
        SET_RAND_RECORD(packets_lost, 0, 100)
        SET_RAND_RECORD(packets_discarded, 0, 20)
        SET_RAND_VIDEO_RECORD(packets_retransmitted, 0, 10)
        SET_RAND_VIDEO_RECORD(packets_repaired, 0, 10)
        SET_RAND_VIDEO_RECORD(nack_count, 0, 10)
        SET_RAND_VIDEO_RECORD(nack_pkt_count, 0, 10)
        SET_RAND_VIDEO_RECORD(kf_count, 0, 3)
        SET_RAND_RECORD(rtt, 50, 300)
        SET_RAND_RECORD(max_pts, prev_sample[i].max_pts, 1000)
        SET_RAND_RECORD(bytes_count, 1000, 1000000)

        prev_sample[i] = rec.samples[j];
      }
      
      last_ts[i] = rec.start_tm + CALL_STATS_BIN_LOG_SAMPLING * CALL_STATS_BIN_LOG_RECORDS_NUM;

			if (write(fd[i], &rec, sizeof(rec)) < 0)
      {
				fprintf(stderr, "failed to write to file %d. err=%s\n",
						fd[i], strerror(errno));
				exit(1);
			}

			if (cur_tm > end_ts[i])
      {
				close(fd[i]);
        fd[i] = -1;
			}
      else
      {
        fopened_count++; // count files which are still open
      }
		} // for 0...conf->load_test_num_pfiles

    sleep = (LOAD_TEST_MIN_SLEEP + rand() % 200) * 1000;
		usleep(sleep);

    if (0 == fopened_count)
      return 0;
	}

  return 0;
}


int
time_alignment(
    uint64_t record_tm,
    uint16_t            sample_dur,
    uint16_t            time_align,
		call_stats_sample_t *sample_in,
    uint64_t            *sample_ts,
		call_stats_sample_t *sample_out,
		int out_len)
{
	call_stats_sample_t sample;
	int                 i;
	uint64_t            start_tm, end_tm;

#define TIME_ALIGNMENT(metric) \
		sample.metric = (sample_in->metric > 1) ? sample_in->metric * time_align / sample_dur : sample_in->metric

#define TIME_ALIGNMENT_ACCURATE(metric) \
		sample.metric = (sample_in->metric > 1) ? round((double)sample_in->metric * (double)time_align / (double)sample_dur) : sample_in->metric

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

	start_tm = record_tm;
	end_tm   = record_tm + sample_dur;

	for (i = 0; i < out_len && start_tm < end_tm; i++, start_tm += time_align)
  {
		sample_out[i]           = sample;
		sample_out[i].epoch_len = time_align * i;
    sample_ts[i] = start_tm + sample_out[i].epoch_len;
	}

	return i;
}


int
format_output(FILE* fd, ms_binlog_config *conf)
{
    char                        buf[sizeof(call_stats_record_t) * MAX_RECORDS_IN_BUFFER];
    size_t                      len;    // in number of records, not bytes
    call_stats_record_t         *first;

    int                         m, k, i, num_rec, num_tm_align_rec;
    call_stats_record_t         *rec;
    call_stats_sample_t         *sample;
    char                        *samples_pos;

    call_stats_sample_t         sample_align[MAX_TIME_ALIGN];
    uint64_t                    sample_ts[MAX_TIME_ALIGN];
    uint16_t                    prev_sample_epoch_len, curr_sample_epoch_len;
    uint64_t                    sample_start_tm;
    
    char                        call_id[UUID_CHAR_LEN+1];
    char                        object_id[UUID_CHAR_LEN+1]; 
    char                        producer_id[UUID_CHAR_LEN+1]; 

    uint64_t                    start_ts, end_ts;

    start_ts = conf->start_ts;
    num_tm_align_rec = 1;
    
    memset(call_id, sizeof(call_id), 0); 
    memset(object_id, sizeof(object_id), 0); 
    memset(producer_id, sizeof(producer_id), 0); 

    first = (call_stats_record_t*)buf;
    len = MAX_RECORDS_IN_BUFFER;

    if (FORMAT_CSV_HEADERS == conf->format)
      print_headers(conf);

    while( (num_rec = fread(first, sizeof(call_stats_record_t), len, fd)) > 0)
    {
      //printf("recsize=%u num_rec=%d\n", sizeof(call_stats_record_t), num_rec);
      // go over all the records currently in the buffer (read from file)
      for (m = 0; m < num_rec; m++)
      {
        rec = &((call_stats_record_t*)buf)[m];
        //printf("\nRecord source=%d mime=%d filled=%d start_tm=%u\n", rec->source, rec->mime, rec->filled, rec->start_tm);
        if (rec->filled > CALL_STATS_BIN_LOG_RECORDS_NUM)
          continue;
               
        if (FORMAT_RAW == conf->format)
        {
          if (!start_ts)
            start_ts = rec->start_tm;

          end_ts = conf->dur ? (start_ts + conf->dur) : (uint64_t)-1;
         
          if (rec->start_tm < start_ts || rec->start_tm > end_ts)
            continue;

          if (write(fileno(stdout), rec, sizeof(call_stats_record_t)) < 0) {
          	fprintf(stderr, "failed to write record to stdout. err=%s\n", strerror(errno));
          	exit(1);
          }
          continue;
        }

        // CSV formats
        memcpy(call_id, rec->call_id, UUID_CHAR_LEN);
        memcpy(object_id, rec->object_id, UUID_CHAR_LEN);
        memcpy(producer_id, rec->producer_id, UUID_CHAR_LEN);
    
        samples_pos = ((char*)rec) + CALL_STATS_HEADER_LEN;
        prev_sample_epoch_len = curr_sample_epoch_len = 0; // we need delta between current and previous epoch len to pass to time alignment function

        for (k = 0; k < rec->filled; k++)
        {
          sample = &((call_stats_sample_t*)samples_pos)[k];
          //printf("call_stats_sample_t epoch_len=%u count=%u lost=%u rtt=%u max_pts=%u bytes=%u\n", sample->epoch_len, sample->packets_count, sample->packets_lost, sample->rtt, sample->max_pts, sample->bytes_count);
          curr_sample_epoch_len = sample->epoch_len;
          if (conf->time_align)
          {
            sample_start_tm = rec->start_tm + sample->epoch_len;

            // just round it up or down to time_align:
            sample_start_tm = (sample_start_tm % conf->time_align) ?
                (sample_start_tm / conf->time_align + (sample_start_tm % conf->time_align + conf->time_align / 2) / conf->time_align) * conf->time_align :
                sample_start_tm;

            num_tm_align_rec = time_alignment(sample_start_tm, curr_sample_epoch_len - prev_sample_epoch_len, conf->time_align, sample, sample_ts, sample_align, MAX_TIME_ALIGN);
            sample = sample_align;
          }
          else
          {
            sample_ts[0] = rec->start_tm + sample->epoch_len; // a sample's real timestamp
          }
          prev_sample_epoch_len = curr_sample_epoch_len;

          for (i = 0; i < num_tm_align_rec; i++)
          {
            // in case remove duplicates is on and this record's timestamp
            // is older than the most recent record then skip it
            if (conf->dedup)
            {
              if (sample_ts[i] <= conf->dedup_last_ts)
              {
                continue;
              } else {
                conf->dedup_last_ts = sample_ts[i];
              }
            }

            if (!start_ts)
              start_ts = sample_ts[i];

            end_ts = conf->dur? (start_ts + conf->dur) : (uint64_t)-1;
            if (sample_ts[i] < start_ts || sample_ts[i] > end_ts)
              continue;

            if (FORMAT_CSV_COMMA == conf->format)
            {
              if (conf->no_callid)
              {
                fprintf(stdout, 
                  "%s,%s"
                  ",%"PRIu64
                  ",%s,%s,%"PRIu16
                  ",%"PRIu16",%"PRIu16",%"PRIu16
                  ",%"PRIu16",%"PRIu16",%"PRIu16
                  ",%"PRIu16",%"PRIu16",%"PRIu32",%"PRIu32"\n",
                  object_id, producer_id,
                  sample_ts[i],
                  source_string[rec->source], mime_string[rec->mime], sample[i].packets_count,
                  sample[i].packets_lost, sample[i].packets_discarded, sample[i].packets_retransmitted,
                  sample[i].packets_repaired, sample[i].nack_count, sample[i].nack_pkt_count,
                  sample[i].kf_count, sample[i].rtt, sample[i].max_pts, sample[i].bytes_count);
              }
              else
              {
                fprintf(stdout, 
                  "%s,%s,%s"
                  ",%"PRIu64
                  ",%s,%s,%"PRIu16
                  ",%"PRIu16",%"PRIu16",%"PRIu16
                  ",%"PRIu16",%"PRIu16",%"PRIu16
                  ",%"PRIu16",%"PRIu16",%"PRIu32",%"PRIu32"\n",
                  call_id, object_id, producer_id,
                  sample_ts[i],
                  source_string[rec->source], mime_string[rec->mime], sample[i].packets_count,
                  sample[i].packets_lost, sample[i].packets_discarded, sample[i].packets_retransmitted,
                  sample[i].packets_repaired, sample[i].nack_count, sample[i].nack_pkt_count,
                  sample[i].kf_count, sample[i].rtt, sample[i].max_pts, sample[i].bytes_count);
              }
            }
            else if (FORMAT_CSV_HEADERS == conf->format)
            {
              if (conf->no_callid)
              {
                fprintf(stdout, 
                  "%s\t%s"
                  "\t%"PRIu64
                  "\t%s\t%s\t%"PRIu16
                  "\t%"PRIu16"\t%"PRIu16"\t%"PRIu16
                  "\t%"PRIu16"\t%"PRIu16"\t%"PRIu16
                  "\t%"PRIu16"\t%"PRIu16"\t%"PRIu32"\t%"PRIu32"\n",
                  object_id, producer_id,
                  sample_ts[i],
                  source_string[rec->source], mime_string[rec->mime], sample[i].packets_count,
                  sample[i].packets_lost, sample[i].packets_discarded, sample[i].packets_retransmitted,
                  sample[i].packets_repaired, sample[i].nack_count, sample[i].nack_pkt_count,
                  sample[i].kf_count, sample[i].rtt, sample[i].max_pts, sample[i].bytes_count);
              }
              else
              {
                fprintf(stdout, 
                  "%s\t%s\t%s"
                  "\t%"PRIu64
                  "\t%s\t%s\t%"PRIu16
                  "\t%"PRIu16"\t%"PRIu16"\t%"PRIu16
                  "\t%"PRIu16"\t%"PRIu16"\t%"PRIu16
                  "\t%"PRIu16"\t%"PRIu16"\t%"PRIu32"\t%"PRIu32"\n",
                  call_id, object_id, producer_id,
                  sample_ts[i],
                  source_string[rec->source], mime_string[rec->mime], sample[i].packets_count,
                  sample[i].packets_lost, sample[i].packets_discarded, sample[i].packets_retransmitted,
                  sample[i].packets_repaired, sample[i].nack_count, sample[i].nack_pkt_count,
                  sample[i].kf_count, sample[i].rtt, sample[i].max_pts, sample[i].bytes_count);
              }
            }
          }
        }
      } // for m ... num_rec
    } // while fread()

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
      "  log_name                 - full path to the log file to parse or call id to be used if load test options are present\n"
      "  -f <format>              - format:\n"
      "                              1 = CSV with no headers, comma separated (default)\n"
      "                              2 = CSV with column headers, tab separated \n"
      "                              3 = raw format, binary copy of original data \n"
      "  -t <interval>             - optional align timestamps to fixed intervals in milliseconds\n"
      "  -s <timestamp>            - optional start timestamp\n"
      "  -d <duration>             - duration in milliseconds (if only dur is specified it is taken from the start of the stream)\n"
      "  -D                        - optional remove duplicate records\n"
      "  -x                        - optional exclude callid column from output if in format 1 or 2\n"
      "  --load-test-consumers <n> - load test simulation, create stats file with n consumers\n"
      "  --load-test-producers <n> - load test simulation, create n producers stats files\n"
      ;

  static struct option long_options[] = {
			{ "format",                  1, 0, 'f'    },
			{ "align-time-interval",     1, 0, 't'    },
			{ "start-time",              1, 0, 's'    },
			{ "duration",                1, 0, 'd'    },
			{ "de-dup",                  0, 0, 'D'    },
			{ "exclude-callid",          0, 0, 'x'    },
			{ "load-test-consumers",     1, 0, 0xFF01 },
			{ "load-test-producers",     1, 0, 0xFF02 },
			{ NULL, 0, NULL, 0 },
    };

  conf.time_align = 0; 
  conf.format     = FORMAT_CSV_COMMA; // CSV without headers, comma separated
  conf.dedup      = 0;
  conf.time_align = 0;
  conf.start_ts   = 0;

  while ((c = getopt_long (argc, argv, "f:t:s:d:Dx", long_options, NULL)) != -1)
  {
    switch(c)
    {
    	case 0xFF01:
    		conf.load_test_num_cfiles = atoi(optarg);
    		break;
    	case 0xFF02:
    		conf.load_test_num_pfiles = atoi(optarg);
    		break;
      case 'f':
        conf.format = atoi(optarg);
        break;
      case 't':
        conf.time_align = atoi(optarg);
        break;
      case 's':
       	conf.start_ts = strtoull(optarg, NULL, 0);
        break;
      case 'd':
        conf.dur = strtoull(optarg, NULL, 0);
        break;
      case 'D':
        conf.dedup = 1;
        break;
      case 'x':
        conf.no_callid = 1;
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

  if (conf.load_test_num_cfiles)
  {
    load_test_cfiles(&conf);
    return 0;
  }
  
  if (conf.load_test_num_pfiles)
  {
    load_test_pfiles(&conf);
    return 0;
  }

  fd = fopen(conf.filename, "r+");
  if (!fd)
  {
      fprintf(stderr, "failed to open %s. err=%s\n", conf.filename, strerror(errno));
      exit(1);
  }

  rc = format_output(fd, &conf);
  fclose(fd);
  
  return rc;
}
