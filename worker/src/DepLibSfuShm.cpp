#define MS_CLASS "DepLibSfuShm"
// #define MS_LOG_DEV

#include "DepLibSfuShm.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"


std::unordered_map<int, const char*> DepLibSfuShm::SfuShmCtx::errToString =
	{
		{ 0, "success (SFUSHM_AV_OK)"   },
		{ -1, "error (SFUSHM_AV_ERR)"   },
		{ -2, "again (SFUSHM_AV_AGAIN)" }
	};


DepLibSfuShm::SfuShmCtx::~SfuShmCtx()
{
  // Call if writer is not closed
  if (SHM_WRT_CLOSED != wrt_status)
  {
    sfushm_av_close_writer(wrt_ctx, 0); //TODO: smth else at the last param
  }
}


void DepLibSfuShm::SfuShmCtx::CloseShmWriterCtx()
{
  // Call if writer is not closed
  if (SHM_WRT_CLOSED != wrt_status)
  {
    sfushm_av_close_writer(wrt_ctx, 0); //TODO: smth else at the last param
  }

  // TODO: do I need this->stream_name.clear() and the rest?

  //  TODO: zeromem wrt_init
}


DepLibSfuShm::ShmWriterStatus DepLibSfuShm::SfuShmCtx::SetSsrcInShmConf(uint32_t ssrc, DepLibSfuShm::ShmChunkType kind)
{
  //Assuming that ssrc does not change, shm writer is initialized, nothing else to do
  if (SHM_WRT_READY == this->Status())
    return SHM_WRT_READY;
  
  switch(kind) {
    case DepLibSfuShm::ShmChunkType::AUDIO:
      ssrc_a = ssrc; 
      this->wrt_init.conf.channels[0].audio = 1;
      this->wrt_init.conf.channels[0].ssrc = ssrc;
      this->wrt_status = (this->wrt_status == SHM_WRT_AUDIO_CHNL_CONF_MISSING) ? SHM_WRT_READY : SHM_WRT_VIDEO_CHNL_CONF_MISSING;
      break;

    case DepLibSfuShm::ShmChunkType::VIDEO:
      ssrc_v = ssrc;
      this->wrt_init.conf.channels[1].video = 1;
      this->wrt_init.conf.channels[1].ssrc = ssrc;
      this->wrt_status = (this->wrt_status == SHM_WRT_VIDEO_CHNL_CONF_MISSING) ? SHM_WRT_READY : SHM_WRT_AUDIO_CHNL_CONF_MISSING;
      break;

    default:
      return this->Status();
  }

  if (this->wrt_status == SHM_WRT_READY) {
    int err = SFUSHM_AV_OK;
    if ((err = sfushm_av_open_writer( &wrt_init, &wrt_ctx)) != SFUSHM_AV_OK) {
      MS_DEBUG_TAG(rtp, "FAILED in sfushm_av_open_writer() to initialize sfu shm %s with error %s", this->wrt_init.stream_name, GetErrorString(err));
      wrt_status = SHM_WRT_UNDEFINED;
    }
    return this->Status();
  }

  return this->Status(); // if shm was not initialized as a result, return false
}


void DepLibSfuShm::SfuShmCtx::InitializeShmWriterCtx(std::string shm, std::string log, int level, int stdio)
{
  MS_TRACE();

  memset( &wrt_init, 0, sizeof(sfushm_av_writer_init_t));

  stream_name.assign(shm);
  log_name.assign(log);

  wrt_init.stream_name = const_cast<char*>(stream_name.c_str());
	wrt_init.stats_win_size = 300;
	wrt_init.conf.log_file_name = log_name.c_str();
	wrt_init.conf.log_level = level;
	wrt_init.conf.redirect_stdio = stdio;
  
  // TODO: initialize with some different values? or keep these?
  // At this points ssrc values are still not known
  wrt_init.conf.channels[0].target_buf_ms = 20000;
  wrt_init.conf.channels[0].target_kbps   = 128;
  wrt_init.conf.channels[0].ssrc         = 0;
  wrt_init.conf.channels[0].sample_rate   = 48000;
  wrt_init.conf.channels[0].num_chn       = 2;
  wrt_init.conf.channels[0].codec_id      = SFUSHM_AV_AUDIO_CODEC_OPUS;
  wrt_init.conf.channels[0].video         = 0; 
  wrt_init.conf.channels[0].audio         = 1;

  wrt_init.conf.channels[1].target_buf_ms = 20000; 
  wrt_init.conf.channels[1].target_kbps   = 2500;
  wrt_init.conf.channels[1].ssrc          = 0;
  wrt_init.conf.channels[1].sample_rate   = 90000;
  wrt_init.conf.channels[1].codec_id      = SFUSHM_AV_VIDEO_CODEC_H264;
  wrt_init.conf.channels[1].video         = 1;
  wrt_init.conf.channels[1].audio         = 0;
}


