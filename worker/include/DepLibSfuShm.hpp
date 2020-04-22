#ifndef DEP_LIBSFUSHM_HPP
#define DEP_LIBSFUSHM_HPP

//#include <sfushm_av_media.h>
extern "C" 
{
#include "/root/build/ff_shm_api/include/sfushm_av_media.h"
}
#include <vector>
#include <string>
#include <unordered_map>

#define UINT64_UNSET ((uint64_t)-1)
#define MAX_SEQ_DELTA 100
#define MAX_PTS_DELTA (90000*10)


class DepLibSfuShm
{
public:
  enum ShmWriterStatus {
    SHM_WRT_UNDEFINED = 1,            // default
    SHM_WRT_CLOSED,                  // writer closed. TODO: implement whatever sets this state... producer paused or closed? transport closed?
    SHM_WRT_VIDEO_CHNL_CONF_MISSING, // when info on audio content has arrived but no video yet
    SHM_WRT_AUDIO_CHNL_CONF_MISSING, // when no info on audio is available yet, but video consumer has already started
    SHM_WRT_READY                   // writer initialized and ready to be used
    // TODO: do I need an explicit value for a writer in error state?
  };

  enum ShmChunkType {
    VIDEO,
    AUDIO,
    RTCP,
    UNDEFINED
  };

  // Contains shm configuration, writer context (if initialized), writer status 
  class SfuShmCtx
  {
  public:
    SfuShmCtx(): wrt_ctx(nullptr), last_seq_a(UINT64_UNSET), last_ts_a(UINT64_UNSET), last_seq_v(UINT64_UNSET), last_ts_v(UINT64_UNSET), wrt_status(SHM_WRT_UNDEFINED) {}
    ~SfuShmCtx();

    void InitializeShmWriterCtx(std::string shm, std::string log, int level, int stdio);
    void CloseShmWriterCtx();

    ShmWriterStatus Status() const { return this->wrt_status; } // (this->wrt_ctx != nullptr ? this->wrt_status : SHM_WRT_UNDEFINED); }
    uint32_t AudioSsrc() const { return (this->wrt_init.conf.channels[0].audio == 1) ? this->wrt_init.conf.channels[0].ssrc : 0; }
    uint32_t VideoSsrc() const { return (this->wrt_init.conf.channels[1].video == 1) ? this->wrt_init.conf.channels[1].ssrc : 0; }
        
    ShmWriterStatus SetSsrcInShmConf(uint32_t ssrc, DepLibSfuShm::ShmChunkType kind);

    uint64_t AdjustPktTs(uint64_t ts, DepLibSfuShm::ShmChunkType kind);
    uint64_t AdjustPktSeq(uint64_t seq, DepLibSfuShm::ShmChunkType kind);
    void UpdatePktStat(uint64_t seq, uint64_t ts, DepLibSfuShm::ShmChunkType kind);
    bool IsSeqUnset(DepLibSfuShm::ShmChunkType kind) const;
    bool IsTsUnset(DepLibSfuShm::ShmChunkType kind) const;    
    uint64_t LastTs(DepLibSfuShm::ShmChunkType kind) const;
    uint64_t LastSeq(DepLibSfuShm::ShmChunkType kind) const;

    int WriteChunk(sfushm_av_frame_frag_t* data, DepLibSfuShm::ShmChunkType kind = DepLibSfuShm::ShmChunkType::UNDEFINED, uint32_t ssrc = 0);
    int WriteRtcpSenderReportTs(uint64_t lastSenderReportNtpMs, uint32_t lastSenderReporTs, DepLibSfuShm::ShmChunkType kind);
    int WriteRtcpPacket(sfushm_av_rtcp_msg_t* msg);
    int WriteStreamMetadata(uint8_t *data, size_t len);

	  bool IsError(int err_code);
	  const char* GetErrorString(int err_code);

  public:
    std::string        stream_name;
    std::string        log_name;

    sfushm_av_wr_ctx_t *wrt_ctx;
    uint64_t           last_seq_a;   // last RTP sequence processed by this input
	  uint64_t           last_ts_a;    // the timestamp of the last processed RTP message  
    uint64_t           last_seq_v;   // last RTP sequence processed by this input
	  uint64_t           last_ts_v;    // the timestamp of the last processed RTP message
    uint32_t           ssrc_v;       // ssrc of audio chn
    uint32_t           ssrc_a;       // ssrc of video chn
  
    int                last_err {0}; // 
  
  private:
    sfushm_av_writer_init_t  wrt_init;
    ShmWriterStatus          wrt_status;

  	static std::unordered_map<int, const char*> errToString;
  };
};


