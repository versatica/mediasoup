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
  wrt_init.conf.channels[0].audio         = 0; // will switch to 1 when actual ssrc arrives

  wrt_init.conf.channels[1].target_buf_ms = 20000; 
  wrt_init.conf.channels[1].target_kbps   = 2500;
  wrt_init.conf.channels[1].ssrc          = 0;
  wrt_init.conf.channels[1].sample_rate   = 90000;
  wrt_init.conf.channels[1].num_chn       = 0;
  wrt_init.conf.channels[1].codec_id      = SFUSHM_AV_VIDEO_CODEC_H264;
  wrt_init.conf.channels[1].video         = 0;  // will switch to 1 when actual ssrc arrives
  wrt_init.conf.channels[1].audio         = 0;

// NGX_SHM_AV_UNSET_PTS is for undefined PTS
}

DepLibSfuShm::SfuShmMapItem::~SfuShmMapItem()
{
  // Call if writer is not closed
  if (SHM_WRT_CLOSED != wrt_status)
  {
    sfushm_av_close_writer(wrt_ctx, 0); //TODO: smth else at the last param
  }
}

bool DepLibSfuShm::SfuShmMapItem::SetSsrcInShmConf(uint32_t ssrc, DepLibSfuShm::ShmChunkType kind)
{
  if (wrt_ctx == nullptr)
    // log smth?
    return false;

  switch(kind) {
    case DepLibSfuShm::ShmChunkType::AUDIO:
      if (this->AudioSsrc() == ssrc) {
        return false; // no action possible
      }
      this->wrt_init.conf.channels[0].audio = 1;
      this->wrt_init.conf.channels[0].ssrc = ssrc;
      this->wrt_status = (this->Status() == SHM_WRT_AUDIO_CHNL_CONF_MISSING) ? SHM_WRT_READY : SHM_WRT_VIDEO_CHNL_CONF_MISSING;

      break;  
    case DepLibSfuShm::ShmChunkType::VIDEO:
      if (this->VideoSsrc() == ssrc) {
        return false;
      }
      this->wrt_init.conf.channels[1].video = 1;
      this->wrt_init.conf.channels[1].ssrc = ssrc;
      this->wrt_status = (this->Status() == SHM_WRT_VIDEO_CHNL_CONF_MISSING) ? SHM_WRT_READY : SHM_WRT_AUDIO_CHNL_CONF_MISSING;

      break;
    default:
      // ignore RTCP and rest
      MS_DEBUG_TAG(rtp, "DepLibSfuShm::SfuShmMapItem::SetSsrcInShmConf() is noop, kind is not set");
      return false;
  }

  if (this->wrt_status == SHM_WRT_READY) {
    MS_DEBUG_TAG(rtp, "DepLibSfuShm::SfuShmMapItem::SetSsrcInShmConf() will call sfushm_av_open_writer()");
    if (SFUSHM_AV_OK != sfushm_av_open_writer( &wrt_init, &wrt_ctx)) {
      MS_DEBUG_TAG(rtp, "FAILED to initialize sfu shm %s", this->wrt_init.stream_name);
      wrt_status = SHM_WRT_UNDEFINED;
      return false;
    }
    MS_DEBUG_TAG(rtp, "DepLibSfuShm::SfuShmMapItem::SetSsrcInShmConf() called sfushm_av_open_writer()");
    return true;
  }

  MS_DEBUG_TAG(rtp, "DepLibSfuShm::SfuShmMapItem::SetSsrcInShmConf() did not initialize shm writer yet");
  return false; // if shm was not initialized as a result, return false
}

std::unordered_map<int, const char*> DepLibSfuShm::errToString =
	{
		{ 0, "success (SFUSHM_AV_OK)"   },
		{ -1, "error (SFUSHM_AV_ERR)"   },
		{ -2, "again (SFUSHM_AV_AGAIN)" }
	};

std::unordered_map<const char*, DepLibSfuShm::SfuShmMapItem*> DepLibSfuShm::shmToWriterCtx;

