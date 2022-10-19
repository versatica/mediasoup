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
//#include <regex.h>

// These defines should match LivelyBinLogs.hpp values
#define BINLOG_FORMAT_VERSION "0242ac"

#define CALL_STATS_BIN_LOG_RECORDS_NUM 8 
#define CALL_STATS_BIN_LOG_SAMPLING    2000

#define MAX_TIME_ALIGN        10
#define MAX_RECORDS_IN_BUFFER 1000

#define FORMAT_CSV_HEADERS 2

#define LOAD_TEST_MAX_FILES    200
#define LOAD_TEST_FILE_MAX_DUR 1800000
#define LOAD_TEST_FILE_MIN_DUR 2000
#define LOAD_TEST_MIN_SLEEP    1950

#define UUID_CHAR_LEN 36
#define ZEROS_UUID    "00000000-0000-0000-0000-000000000000\0"
#define UUID_BYTE_LEN 16


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
} stats_sample_t;

#define SAMPLE_SIZE 28

typedef struct {
  uint64_t       start_tm;                    // the record start timestamp in milliseconds
  uint32_t       filled;                      // number of filled records in the array below
  uint16_t       ssrc;                        // ssrc as in original RTP stream
  uint8_t        payload;                     // payload id as in original RTP stream
  uint8_t        content;                     // 'a' or 'v'
  char           consumer_id [UUID_BYTE_LEN]; // 
  char           producer_id [UUID_BYTE_LEN]; // 
} stats_consumer_record_header_t;

typedef struct {
  uint64_t       start_tm;                    // the record start timestamp in milliseconds
  uint32_t       filled;                      // number of filled records in the array below
  uint16_t       ssrc;                        // ssrc as in original RTP stream
  uint8_t        payload;                     // payload id as in original RTP stream
  uint8_t        content;                     // 'a' or 'v'
} stats_producer_record_header_t;

// Bytes to skip over and start reading stats_sample_t
#define CONSUMER_HEADER_LEN 48
// Total size of a record: header and samples
#define CONSUMER_RECORD_LEN (CONSUMER_HEADER_LEN + SAMPLE_SIZE * CALL_STATS_BIN_LOG_RECORDS_NUM)

#define PRODUCER_HEADER_LEN 16
#define PRODUCER_RECORD_LEN (PRODUCER_HEADER_LEN + SAMPLE_SIZE * CALL_STATS_BIN_LOG_RECORDS_NUM)


static const char *header[][3] = {
  {"call_id                             ", "Call ID",     "ID of a call"},
  {"object_id                           ", "Object ID",   "ID of a producer or consumer object"},
  {"producer_id                         ", "Producer ID", "ID of a consumer's producer or empty field"}, 
  {"start_ts",  "Start Time",            "The statime of the epoch (HH:MM:SS,sss)"},
  {"type",     "Stream Type",           "0 - producer, 1 - consumer"},
  {"ssrc",  "Ssrc",                    "16 bits of SSRC as in original RTP stream"},
  {"pay",  "Payload Id",                  "Payload Id as in original RTP stream"},
  {"content",  "Content",               "Content type: audio or video"},
  {"pkt ",  "Packets Count",         "Packets received or sent during epoch"},
  {"lost ", "Packets Lost",          "Packets lost during epoch"},
  {"disc ", "Packets Discarded",     "Packets discarded during epoch"},
  {"rtx ",  "Packets Retransmitted", "Packets Retransmitted during epoch"},
  {"repair",  "Packets Repaired",      "Packets repaired during epoch"},
  {"nacks",  "NACK Count",            "NACKs during epoch"},
  {"nackpkt",  "NACK Packets",          "Number of NACK packets requested"},
  {"kf",        "Keyframe Requests",     "Keyframe requests during epoch"},
  {"rtt",       "RTT",                   "RTT in milliseconds"},
  {"max_pts  ",   "Maximum PTS",           "The most recent PTS in a stream"},
  {"bytes", "Bytes Count",           "Bytes received or sent during epoch"},
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
  char        call_id[UUID_CHAR_LEN + 1]; // call id as parsed from file
  char        producer_id[UUID_CHAR_LEN + 1]; // producer id
  char        type; // 'c' or 'p'

} ms_binlog_config;

