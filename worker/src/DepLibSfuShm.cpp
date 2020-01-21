#define MS_CLASS "DepLibSfuShm"
// #define MS_LOG_DEV

#include "DepLibSfuShm.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"


DepLibSfuShm::SfuShmMapItem::~SfuShmMapItem()
{
  // Call if writer is not closed
  if (SHM_WRT_CLOSED != wrt_status)
  {
    sfushm_av_close_writer(wrt_ctx, 0); //TODO: smth else at the last param
  }
}


DepLibSfuShm::ShmWriterStatus DepLibSfuShm::SfuShmMapItem::SetSsrcInShmConf(uint32_t ssrc, DepLibSfuShm::ShmChunkType kind)
{
  //Assuming that ssrc does not change, shm writer is initialized, nothing else to do
  if (SHM_WRT_READY == this->Status())
    return SHM_WRT_READY;
  
  switch(kind) {
    case DepLibSfuShm::ShmChunkType::AUDIO:
      //if (this->AudioSsrc() == ssrc) {
      //  return false;
      //}
      this->wrt_init.conf.channels[0].audio = 1;
      this->wrt_init.conf.channels[0].ssrc = ssrc;
      this->wrt_status = (this->wrt_status == SHM_WRT_AUDIO_CHNL_CONF_MISSING) ? SHM_WRT_READY : SHM_WRT_VIDEO_CHNL_CONF_MISSING;

      break;  
    case DepLibSfuShm::ShmChunkType::VIDEO:
      //if (this->VideoSsrc() == ssrc) {
      //  return false;
      //}
      this->wrt_init.conf.channels[1].video = 1;
      this->wrt_init.conf.channels[1].ssrc = ssrc;
      this->wrt_status = (this->wrt_status == SHM_WRT_VIDEO_CHNL_CONF_MISSING) ? SHM_WRT_READY : SHM_WRT_AUDIO_CHNL_CONF_MISSING;

      break;
    default:
      // ignore RTCP and rest
      return this->Status();
  }

  if (this->wrt_status == SHM_WRT_READY) {
    MS_DEBUG_TAG(rtp, "DepLibSfuShm::SfuShmMapItem::SetSsrcInShmConf() will call sfushm_av_open_writer()");
    int err = SFUSHM_AV_OK;
    if ((err = sfushm_av_open_writer( &wrt_init, &wrt_ctx)) != SFUSHM_AV_OK) {
      MS_DEBUG_TAG(rtp, "FAILED in sfushm_av_open_writer() to initialize sfu shm %s with error %s", this->wrt_init.stream_name, DepLibSfuShm::GetErrorString(err));
      wrt_status = SHM_WRT_UNDEFINED;
    }
    return this->Status();
  }

  //MS_DEBUG_TAG(rtp, "DepLibSfuShm::SfuShmMapItem::SetSsrcInShmConf() did not initialize shm writer yet this->wrt_status %u ", this->wrt_status);
  return this->Status(); // if shm was not initialized as a result, return false
}


std::unordered_map<int, const char*> DepLibSfuShm::errToString =
	{
		{ 0, "success (SFUSHM_AV_OK)"   },
		{ -1, "error (SFUSHM_AV_ERR)"   },
		{ -2, "again (SFUSHM_AV_AGAIN)" }
	};


//std::unordered_map<const char*, DepLibSfuShm::SfuShmMapItem*> DepLibSfuShm::shmToWriterCtx;


