#define MS_CLASS "DepLibSfuShm"
// #define MS_LOG_DEV

#include "DepLibSfuShm.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"


DepLibSfuShm::SfuShmMapItem::SfuShmMapItem(const char* shm_name) : wrt_ctx(nullptr), wrt_status(SHM_WRT_UNDEFINED) // TODO: add SSRC to params, video and audio
{
  wrt_init.stream_name = const_cast<char *> (shm_name); // TODO: instead declare stream_name a const char*
  
  wrt_init.conf.channels[0].target_buf_ms = 20000; // TODO: actual values, I don't know what should be here
  wrt_init.conf.channels[0].target_kbps   = 128;
  wrt_init.conf.channels[0].ssrc          = 0;
  wrt_init.conf.channels[0].sample_rate   = 48000;
  wrt_init.conf.channels[0].num_chn       = 2;
  wrt_init.conf.channels[0].codec_id      = SFUSHM_AV_AUDIO_CODEC_OPUS;
  wrt_init.conf.channels[0].video         = 0;
  wrt_init.conf.channels[0].audio         = 1;

  wrt_init.conf.channels[1].target_buf_ms = 20000; 
  wrt_init.conf.channels[1].target_kbps   = 2500;
  wrt_init.conf.channels[1].ssrc          = 0;
  wrt_init.conf.channels[1].sample_rate   = 90000;
  wrt_init.conf.channels[1].num_chn       = 0;
  wrt_init.conf.channels[1].codec_id      = SFUSHM_AV_VIDEO_CODEC_H264;
  wrt_init.conf.channels[1].video         = 1;
  wrt_init.conf.channels[1].audio         = 0;

// NGX_SHM_AV_UNSET_PTS is for undefined PTS

  if (SFUSHM_AV_OK != sfushm_av_open_writer( &wrt_init, &wrt_ctx))
    MS_THROW_ERROR("Failed to initialize sfu shm %s", shm_name);
  
  wrt_status = SHM_WRT_INITIALIZED;    
}

DepLibSfuShm::SfuShmMapItem::~SfuShmMapItem()
{
  // Call if writer is not closed
  if (SHM_WRT_CLOSED != wrt_status)
  {
    sfushm_av_close_writer(wrt_ctx, 0); //TODO: smth else at the last param
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

int DepLibSfuShm::WriteChunk(std::string shm, sfushm_av_frame_frag_t* data, DepLibSfuShm::ShmChunkType kind)
{
  int err;
  // basic data checks
  sfushm_av_wr_ctx_t *wr_ctx = nullptr;

  err = DepLibSfuShm::GetShmWriterCtx( shm.c_str(), wr_ctx );
  if(DepLibSfuShm::IsError(err))
  {
    // TODO: log smth
    return -1;
  }

  switch(kind) {
  case DepLibSfuShm::ShmChunkType::VIDEO:
    err = sfushm_av_write_video( wr_ctx, data);
    break;

  case DepLibSfuShm::ShmChunkType::AUDIO:
    err = sfushm_av_write_audio(wr_ctx, data);
    break;

  case DepLibSfuShm::ShmChunkType::RTCP:
    // TODO: this is not implemented in sfushm API yet err = sfushm_av_write_rtcp(wr_ctx, data);
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
  err = 0; // TODO: this is not implemented in sfushm yet sfushm_av_write_stream_metadata(wr_ctx, data, len);
  if(DepLibSfuShm::IsError(err))
  {
    // TODO: log smth
    return -1; // the caller may need to tell the producer of this metadata that writing op failed... or just logging and error would be enough?
  }

  return 0;
}
