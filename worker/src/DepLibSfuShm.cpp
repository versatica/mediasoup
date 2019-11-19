#define MS_CLASS "DepLibSfuShm"
// #define MS_LOG_DEV

#include "DepLibSfuShm.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"


DepLibSfuShm::SfuShmMapItem::SfuShmMapItem(const char* shm_name) : wrt_ctx(nullptr), wrt_status(SHM_WRT_UNDEFINED)
{
  /*
  typedef struct {
	uint32_t          target_buf_ms; // target number of milliseconds to store in shared memory for this channel
	uint32_t          target_kbps;   // expected birtate in kbps
	uint32_t          ssrc;          // source RTP SSRC. Used for correlating with RTCP messages
	uint32_t          sample_rate;   // clock sample rate for the this channel
	uint8_t           num_chn;       // number of audio channels
	uint8_t           codec_id;      // codec id as one of SFUSHM_AV_VIDEO_CODEC_XXX constants
	unsigned          video:1;       // if set the channel contains video
	unsigned          audio:1;       // if set the channel contains audio
} sfushm_av_chn_conf_t;

typedef struct {
	const char             *log_file_name;      // full path for the log file
	unsigned int           log_level;           // default log level. one of SFUSHM_AV_LOG_LEVEL_XXX
	sfushm_av_chn_conf_t   channels[SFUSHM_AV_MAX_NUM_CHANNELS]; // channels configuration
	unsigned               redirect_stdio:1;    // if set log output will be redirected to stdout
} sfushm_av_conf_t;

// we hide the implementation details behind this context from clients of this library
typedef struct{
	void            *wr_ctx;
} sfushm_av_wr_ctx_t;

typedef struct {
	sfushm_av_conf_t    conf;               // context configuration
	char                *stream_name;       // the name of the stream to open for writing
    unsigned int        stats_win_size;     // the writer continuously collects stats about the stream such as fps and bitrate
                                            // it collects it as moving average. this parameter determines the window size in number of samples
} sfushm_av_writer_init_t;
*/
  // Assume that we always have both audio and video in shm
         /*
        class SfuShmMapItem {
    sfushm_av_writer_init_t  init_data;
    sfushm_av_wr_ctx_t      *wrt_ctx;
    ShmWriterStatus          wrt_status;
  };
      */
    //sfushm_av_writer_init_t  init_data;

  // 
  
  wrt_init.stream_name = shm_name; 
  
  wrt_init.conf.channels[0].target_buf_ms = 2000; // TODO: actual values, I don't know what should be here
  wrt_init.conf.channels[0].target_kbps   = 4000;
  wrt_init.conf.channels[0].ssrc          = 0; //TODO: when will I know it?
  wrt_init.conf.channels[0].sample_rate   = 44000;
  wrt_init.conf.channels[0].num_chn       = 2;
  wrt_init.conf.channels[0].codec_id      = SFUSHM_AV_AUDIO_CODEC_OPUS;
  wrt_init.conf.channels[0].video         = 0;
  wrt_init.conf.channels[0].audio         = 1;

  wrt_init.conf.channels[1].target_buf_ms = 2000; 
  wrt_init.conf.channels[1].target_kbps   = 4000;
  wrt_init.conf.channels[1].ssrc          = 0; //TODO: when will I know it?
  wrt_init.conf.channels[1].sample_rate   = 90000;
  wrt_init.conf.channels[1].num_chn       = 0;
  wrt_init.conf.channels[1].codec_id      = SFUSHM_AV_VIDEO_CODEC_H264;
  wrt_init.conf.channels[1].video         = 1;
  wrt_init.conf.channels[1].audio         = 0;

  if (SFUHSM_AV_OK != sfushm_av_open_writer( &wrt_init, &wrt_ctx))
    MS_THROW_ERROR("Failed to initialize sfu shm %s", shm_name);
  
  wrt_status = SHM_WRT_INITIALIZED;    
}

