#ifndef DEP_LIBSFUSHM_HPP
#define DEP_LIBSFUSHM_HPP

//#include <sfushm_av_media.h>
extern "C" 
{
#include "/root/build/ff_shm_api/include/sfushm_av_media.h"
}
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include "RTC/ShmConsumer.hpp"

#define UINT64_UNSET ((uint64_t)-1)
#define MAX_SEQ_DELTA 100
#define MAX_PTS_DELTA (90000*10)
#define MTU_SIZE       1500

namespace DepLibSfuShm
{
  enum ShmWriterStatus
  {
    SHM_WRT_UNDEFINED = 1,            // default
    SHM_WRT_CLOSED,                  // writer closed. TODO: implement whatever sets this state... producer paused or closed? transport closed?
    SHM_WRT_VIDEO_CHNL_CONF_MISSING, // when info on audio content has arrived but no video yet
    SHM_WRT_AUDIO_CHNL_CONF_MISSING, // when no info on audio is available yet, but video consumer has already started
    SHM_WRT_READY                   // writer initialized and ready to be used
    // TODO: do I need an explicit value for a writer in error state?
  };

  enum ShmChunkType
  {
    VIDEO,
    AUDIO,
    RTCP,
    UNDEFINED
  };

	// Need this to keep a buffer of roughly 2-3 video frames
	enum ShmQueueStatus
	{
		SHM_Q_PKT_CANWRITETHRU = 99,        // queue was empty, the pkt represents whole video frame or it is an aggregate, its seqId is (last_seqId+1), won't add pkt to queue
		SHM_Q_PKT_TOO_OLD = 100,            // timestamp too old, don't queue
		SHM_Q_PKT_QUEUED_OK = 101,          // added pkt data into the queue
    SHM_Q_PKT_DEQUEUED_OK = 102,        // wrote some data into shm on dequeueing
    SHM_Q_PKT_DEQUEUED_NOTHING = 103,   // nothing was written into shm on dequeueing call
    SHM_Q_PKT_WAIT_FOR_NACK = 104,      // stopped dequeueing because waiting for retransmission
		SHM_Q_PKT_CHUNKSTART_MISSING = 105, // start of fragment chunk missing, dropped the whole chunk 
		SHM_Q_PKT_SHMWRITE_ERR = 106        // shm writer returned some err, check the logs for details
	};

  // Video NALUs queue item
	struct ShmQueueItem
	{
    sfushm_av_frame_frag_t chunk;  // Cloned data representing either a whole NALU or a fragment
		uint8_t store[MTU_SIZE];       // Memory to hold the cloned pkt data, won't be larger than MTU (should match RTC::MtuSize)
    bool isChunkFragment{ false }; // If this is a picture's fragment (can be whole NALU or NALU's fragment)
		bool isChunkStart{ false };    // The first (or only) picture's fragment
		bool isChunkEnd{ false };      // Picture's last (or only) fragment

    ShmQueueItem(sfushm_av_frame_frag_t* data, bool isfragment, bool isfragmentstart, bool isfragmentend);
	};

  // Contains shm configuration, writer context (if initialized), writer status 
  class SfuShmCtx
  {
  public:
  	class Listener
		{
		public:
			virtual void OnShmWriterReady() = 0;
		};

  public:
    SfuShmCtx(): wrt_ctx(nullptr), last_seq_a(UINT64_UNSET), last_ts_a(UINT64_UNSET), last_seq_v(UINT64_UNSET), last_ts_v(UINT64_UNSET), wrt_status(SHM_WRT_UNDEFINED) {}
    ~SfuShmCtx();

    void InitializeShmWriterCtx(std::string shm, std::string log, int level, int stdio);
    void CloseShmWriterCtx();

    ShmWriterStatus Status() const { return this->wrt_status; }
    bool CanWrite() const { return (ShmWriterStatus::SHM_WRT_READY == this->wrt_status) && this->srReceived; }
    void SetListener(DepLibSfuShm::SfuShmCtx::Listener* l) { this->listener = l; }
    uint32_t AudioSsrc() const { return (this->wrt_init.conf.channels[0].audio == 1) ? this->wrt_init.conf.channels[0].ssrc : 0; }
    uint32_t VideoSsrc() const { return (this->wrt_init.conf.channels[1].video == 1) ? this->wrt_init.conf.channels[1].ssrc : 0; }
        
    void SetSsrcInShmConf(uint32_t ssrc, ShmChunkType kind);

    uint64_t AdjustPktTs(uint64_t ts, ShmChunkType kind);
    uint64_t AdjustPktSeq(uint64_t seq, ShmChunkType kind);
    void UpdatePktStat(uint64_t seq, uint64_t ts, ShmChunkType kind);
    bool IsSeqUnset(ShmChunkType kind) const;
    bool IsTsUnset(ShmChunkType kind) const;    
    uint64_t LastTs(ShmChunkType kind) const;
    uint64_t LastSeq(ShmChunkType kind) const;