static char hx[] = "0123456789abcdef";

uint8_t* 
fill_with_rand_hex(uint8_t *p, int cnt)
{
  int i, r;

  for (i = 0; i < cnt; i++)
  {
    r = rand() % 16;
    *p++ = hx[r];
  }
  return p;
}

// from is 16 bytes, to is 37 bytes
void uuidBytesToHexStr(const uint8_t* from, char* to)
{
  strcpy(to, ZEROS_UUID); 
  char* p = to;
  int i = 0;
  while(p && i < 16)
  {
    if(*p == '-')
      p++;
    // convert a byte into 2 chars
    *p++ = hx[(from[i] & 0xF0) >> 4];
    *p++ = hx[from[i] & 0x0F];
    i++;
  }
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

// Result is random 16 bytes not starting with zeros
void
rand_pseudo_uuid_bytes(uint8_t *dst)
{
  uint8_t *ptr = dst;
  int i;
  memset(dst, 0, UUID_BYTE_LEN);
  *ptr++ = 1 + rand() % 254;
  for (i = 1; i < UUID_BYTE_LEN; i++)
  {
    *ptr++ = rand() % 255;
  }
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


// even - audio, odd - video
#define SET_RAND_VIDEO_RECORD(f, s, r) rec.samples[j].f = (i % 2) ? s + rand() % r : 0;
#define SET_RAND_RECORD(f, s, r) rec.samples[j].f = s + rand() % r;

// Creates a single test file with data for several consumers
int
load_test_cfiles(ms_binlog_config *conf)
{
  /*
  BUGBUG: rewrite this function to accomodate for changes in data sizes, etc.
  int                 fd;
  char                filename[256];

	uint64_t            start_ts[LOAD_TEST_MAX_FILES];
  uint64_t            last_ts[LOAD_TEST_MAX_FILES];
  uint64_t            end_ts[LOAD_TEST_MAX_FILES];
	uint64_t            active[LOAD_TEST_MAX_FILES];
  uint8_t             payload[LOAD_TEST_MAX_FILES];
  uint64_t            tmp;
  uint16_t            active_consumers_cnt;

  uint8_t             object_id[LOAD_TEST_MAX_FILES][UUID_BYTE_LEN];
  uint8_t             producer_id[LOAD_TEST_MAX_FILES][UUID_BYTE_LEN];
  
  int                 i, j;
  int                 count;
  int                 source;

  stats_consumer_record_t rec;
  stats_sample_t sample;
  stats_sample_t prev_sample[LOAD_TEST_MAX_FILES];

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
    // TODO: payload is rand uint8_t unless there are some restrictions
    payload[i] = rand() % 255;
    active[i] = 1;
    rand_pseudo_uuid_bytes(object_id[i]);
    rand_pseudo_uuid_bytes(producer_id[i]);
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

      memcpy(rec.consumer_id, object_id[i], UUID_BYTE_LEN);
      memcpy(rec.producer_id, producer_id[i], UUID_BYTE_LEN);
      rec.payload = payload[i];
      rec.filled = CALL_STATS_BIN_LOG_RECORDS_NUM;

      prev_sample[i].max_pts = cur_tm + 500;
      prev_sample[i].epoch_len = CALL_STATS_BIN_LOG_SAMPLING;

      for (j = 0; j < CALL_STATS_BIN_LOG_RECORDS_NUM; j++)
      {
        memset(&(rec.samples[j]), 0, sizeof(stats_sample_t));
        
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

  close(fd);*/
  return 0;
}


int
load_test_pfiles(ms_binlog_config *conf)
{
  /*
  char                filename[LOAD_TEST_MAX_FILES][256];
  int                 fd[LOAD_TEST_MAX_FILES]; // set into -1 explicitly after file is closed, not to be confused with error state
  uint64_t            last_ts[LOAD_TEST_MAX_FILES];
	uint64_t            end_ts[LOAD_TEST_MAX_FILES];
  uint8_t             payload[LOAD_TEST_MAX_FILES];
  int                 fopened_count;
  
  char                object_id[LOAD_TEST_MAX_FILES][UUID_CHAR_LEN + 1];
  int                 source, mime;
  int                 i, j, count;

  stats_producer_record_t rec;
  stats_sample_t sample;
  stats_sample_t prev_sample[LOAD_TEST_MAX_FILES];

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
    payload[i] = rand() % 255;
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

      rec.payload = payload[i];
      rec.filled = CALL_STATS_BIN_LOG_RECORDS_NUM;

      prev_sample[i].max_pts = cur_tm + 1000;
      prev_sample[i].epoch_len = CALL_STATS_BIN_LOG_SAMPLING;

      for (j = 0; j < CALL_STATS_BIN_LOG_RECORDS_NUM; j++)
      {
        memset(&(rec.samples[j]), 0, sizeof(stats_sample_t));

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
*/
  return 0;
}


int
time_alignment(
    uint64_t record_tm,
    uint16_t            sample_dur,
    uint16_t            time_align,
		stats_sample_t *sample_in,
    uint64_t            *sample_ts,
		stats_sample_t *sample_out,
		int out_len)
{
	stats_sample_t sample;
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
		sample_out[i].epoch_len = time_align; //time_align * i;
    sample_ts[i] = start_tm + time_align * i; //sample_out[i].epoch_len;
	}

	return i;
}

// should run consumer's or producer's parsing code
//ms_p_00000000-0000-0000-0000-000000000000_00000000-0000-0000-0000-000000000000_1652210519459.123abc.bin.log
//ms_c_00000000-0000-0000-0000-000000000000_1652210519459.123abc.bin.log
int parse_file_name(ms_binlog_config *conf)
{
  #define FILENAME_LEN_MAX sizeof("/var/log/sfu/ms_p_00000000-0000-0000-0000-000000000000_00000000-0000-0000-0000-000000000000_1652210519459.123abc.bin") * 2
  char tmp[FILENAME_LEN_MAX];
  char* ch;
  char* ver;

  memset(tmp, '\0', FILENAME_LEN_MAX);
  memcpy(tmp, conf->filename, strlen(conf->filename));

  memset(conf->call_id, 0, UUID_CHAR_LEN + 1);
  memset(conf->producer_id, 0, UUID_CHAR_LEN + 1);

  ch = strtok(tmp, "_");

  if(ch == 0 || strcmp(ch, "ms") != 0)
  {
    printf("wrong file name %s, should start with 'ms_', exit...", conf->filename);
    return 1;
  }

  ch = strtok(NULL,"_");

  // c or p? get callid and optionally producer's id
  if (ch && strlen(ch) == 1 && ch[0] == 'c')
  {
    conf->type = 'c';
  }
  else if (ch && strlen(ch) == 1 && ch[0] == 'p')
  {
    conf->type = 'p';
  }
  else
  {
    printf("wrong file name %s, can't tell type 'c' or 'p', exit...", conf->filename);
    return 1;
  }

  ch = strtok(NULL,"_");
  if (ch == 0 || strlen(ch) < UUID_CHAR_LEN)
  {
    printf("wrong file name %s, should contain uuid, exit...", conf->filename);
    return 1;
  }
  memcpy(conf->call_id, ch, UUID_CHAR_LEN);

  if (conf->type == 'p')
  {
    ch = strtok(NULL, "_");
    if (ch == 0 || strlen(ch) < UUID_CHAR_LEN)
    {
      printf("wrong file name %s, should contain two uuids, exit...", conf->filename);
      return 1;
    }

    memcpy(conf->producer_id, ch, UUID_CHAR_LEN);
  }
  // now left with timestamp.version.bin let's match versions
  memset(tmp, '\0', FILENAME_LEN_MAX);
  memcpy(tmp, conf->filename,strlen(conf->filename));
  ver = strtok(tmp, "."); // skip over timestamp
  if (!ver)
  {
    printf("wrong file name %s, should end with timestamp.version.bin, exit...", conf->filename);
    return 1;
  }

  ver = strtok(NULL, ".");
  if (!ver || strcmp(ver, BINLOG_FORMAT_VERSION) != 0)
  {
    printf("wrong file name %s, should end with timestamp.%s.bin, exit...", conf->filename,  BINLOG_FORMAT_VERSION);
    return 1;
  }

  return 0;
}

// parametrized: header_length, structure header size

int
format_output(FILE* fd, ms_binlog_config *conf)
{
  char                        buf_c[CONSUMER_RECORD_LEN * MAX_RECORDS_IN_BUFFER];
  char                        buf_p[PRODUCER_RECORD_LEN * MAX_RECORDS_IN_BUFFER];
  size_t                      len;  // in bytes
  char                        *first_c;
  char                        *first_p;

  int                            m, k, i, num_bytes, num_rec, num_tm_align_rec;
  stats_consumer_record_header_t *rec_c;
  stats_producer_record_header_t *rec_p;
  uint8_t                        filled;
  uint64_t                       rec_start_tm;
  stats_sample_t                 *sample;
  char                           *samples_pos;
  uint16_t                       ssrc;
  uint8_t                        payload;
  char                           content;

  stats_sample_t              sample_align[MAX_TIME_ALIGN];
  uint64_t                    sample_ts[MAX_TIME_ALIGN];
  uint64_t                    total_epoch_len_in_samples;
  uint64_t                    sample_start_tm;
  
  char                        call_id[UUID_CHAR_LEN+1];
  char                        object_id[UUID_CHAR_LEN+1]; 
  char                        producer_id[UUID_CHAR_LEN+1]; 

  uint64_t                    start_ts, end_ts;

  first_c = &buf_c[0];
  first_p = &buf_p[0];

  start_ts = conf->start_ts;
  num_tm_align_rec = 1;
  
  memset(call_id, sizeof(call_id), 0);
  memcpy(call_id, conf->call_id, UUID_CHAR_LEN);

  memset(object_id, sizeof(object_id), 0); 
  memset(producer_id, sizeof(producer_id), 0);

  if (conf->type == 'p')
  {
    memcpy(object_id, conf->producer_id, UUID_CHAR_LEN);
    memcpy(producer_id, conf->producer_id, UUID_CHAR_LEN);
  }

  len = (conf->type == 'c') ? CONSUMER_RECORD_LEN * MAX_RECORDS_IN_BUFFER : PRODUCER_RECORD_LEN * MAX_RECORDS_IN_BUFFER;

  if (FORMAT_CSV_HEADERS == conf->format)
    print_headers(conf);

 
  while( (num_bytes = (conf->type == 'c') 
                      ? fread(buf_c, sizeof(uint8_t), len, fd)
                      : fread(buf_p, sizeof(uint8_t), len, fd)) > 0 )
  {
    num_rec = (conf->type == 'c') ? num_bytes/CONSUMER_RECORD_LEN : num_bytes/PRODUCER_RECORD_LEN;
    for (m = 0; m < num_rec; m++)
    {
      if (conf->type == 'c')
      {
        rec_c = (stats_consumer_record_header_t*)first_c;
        filled = rec_c->filled;
        rec_start_tm = rec_c->start_tm;
        //printf("\nRecord payload=%d ssrc=%"PRIu16" content=%c filled=%d\n", rec_c->payload, rec_c->ssrc, rec_c->content, filled);
      }
      else
      {
        rec_p = (stats_producer_record_header_t*)first_p;
        filled = rec_p->filled;
        rec_start_tm = rec_p->start_tm;
        //printf("\nRecord payload=%d ssrc=%"PRIu16" content=%c filled=%d\n", rec_p->payload, rec_p->ssrc, rec_p->content, filled);
      }
      if (filled > CALL_STATS_BIN_LOG_RECORDS_NUM)
        continue;

      if (conf->type == 'c')
      {
        uuidBytesToHexStr(rec_c->consumer_id, object_id);
        uuidBytesToHexStr(rec_c->producer_id, producer_id);
      }

      samples_pos = (conf->type == 'c') ? ((char*)rec_c) + CONSUMER_HEADER_LEN : ((char*)rec_p) + PRODUCER_HEADER_LEN;
      ssrc = (conf->type == 'c') ? rec_c->ssrc : rec_p->ssrc;
      payload = (conf->type == 'c') ? rec_c->payload : rec_p->payload;
      content = (conf->type == 'c') ? rec_c->content : rec_p->content;

      total_epoch_len_in_samples = 0;

      for (k = 0; k < filled; k++)
      {
        sample = (stats_sample_t*)(samples_pos + k * SAMPLE_SIZE);

        total_epoch_len_in_samples += sample->epoch_len;

        if (conf->time_align)
        {
          sample_start_tm = rec_start_tm + total_epoch_len_in_samples;

          // just round it up or down to time_align:
          sample_start_tm = (sample_start_tm % conf->time_align) ?
              (sample_start_tm / conf->time_align + (sample_start_tm % conf->time_align + conf->time_align / 2) / conf->time_align) * conf->time_align :
              sample_start_tm;

          num_tm_align_rec = time_alignment(sample_start_tm, sample->epoch_len,  conf->time_align, sample, sample_ts, sample_align, MAX_TIME_ALIGN);
          sample = sample_align;
        }
        else
        {
          sample_ts[0] = rec_start_tm + total_epoch_len_in_samples;
        }

        for (i = 0; i < num_tm_align_rec; i++)
        {
          // in case remove duplicates is on and this record's timestamp
          // is older than the most recent record then skip it
          if (conf->dedup)
          {
            // TODO: this does not work for consumer bin logs b/c a single log contains records from multiple sources.
            // Need to keep track of the last ts for each payload#
            if (sample_ts[i] <= conf->dedup_last_ts)
            {
              continue;
            }
            else {
              conf->dedup_last_ts = sample_ts[i];
            }
          }

          if (!start_ts)
            start_ts = sample_ts[i];

          end_ts = conf->dur? (start_ts + conf->dur) : (uint64_t)-1;
          if (sample_ts[i] < start_ts || sample_ts[i] > end_ts)
            continue;

          fprintf(stdout, 
            "%s\t%s\t%s"
            "\t%"PRIu64
            "\t%c\t%"PRIu16"\t%"PRIu8"\t%c\t%"PRIu16
            "\t%"PRIu16"\t%"PRIu16"\t%"PRIu16
            "\t%"PRIu16"\t%"PRIu16"\t%"PRIu16
            "\t%"PRIu16"\t%"PRIu16"\t%"PRIu32"\t%"PRIu32"\n",
            call_id, object_id, producer_id,
            sample_ts[i],
            conf->type, ssrc, payload, content, sample[i].packets_count,
            sample[i].packets_lost, sample[i].packets_discarded, sample[i].packets_retransmitted,
            sample[i].packets_repaired, sample[i].nack_count, sample[i].nack_pkt_count,
            sample[i].kf_count, sample[i].rtt, sample[i].max_pts, sample[i].bytes_count);
        }
      }
      if(conf->type == 'c') // TODO: see that ptr does not go over a read buffer
      {
        first_c += CONSUMER_RECORD_LEN;
      }
      else
      {
        first_p += PRODUCER_RECORD_LEN;
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
      "  -t <interval>             - optional align timestamps to fixed intervals in milliseconds\n"
      "  -s <timestamp>            - optional start timestamp\n"
      "  -d <duration>             - duration in milliseconds (if only dur is specified it is taken from the start of the stream)\n"
      "  -D                        - optional remove duplicate records\n"
      "  -x                        - optional exclude callid column from output if in format 1 or 2\n"
      "  --load-test-consumers <n> - load test simulation, create stats file with n consumers\n"
      "  --load-test-producers <n> - load test simulation, create n producers stats files\n"
      ;

  static struct option long_options[] = {
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
  conf.format     = FORMAT_CSV_HEADERS; // CSV without headers, comma separated
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
  if ((rc = parse_file_name(&conf)) != 0)
    return rc;

  rc = format_output(fd, &conf);
  fclose(fd);
  
  return rc;
}