DepLibSfuShm::SfuShmMapItem::~SfuShmMapItem()
{
  // Call if writer is not closed
  if (SHM_WRT_CLOSED != wrt_status)
  {
    sfushm_av_close_writer(>wr_ctx, 0)); //TODO: smth else at the last param
  }
}

std::unordered_map<int, const char*> DepLibSfuShm::errToString =
	{
		{ 0, "success (SFUSHM_AV_OK)"   },
		{ -1, "error (SFUSHM_AV_ERR)"   },
		{ -2, "again (SFUSHM_AV_AGAIN)" }
	};

std::unordered_map<std::string, DepLibSfuShm::SfuShmMapItem*> DepLibSfuShm::shmToWriterCtx;

int DepLibSfuShm::GetShmWriterCtx(const char* shm_name, sfushm_av_wr_ctx_t *ctx_out)
{
  // Either find an initialized writer in the map, or create a new one.
  // Return error in case writter cannot be created.

  MS_TRACE();

  // todo: initialize std::string from const char*
  std::string shm(shm_name);
  // Force lowcase kind.
  Utils::String::ToLowerCase(shm);

  auto it = DepLibSfuShm::shmToWriterCtx.find(shm);

  if (it != DepLibSfuShm::shmToWriterCtx.end()) {
    // Check if wrt_ctx is initialized, may need to reinitialize
    if (SHM_WRT_INITIALIZED == it->second->status()) {
      ctx_out = it->second->wrt_ctx;
      return 0; // done
    }
    else {
      // remove and then reinit
      it = DepLibSfuShm::shmToWriterCtx.erase(it);
    }
  }

  // create new writer ctx for Media::Kind::ALL, assume we always have both video and audio
  auto i = new DepLibSfuShm::SfuShmMapItem(shm.c_str()); 
  DepLibSfuShm::shmToWriterCtx[shm] = i;

  ctx_out = DepLibSfuShm::shmToWriterCtx[shm]->wrt_ctx;

  return 0;
}

int DepLibSfuShm::WriteChunk(const char* shm, sfushm_av_frame_frag_t* data, ShmChunkType kind)
{
  int err;
  // basic data checks
  sfushm_av_wr_ctx_t *wr_ctx = nullptr;

  err = DepLibSfuShm::GetShmWriterCtx( shm, wr_ctx );
  if(DepLibSfuShm::IsError(err))
  {
    // TODO: log smth
    return -1;
  }

  switch(kind) {
  case SHM_VIDEO:
    err = sfushm_av_write_video( wr_ctx, data);
    break;

  case SHM_AUDIO:
    err = sfushm_av_write_audio(wr_ctx, data);
    break;

  case SHM_RTCP:
    err = sfushm_av_write_rtcp(wr_ctx, data);
    break;

  default:
    //TODO: LOG SMTH
    return -1;
  }

  if(DepLibSfuShm::IsError(err))
  {
    // TODO: log smth
    return -1; // depending on err might stop writing all together, or ignore this particular packet (and do smth specific in ShmTransport.cpp)
  }
  return 0;
}


int DepLibSfuShm::WriteStreamMetadata(const char* shm, uint8_t *data, size_t len)
{
  int err;
  // basic data checks
  sfushm_av_wr_ctx_t *wr_ctx = nullptr;

  err = DepLibSfuShm::GetShmWriterCtx( shm, wr_ctx );
  if(DepLibSfuShm::IsError(err))
  {
    // TODO: log smth
    return -1;
  }

  // write opaque data to shared memory. this allows an external controller to set stream meta-data such as room state
  // in the shared memory.
  err = sfushm_av_write_stream_metadata(wr_ctx, data, len);
  if(DepLibSfuShm::IsError(err))
  {
    // TODO: log smth
    return -1; // the caller may need to tell the producer of this metadata that writing op failed... or just logging and error would be enough?
  }
}