    int WriteRtpDataToShm(ShmChunkType type, sfushm_av_frame_frag_t* data, bool isChunkFragment = false);
    int WriteRtcpSenderReportTs(uint64_t lastSenderReportNtpMs, uint32_t lastSenderReporTs, ShmChunkType kind);
    int WriteStreamMeta(std::string metadata, std::string shm);
    int WriteVideoOrientation(uint16_t rotation);

	  bool IsError(int err_code);
	  const char* GetErrorString(int err_code);

  private:
    ShmQueueStatus Enqueue( sfushm_av_frame_frag_t* data, bool isChunkFragment);
    ShmQueueStatus Dequeue();
    int WriteChunk(sfushm_av_frame_frag_t* data, ShmChunkType kind = ShmChunkType::UNDEFINED);

  public:
    std::string        stream_name;
    std::string        log_name;

    sfushm_av_wr_ctx_t *wrt_ctx;
    uint64_t           last_seq_a;   // last RTP pkt sequence processed by this input
	  uint64_t           last_ts_a;    // the timestamp of the last processed RTP pkt  
    uint64_t           last_seq_v;   // last RTP pkt sequence processed by this input
	  uint64_t           last_ts_v;    // the timestamp of the last processed RTP messpktage
    uint32_t           ssrc_v;       // ssrc of audio chn
    uint32_t           ssrc_a;       // ssrc of video chn
  
  private:
    sfushm_av_writer_init_t wrt_init;
    ShmWriterStatus         wrt_status;
    bool                    srReceived{ false }; // do not write into shm until SR received

		std::list<ShmQueueItem> videoPktBuffer; // Video frames queue: newest items (by seqId) added at the end of queue, oldest are read from the front

  	static std::unordered_map<int, const char*> errToString;

    Listener *listener{ nullptr }; // needs to be initialized with smth from ShmConsumer so that we can notify it when shm writer is ready
  };

  // Inline methods

  inline uint64_t SfuShmCtx::AdjustPktTs(uint64_t ts, ShmChunkType kind)
  {
    uint64_t last_ts;
    switch(kind)
    {
        case ShmChunkType::VIDEO:
        last_ts = this->last_ts_v;
        break;

        case ShmChunkType::AUDIO:
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

  inline uint64_t SfuShmCtx::AdjustPktSeq(uint64_t seq, ShmChunkType kind)
  {
    uint64_t last_seq;
    switch(kind)
    {
        case ShmChunkType::VIDEO:
        last_seq = this->last_seq_v;
        break;

        case ShmChunkType::AUDIO:
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

  inline void SfuShmCtx::UpdatePktStat(uint64_t seq, uint64_t ts, ShmChunkType kind)
  {
    //No checks, just update values since we have just written that pkt's data into shm
    switch(kind)
    {
        case ShmChunkType::VIDEO:
        this->last_seq_v = seq;
        this->last_ts_v = ts;
        break;

        case ShmChunkType::AUDIO:
        this->last_seq_a = seq;
        this->last_ts_a = ts;
        break;

        default:
        break; // TODO: will need stats on other pkt types
    }
  }


  inline bool SfuShmCtx::IsSeqUnset(ShmChunkType kind) const
  {
    if (kind == ShmChunkType::VIDEO)
      return (this->last_seq_v == UINT64_UNSET);
    else if (kind == ShmChunkType::AUDIO)
      return (this->last_seq_a == UINT64_UNSET);
    else
      return true; // no support for other pkt types yet
  }


  inline bool SfuShmCtx::IsTsUnset(ShmChunkType kind) const
  {
    if (kind == ShmChunkType::VIDEO)
      return (this->last_ts_v == UINT64_UNSET);
    else if (kind == ShmChunkType::AUDIO)
      return (this->last_ts_a == UINT64_UNSET);
    else
      return true; // no support for other pkt types yet
  }


  inline uint64_t SfuShmCtx::LastTs(ShmChunkType kind) const
  {
    if (kind == ShmChunkType::VIDEO)
      return this->last_ts_v;
    else if (kind == ShmChunkType::AUDIO)
      return this->last_ts_a;
    else
      return UINT64_UNSET; // no support for other pkt types yet
  }


  inline uint64_t SfuShmCtx::LastSeq(ShmChunkType kind) const
  {
    if (kind == ShmChunkType::VIDEO)
      return this->last_seq_v;
    else if (kind == ShmChunkType::AUDIO)
      return this->last_seq_a;
    else
      return UINT64_UNSET; // no support for other pkt types yet
  }


  inline bool SfuShmCtx::IsError(int err_code)
  {
    return (err_code != SFUSHM_AV_OK && err_code != SFUSHM_AV_AGAIN);
  }


  inline const char* SfuShmCtx::GetErrorString(int err_code)
  {
    static const char* unknownErr = "unknown SFUSHM error";

    auto it = SfuShmCtx::errToString.find(err_code);

    if (it == SfuShmCtx::errToString.end())
      return unknownErr;

    return it->second;
  }
} // namespace DepLibSfuShm

#endif // DEP_LIBSFUSHM_HPP