bool DepLibSfuShm::IsConnected(const char* shm)
{
  MS_TRACE();

  auto it = DepLibSfuShm::shmToWriterCtx.find(shm);

  if (it != DepLibSfuShm::shmToWriterCtx.end()) {
    return (SHM_WRT_READY == it->second->Status());
  }
  else {
    return false;
  }
}

DepLibSfuShm::SfuShmMapItem* DepLibSfuShm::GetShmWriterCtx(const char* shm_name)
{
  MS_TRACE();
  //Utils::String::ToLowerCase(shm);
  auto it = DepLibSfuShm::shmToWriterCtx.find(shm_name);

  if (it != DepLibSfuShm::shmToWriterCtx.end()) {
    if (SHM_WRT_READY == it->second->Status()) {
      return it->second;
    }
  }
  return nullptr;
}

int DepLibSfuShm::ConfigureShmWriterCtx(const char* shm_name, DepLibSfuShm::SfuShmMapItem **shmCtx, DepLibSfuShm::ShmChunkType kind, uint32_t ssrc)
{
  // Either find an initialized writer in the map, or create a new one.
  // Return error in case writter cannot be created.
  MS_TRACE();

  if (shmCtx == nullptr) {
    auto it = DepLibSfuShm::shmToWriterCtx.find(shm_name); // look up by name, create one if needed
    if (it == DepLibSfuShm::shmToWriterCtx.end()) {
      auto i = new DepLibSfuShm::SfuShmMapItem(shm_name); 
      DepLibSfuShm::shmToWriterCtx[shm_name] = i;
      *shmCtx = DepLibSfuShm::shmToWriterCtx[shm_name];
      MS_DEBUG_TAG(rtp, "New DepLibSfuShm::SfuShmMapItem is created");
    }
    else {
      *shmCtx = it->second;
      MS_DEBUG_TAG(rtp, "Existing DepLibSfuShm::SfuShmMapItem is found");
      if (SHM_WRT_READY == it->second->Status()) {
        MS_DEBUG_TAG(rtp, "Existing DepLibSfuShm::SfuShmMapItem is found at SHM_WRT_READY");
        return 0;  // fully initialized, just return
      }
    }
  }
  // Here ctx is allocated but not fully initialized, try to init
  MS_ASSERT((*shmCtx)->Status() != SHM_WRT_READY, "Error: status cannot be SHM_WRT_READY");
  if ((*shmCtx)->SetSsrcInShmConf(ssrc, kind)) {
    	MS_DEBUG_TAG(rtp, "-=-=-=-=-=-=-=-=-=-=-=-=- SHM_WRT_READY!");
  }

  return 0;
}

int DepLibSfuShm::WriteChunk(const char* shm_name, DepLibSfuShm::SfuShmMapItem *shmCtx, sfushm_av_frame_frag_t* data, DepLibSfuShm::ShmChunkType kind, uint32_t ssrc)
{
  int err;
  sfushm_av_wr_ctx_t *wr_ctx = nullptr;

  if (shmCtx->Status() != SHM_WRT_READY)
  {
    err = DepLibSfuShm::ConfigureShmWriterCtx(shm_name, &shmCtx, kind, ssrc);
    if (DepLibSfuShm::IsError(err))
    {
      // TODO: log smth
      return -1;
    }

    if (shmCtx->Status() != SHM_WRT_READY)
    {
      // log that pkt is going to be ignored because cannot initialized the shm yet, and return
      return 0;
    }
  }

  switch (kind) {
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

  if (DepLibSfuShm::IsError(err))
  {
    // TODO: log smth
    return -1; // depending on err might stop writing all together, or ignore this particular packet (and do smth specific in ShmTransport.cpp)
  }

  return 0;
}


int DepLibSfuShm::WriteStreamMetadata(const char* shm_name, DepLibSfuShm::SfuShmMapItem *shmCtx, uint8_t *data, size_t len)
{
  // TODO: there should be shm writing API for this

 // DepLibSfuShm::SfuShmMapItem* shmCtx = GetShmWriterCtx(const char* shm_name);
  if (shmCtx == nullptr || shmCtx->Status() != SHM_WRT_READY)
  {
    // TODO: log smth
    return -1;
  }

  return 0;
}