void DepLibSfuShm::InitializeShmWriterCtx(std::string shm_name, std::string log_name, int log_level, DepLibSfuShm::SfuShmMapItem *shmCtx)
{
  MS_TRACE();

  shmCtx->stream_name.assign(shm_name);

  shmCtx->wrt_init.stream_name = "SFUTEST"; //shmCtx->stream_name.c_str()
	shmCtx->wrt_init.stats_win_size = 300;
	shmCtx->wrt_init.conf.log_file_name = "/var/log/sg/nginx/test_sfu_shm.log"; // log_name
	shmCtx->wrt_init.conf.log_level = 9; // log_level
	shmCtx->wrt_init.conf.redirect_stdio = 1;
  
  // TODO: initialize with some different values? or keep these?
  shmCtx->wrt_init.conf.channels[0].target_buf_ms = 20000;
  shmCtx->wrt_init.conf.channels[0].target_kbps   = 128;
  shmCtx-> wrt_init.conf.channels[0].ssrc          = 0;
  shmCtx->wrt_init.conf.channels[0].sample_rate   = 48000;
  shmCtx->wrt_init.conf.channels[0].num_chn       = 2;
  shmCtx->wrt_init.conf.channels[0].codec_id      = SFUSHM_AV_AUDIO_CODEC_OPUS;
  shmCtx->wrt_init.conf.channels[0].video         = 0; 
  shmCtx->wrt_init.conf.channels[0].audio         = 0; // will switch to 1 when actual ssrc arrives

  shmCtx->wrt_init.conf.channels[1].target_buf_ms = 20000; 
  shmCtx->wrt_init.conf.channels[1].target_kbps   = 2500;
  shmCtx->wrt_init.conf.channels[1].ssrc          = 0;
  shmCtx->wrt_init.conf.channels[1].sample_rate   = 90000;
  shmCtx->wrt_init.conf.channels[1].codec_id      = SFUSHM_AV_VIDEO_CODEC_H264;
  shmCtx->wrt_init.conf.channels[1].video         = 0;  // will switch to 1 when actual ssrc arrives
  shmCtx->wrt_init.conf.channels[1].audio         = 0;

   // shmCtx = new DepLibSfuShm::SfuShmMapItem(shm_name, log_name, log_level); 
    // TODO: I don't need a map
    //DepLibSfuShm::shmToWriterCtx[shm_name] = i;
     //DepLibSfuShm::shmToWriterCtx[shm_name];
  MS_DEBUG_TAG(rtp, "DepLibSfuShm::SfuShmMapItem created with name %s", shmCtx->stream_name.c_str());
}


DepLibSfuShm::ShmWriterStatus DepLibSfuShm::ConfigureShmWriterCtx(DepLibSfuShm::SfuShmMapItem *shmCtx, DepLibSfuShm::ShmChunkType kind, uint32_t ssrc)
{
  MS_TRACE();
  
  if (SHM_WRT_READY == shmCtx->Status()) {
    return SHM_WRT_READY;  // fully initialized, just return
  }

  // Here ctx is allocated but not fully initialized, try to init
  if (SHM_WRT_READY == shmCtx->SetSsrcInShmConf(ssrc, kind)) {
    	MS_DEBUG_TAG(rtp, "DepLibSfuShm::ConfigureShmWriterCtx() SHM_WRT_READY");
  }

  return shmCtx->Status();
}


int DepLibSfuShm::WriteChunk(DepLibSfuShm::SfuShmMapItem *shmCtx, sfushm_av_frame_frag_t* data, DepLibSfuShm::ShmChunkType kind, uint32_t ssrc)
{
  int err;
  sfushm_av_wr_ctx_t *wr_ctx = nullptr;

  if (shmCtx->Status() != SHM_WRT_READY)
  {
    if (SHM_WRT_READY != DepLibSfuShm::ConfigureShmWriterCtx(shmCtx, kind, ssrc))
    {
      return 0;
    }
  }

  switch (kind)
  {
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


int DepLibSfuShm::WriteStreamMetadata(DepLibSfuShm::SfuShmMapItem *shmCtx, uint8_t *data, size_t len)
{
  // TODO: there should be shm writing API for this

  if (shmCtx == nullptr || shmCtx->Status() != SHM_WRT_READY)
  {
    // TODO: log smth
    return -1;
  }

  return 0;
}