// Inline methods

inline uint64_t DepLibSfuShm::SfuShmCtx::AdjustPktTs(uint64_t ts, DepLibSfuShm::ShmChunkType kind)
{
  uint64_t last_seq;
  uint64_t last_ts;
  switch(kind)
  {
      case DepLibSfuShm::ShmChunkType::VIDEO:
      last_seq = this->last_seq_v;
      last_ts = this->last_ts_v;
      break;

      case DepLibSfuShm::ShmChunkType::AUDIO:
      last_seq = this->last_seq_a;
      last_ts = this->last_ts_a;
      break;

      default:
      return ts; // do nothing
  }

  if (last_ts != UINT64_UNSET) {
  	sfushm_av_adjust_for_overflow_32_64(last_ts, &ts, MAX_PTS_DELTA);
  }
  return ts;
}

inline uint64_t DepLibSfuShm::SfuShmCtx::AdjustPktSeq(uint64_t seq, DepLibSfuShm::ShmChunkType kind)
{
  uint64_t last_seq;
  switch(kind)
  {
      case DepLibSfuShm::ShmChunkType::VIDEO:
      last_seq = this->last_seq_v;
      break;

      case DepLibSfuShm::ShmChunkType::AUDIO:
      last_seq = this->last_seq_a;
      break;

      default:
      return seq; // do nothing
  }

  if (last_seq != UINT64_UNSET) {
		sfushm_av_adjust_for_overflow_16_64(last_seq, &seq, MAX_SEQ_DELTA);
  }
  return seq;
}

inline void DepLibSfuShm::SfuShmCtx::UpdatePktStat(uint64_t seq, uint64_t ts, DepLibSfuShm::ShmChunkType kind)
{
  //No checks, just update values since we have just written that pkt's data into shm
  switch(kind)
  {
      case DepLibSfuShm::ShmChunkType::VIDEO:
      this->last_seq_v = seq;
      this->last_ts_v = ts;
      break;

      case DepLibSfuShm::ShmChunkType::AUDIO:
      this->last_seq_a = seq;
      this->last_ts_a = ts;
      break;

      default:
      break; // TODO: will need stats on other pkt types
  }
}


inline bool DepLibSfuShm::SfuShmCtx::IsSeqUnset(DepLibSfuShm::ShmChunkType kind) const
{
  if (kind == DepLibSfuShm::ShmChunkType::VIDEO)
    return (this->last_seq_v == UINT64_UNSET);
  else if (kind == DepLibSfuShm::ShmChunkType::AUDIO)
    return (this->last_seq_a == UINT64_UNSET);
  else
    return true; // no support for other pkt types yet
}


inline bool DepLibSfuShm::SfuShmCtx::IsTsUnset(DepLibSfuShm::ShmChunkType kind) const
{
  if (kind == DepLibSfuShm::ShmChunkType::VIDEO)
    return (this->last_ts_v == UINT64_UNSET);
  else if (kind == DepLibSfuShm::ShmChunkType::AUDIO)
    return (this->last_ts_a == UINT64_UNSET);
  else
    return true; // no support for other pkt types yet
}


inline uint64_t DepLibSfuShm::SfuShmCtx::LastTs(DepLibSfuShm::ShmChunkType kind) const
{
  if (kind == DepLibSfuShm::ShmChunkType::VIDEO)
    return this->last_ts_v;
  else if (kind == DepLibSfuShm::ShmChunkType::AUDIO)
    return this->last_ts_a;
  else
    return UINT64_UNSET; // no support for other pkt types yet
}


inline uint64_t DepLibSfuShm::SfuShmCtx::LastSeq(DepLibSfuShm::ShmChunkType kind) const
{
  if (kind == DepLibSfuShm::ShmChunkType::VIDEO)
    return this->last_seq_v;
  else if (kind == DepLibSfuShm::ShmChunkType::AUDIO)
    return this->last_seq_a;
  else
    return UINT64_UNSET; // no support for other pkt types yet
}


inline bool DepLibSfuShm::SfuShmCtx::IsError(int err_code)
{
	return (err_code != SFUSHM_AV_OK && err_code != SFUSHM_AV_AGAIN);
}


inline const char* DepLibSfuShm::SfuShmCtx::GetErrorString(int err_code)
{
  static const char* unknownErr = "unknown SFUSHM error";

  auto it = DepLibSfuShm::SfuShmCtx::errToString.find(err_code);

  if (it == DepLibSfuShm::SfuShmCtx::errToString.end())
    return unknownErr;

  return it->second;
}

#endif // DEP_LIBSFUSHM_HPP