int DepLibSfuShm::SfuShmCtx::WriteChunk(sfushm_av_frame_frag_t* data, DepLibSfuShm::ShmChunkType kind, uint32_t ssrc)
{
  int err;

  if (Status() != SHM_WRT_READY)
  {
    return 0;
  }

  switch (kind)
  {
    case DepLibSfuShm::ShmChunkType::VIDEO:
      err = sfushm_av_write_video(wrt_ctx, data);
      break;

    case DepLibSfuShm::ShmChunkType::AUDIO:
      err = sfushm_av_write_audio(wrt_ctx, data);
      break;

    case DepLibSfuShm::ShmChunkType::RTCP:
      //err = sfushm_av_write_rtcp(wrt_ctx, data);
      break;

    default:
      return -1;
  }

  if (IsError(err))
  {
    MS_WARN_TAG(rtp, "ERROR writing chunk to shm: %d - %s", err, GetErrorString(err));
    return -1;
  }
  return 0;
}

int  DepLibSfuShm::SfuShmCtx::WriteRtcpPacket(sfushm_av_rtcp_msg_t* msg)
{
  if (Status() != SHM_WRT_READY)
  {
    return 0;
  }
  
  int err = sfushm_av_write_rtcp(wrt_ctx, msg);
  
  if (IsError(err))
  {
    MS_WARN_TAG(rtp, "ERROR writing RTCP Sender Report to shm: %d - %s", err, GetErrorString(err));
    return -1;
  }
  
  return 0;
}

int DepLibSfuShm::SfuShmCtx::WriteRtcpSenderReportTs(uint64_t lastSenderReportNtpMs, uint32_t lastSenderReporTs, DepLibSfuShm::ShmChunkType kind)
{
  if (Status() != SHM_WRT_READY)
  {
    return 0;
  }
  int err;
  uint32_t ssrc;

  switch(kind)
  {
    case DepLibSfuShm::ShmChunkType::AUDIO:
    ssrc = ssrc_a;
    break;

    case DepLibSfuShm::ShmChunkType::VIDEO:
    ssrc = ssrc_v;
    break;

    default:
      return 0;
  }

  auto ntp = Utils::Time::TimeMs2Ntp(lastSenderReportNtpMs);
  auto ntp_sec = ntp.seconds;
  auto ntp_frac = ntp.fractions;

	MS_DEBUG_TAG(rtp, "RTCP SR: SSRC=%d NTP(ms)=%" PRIu64 "(%" PRIu32 "/%" PRIu32 ") RtpTs=%" PRIu32, ssrc, lastSenderReportNtpMs, ntp_sec, ntp_frac, lastSenderReporTs);

  err = sfushm_av_write_rtcp_sr_ts(wrt_ctx, ntp_sec, ntp_frac, lastSenderReporTs, ssrc);

  if (IsError(err))
  {
    MS_WARN_TAG(rtp, "ERROR writing RTCP Sender Report to shm: %d - %s", err, GetErrorString(err));
    return -1;
  }
  return 0;
}

int DepLibSfuShm::SfuShmCtx::WriteStreamMetadata(uint8_t *data, size_t len)
{
  if (Status() != SHM_WRT_READY)
  {
    // TODO: log smth
    return -1;
  }
  // TODO: no API for writing metadata yet
  return 0;
}
