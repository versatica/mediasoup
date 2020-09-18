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

  enum Media
  {
    VIDEO,
    AUDIO
  };

  // If Sender Report arrived but shm writer could not be initialized yet, we can save it and write it in as soon as shm writer is initialized
  struct MediaState
  {
    uint64_t last_seq{ UINT64_UNSET };   // last RTP pkt sequence processed by this input
	  uint64_t last_ts{ UINT64_UNSET };    // the timestamp of the last processed RTP message
    uint32_t ssrc{ 0 };                  // ssrc of audio or video chn
    bool     sr_received{ false };       // if sender report was received
    bool     sr_written{ false };        // if sender report was written into shm
    uint32_t sr_ntp_msb{ 0 };
    uint32_t sr_ntp_lsb{ 0 };
    uint64_t sr_rtp_tm{ 0 };
  };

	// Need this to keep a buffer of roughly 2-3 video frames
	enum EnqueueResult
	{
		SHM_Q_PKT_CANWRITETHRU, // queue was empty, the pkt represents whole video frame or it is an aggregate, its seqId is (last_seqId+1), won't add pkt to queue
		SHM_Q_PKT_QUEUED_OK     // added pkt data into the queue
	};

  // Video NALUs queue item
	struct ShmQueueItem
	{
    sfushm_av_frame_frag_t chunk;                    // Cloned data representing either a whole NALU or a fragment
		uint8_t                store[MTU_SIZE];          // Memory to hold the cloned pkt data, won't be larger than MTU (should match RTC::MtuSize)
    bool                   isChunkFragment{ false }; // If this is a picture's fragment (can be whole NALU or NALU's fragment)
		bool                   isChunkStart{ false };    // The first (or only) picture's fragment
		bool                   isChunkEnd{ false };      // Picture's last (or only) fragment

    ShmQueueItem(sfushm_av_frame_frag_t* data, bool isfragment, bool isfragmentstart, bool isfragmentend);
	};

  // Contains shm configuration, writer context (if initialized), writer status 
  class ShmCtx
  {
  public:
  	class Listener
		{
		public:
			virtual void OnShmWriterReady() = 0;
		};

  public:
    ShmCtx(): wrt_ctx(nullptr), wrt_status(SHM_WRT_UNDEFINED) {}
    ~ShmCtx();

    void InitializeShmWriterCtx(std::string shm, std::string log, int level, int stdio);
    void CloseShmWriterCtx();

    bool CanWrite() const;
    void SetListener(DepLibSfuShm::ShmCtx::Listener* l) { this->listener = l; }

    void ConfigureMediaSsrc(uint32_t ssrc, Media kind);

    uint64_t AdjustVideoPktTs(uint64_t ts);
    uint64_t AdjustAudioPktTs(uint64_t ts);
    uint64_t AdjustVideoPktSeq(uint64_t seq);
    uint64_t AdjustAudioPktSeq(uint64_t seq);
    void UpdatePktStat(uint64_t seq, uint64_t ts, Media kind);

    bool IsLastVideoSeqNotSet() const;  
    uint64_t LastVideoTs() const;
    uint64_t LastVideoSeq() const;

    void WriteRtpDataToShm(Media type, sfushm_av_frame_frag_t* data, bool isChunkFragment = false);
    void WriteRtcpSenderReportTs(uint64_t lastSenderReportNtpMs, uint32_t lastSenderReporTs, Media kind);
    int WriteStreamMeta(std::string metadata, std::string shm);
    void WriteVideoOrientation(uint16_t rotation);

  private:
    EnqueueResult Enqueue( sfushm_av_frame_frag_t* data, bool isChunkFragment);
    void Dequeue();
    
    void WriteChunk(sfushm_av_frame_frag_t* data, Media kind);
    void WriteSR(Media kind);

	  bool IsError(int err_code);
	  const char* GetErrorString(int err_code);

  public:
    std::string        stream_name;
    std::string        log_name;

  
  private:
    sfushm_av_writer_init_t wrt_init;
    sfushm_av_wr_ctx_t     *wrt_ctx;        // 0 - audio, 1 - video
    ShmWriterStatus         wrt_status;

    MediaState              data[2];        // 0 - audio, 1 - video
		std::list<ShmQueueItem> videoPktBuffer; // Video frames queue: newest items (by seqId) added at the end of queue, oldest are read from the front

    Listener *listener{ nullptr }; // needs to be initialized with smth from ShmConsumer so that we can notify it when shm writer is ready. TODO: see if it's worth the effort
  	static std::unordered_map<int, const char*> errToString;
  };

  // Inline methods

  // Begin writing into shm when shm writer is initialized (only happens after ssrcs for both audio and video are known)
  // and both Sender Reports have been received and written into shm
  inline bool ShmCtx::CanWrite() const
  { 
    return nullptr != this->wrt_ctx 
      && ShmWriterStatus::SHM_WRT_READY == this->wrt_status
      && this->data[0].sr_written
      && this->data[1].sr_written;
  }

  inline uint64_t ShmCtx::AdjustAudioPktTs(uint64_t ts)
  {
    if (this->data[0].last_ts != UINT64_UNSET) {
      sfushm_av_adjust_for_overflow_32_64(this->data[0].last_ts, &ts, MAX_PTS_DELTA);
    }
    return ts;
  }

  inline uint64_t ShmCtx::AdjustAudioPktSeq(uint64_t seq)
  {
    if (this->data[0].last_seq != UINT64_UNSET) {
      sfushm_av_adjust_for_overflow_16_64(this->data[0].last_seq, &seq, MAX_SEQ_DELTA);
    }
    return seq;
  }

  inline uint64_t ShmCtx::AdjustVideoPktTs(uint64_t ts)
  {
    if (this->data[1].last_ts != UINT64_UNSET) {
      sfushm_av_adjust_for_overflow_32_64(this->data[1].last_ts, &ts, MAX_PTS_DELTA);
    }
    return ts;
  }

  inline uint64_t ShmCtx::AdjustVideoPktSeq(uint64_t seq)
  {
    if (this->data[1].last_seq != UINT64_UNSET) {
      sfushm_av_adjust_for_overflow_16_64(this->data[1].last_seq, &seq, MAX_SEQ_DELTA);
    }
    return seq;
  }
  
  inline void ShmCtx::UpdatePktStat(uint64_t seq, uint64_t ts, Media kind)
  {
    //No checks, just update values since we have just written that pkt's data into shm
    if (kind == Media::AUDIO)
    {
      this->data[0].last_seq = seq;
      this->data[0].last_ts = ts;
    }
    else // video
    {
      this->data[1].last_seq = seq;
      this->data[1].last_ts = ts;
    }
  }

  inline bool ShmCtx::IsLastVideoSeqNotSet() const
  {
      return (this->data[1].last_seq == UINT64_UNSET);
  }

  inline uint64_t ShmCtx::LastVideoTs() const
  {
    return this->data[1].last_ts;
  }

  inline uint64_t ShmCtx::LastVideoSeq() const
  {
    return this->data[1].last_seq;
  }

  inline bool ShmCtx::IsError(int err_code)
  {
    return (err_code != SFUSHM_AV_OK && err_code != SFUSHM_AV_AGAIN);
  }

  inline const char* ShmCtx::GetErrorString(int err_code)
  {
    static const char* unknownErr = "unknown SFUSHM error";

    auto it = ShmCtx::errToString.find(err_code);

    if (it == ShmCtx::errToString.end())
      return unknownErr;

    return it->second;
  }
} // namespace DepLibSfuShm

#endif // DEP_LIBSFUSHM_HPP
