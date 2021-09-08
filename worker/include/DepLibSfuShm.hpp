#ifndef DEP_LIBSFUSHM_HPP
#define DEP_LIBSFUSHM_HPP

//#include <sfushm_av_media.h>
extern "C" 
{
#include "/root/build/ff_shm_api/include/sfushm_av_media.h"
#include "/root/build/ff_shm_api/include/sfushm_bin_log.h"
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
    SHM_WRT_UNDEFINED = 1, // default
    SHM_WRT_READY,         // writer initialized and ready to be used
    SHM_WRT_CLOSED         // writer closed. TODO: implement whatever sets this state... producer paused or closed? transport closed?
  };

  enum Media
  {
    VIDEO,
    AUDIO
  };

	// Need this to keep a buffer of roughly 2-3 video frames
	enum EnqueueResult
	{
		SHM_Q_PKT_QUEUED_OK     // added pkt data into the queue
	};

  enum AnnexB
  {
    NO_HEADER,
    LONG_HEADER,
    SHORT_HEADER
  };

  // If Sender Report arrived but shm writer could not be initialized yet, we can save it and write it in as soon as shm writer is initialized
  struct MediaState
  {
    uint64_t last_rtp_seq{ UINT64_UNSET }; // last RTP pkt sequence processed by this input
	  uint64_t last_ts{ UINT64_UNSET };      // the timestamp of the last processed RTP message
    uint32_t new_ssrc{ 0 };                // mapped ssrc - always generated
    bool     sr_received{ false };         // if sender report was received
    bool     sr_written{ false };          // if sender report was written into shm
    uint32_t sr_ntp_msb{ 0 };
    uint32_t sr_ntp_lsb{ 0 };
    uint64_t sr_rtp_tm{ 0 };
    uint64_t rtp_seq_offset{ 0 };
  };

  // Video NALUs queue item
	struct ShmQueueItem
	{
		uint8_t  store[MTU_SIZE]{ 0 };   // Memory to hold the cloned pkt data, won't be larger than MTU (should match RTC::MtuSize)
    uint8_t *data{ nullptr };        // pointer to data start in store
    size_t   len;                    // length of data in store
    uint64_t seqid;                  // seq id of the item
    uint64_t ts;                     // RTP timestamp
    uint8_t  nal{ 0 };               // NALU type
    bool     fragment{ false };      // If this is a picture's fragment (can be whole NALU or NALU's fragment)
    bool     firstfragment{ false }; // Only makes sense if fragment == true, fragment with start bit set; means the first picture frame's fragment in case when SPS and PPS were just sent (then SPS begins the picture)
    bool     endfragment{ false };   // Only makes sense if fragment == true; fragment that had end bit set
	  bool     beginpicture{ false };  // The first (or only) picture's fragment
		bool     endpicture{ false };    // Picture's last (or only) fragment
    bool     keyframe{ false };      // This item belongs to key frame

    ShmQueueItem() = default;
    ShmQueueItem(uint8_t* data, size_t datalen, uint64_t seq, uint64_t timestamp,uint8_t nalu, bool isfragment, bool isfirstfrag, bool isendfrag, bool isbeginpicture, bool isendpicture, bool iskeyframe);
	};

  // Contains shm configuration, writer context (if initialized), writer status 
  class ShmCtx
  {
  public:
  	class Listener
		{
		public:
			virtual void OnNeedToSync() = 0;
		};

  public:
    ShmCtx();
    ~ShmCtx();

    static uint8_t* hex_dump(uint8_t *dst, uint8_t *src, size_t len);

    void InitializeShmWriterCtx(std::string shm, int queueAge, bool useReverse, int testNack, std::string log, int level);
    void CloseShmWriterCtx();

    std::string StreamName() const { return this->stream_name; }
    std::string LogName() const { return this->log_name; }
    ShmWriterStatus Status() const { return this->wrt_status; }
    uint64_t MaxQueuePktDelayMs() const { return this->maxVideoPktDelay; }
    uint64_t TestNackMs() const { return this->testNackEachMs; }

    bool CanWrite(Media kind) const;
    void SetListener(DepLibSfuShm::ShmCtx::Listener* l) { this->listener = l; }
    uint64_t AdjustVideoPktTs(uint64_t ts);
    uint64_t AdjustAudioPktTs(uint64_t ts);
    uint64_t AdjustVideoPktSeq(uint64_t seq);
    uint64_t AdjustAudioPktSeq(uint64_t seq);
    void UpdateRtpStats(uint64_t seq, uint64_t ts, Media kind);
    void ResetShmMediaStatsAndQueue(Media kind);

    bool IsLastVideoSeqNotSet() const;  
    uint64_t LastVideoTs() const;
    uint64_t LastVideoSeq() const;

    void WriteAudioRtpDataToShm(uint8_t *data, size_t len, uint64_t seqid, uint64_t ts);
    void WriteVideoRtpDataToShm(uint8_t *data, size_t len, uint64_t seqid, uint64_t ts, uint8_t nal, bool isfragment, bool isfirstfragment, bool isendfragment, bool ispicturebegin, bool ispictureend, bool iskeyframe);
    void WriteRtcpSenderReportTs(uint64_t lastSenderReportNtpMs, uint32_t lastSenderReportTs, Media kind);
    int WriteStreamMeta(std::string metadata, std::string shm);
    void WriteVideoOrientation(uint16_t rotation);

  public:
    // Binary log, public so that ShmConsumer or ShmTransport can write some data in
    uint64_t                          bin_log_rec_intervals {2000}; // optionally configurable via sfui config
    static xcode_sfushm_bin_log_ctx_t bin_log_ctx;                  // context for writing statistics to binary log
    #define bin_log_record bin_log_ctx.record

    void DumpBinLogDataIfNeeded(bool signal_set);


  private:
    void WriteAudioChunk(sfushm_av_frame_frag_t* data);
    void WriteVideoChunk(ShmQueueItem* item, bool invalid);
    
    void WriteSR(Media kind);

    EnqueueResult Enqueue(uint8_t *data, size_t len, uint64_t seqid, uint64_t ts, uint8_t nal, bool isfragment, bool isfirstfrag, bool isendfrag, bool isbeginpicture, bool isendpicture, bool iskeyframe);
    void Dequeue();
    void WriteFrame(std::list<ShmQueueItem>::iterator& firstIt, std::list<ShmQueueItem>::iterator& lastIt, bool invalid);
    bool GetNextFrame(std::list<ShmQueueItem>::iterator& firstIt, std::list<ShmQueueItem>::iterator& lastIt, bool& gaps);
  
	  bool IsError(int err_code);
	  const char* GetErrorString(int err_code);

  private:
    std::string             stream_name;
    std::string             log_name;
    sfushm_av_writer_init_t wrt_init;
    sfushm_av_wr_ctx_t      *wrt_ctx;           // 0 - audio, 1 - video
    ShmWriterStatus         wrt_status;
    MediaState              media[2];           // 0 - audio, 1 - video
    Listener                *listener{nullptr}; // can notify video consumer that shm writer is ready, and request a key frame

    std::list<ShmQueueItem> videoPktBuffer;                // Video frames queue: newest items (by seqId) added at the end of queue, oldest are read from the front
    uint64_t                maxVideoPktDelay {9000};       // Default is 100ms at 90000 sample rate
    bool                    useReverseIterator {false};    // Test Enqueue() with plain or reverse iterator 
    uint64_t testNackEachMs {0}; // Pass this value to ShmConsumer, if not 0 it will drop incoming pkt periodically and form NACK request

    uint64_t                lastKeyFrameTs {UINT64_UNSET}; // keep track of the keyframes sitting in videoPktBuffer queue: if there is one waiting - do not re-request another

  	static std::unordered_map<int, const char*> errToString;
  };

  // Inline methods

  // Begin writing into shm when shm writer is initialized (only happens after ssrcs for both audio and video are known)
  // and both Sender Reports have been received and written into shm
  inline bool ShmCtx::CanWrite(Media kind) const
  { 
    return nullptr != this->wrt_ctx 
      && ShmWriterStatus::SHM_WRT_READY == this->wrt_status
      && (kind == Media::AUDIO) ? this->media[0].sr_written : this->media[1].sr_written;
  }

  inline uint64_t ShmCtx::AdjustAudioPktTs(uint64_t ts)
  {
    if (this->media[0].last_ts != UINT64_UNSET) {
      sfushm_av_adjust_for_overflow_32_64(this->media[0].last_ts, &ts, MAX_PTS_DELTA);
    }
    return ts;
  }

  inline uint64_t ShmCtx::AdjustAudioPktSeq(uint64_t seq)
  {
    if (this->media[0].last_rtp_seq != UINT64_UNSET) {
      sfushm_av_adjust_for_overflow_16_64(this->media[0].last_rtp_seq, &seq, MAX_SEQ_DELTA);
    }
    return seq;
  }

  inline uint64_t ShmCtx::AdjustVideoPktTs(uint64_t ts)
  {
    if (this->media[1].last_ts != UINT64_UNSET) {
      sfushm_av_adjust_for_overflow_32_64(this->media[1].last_ts, &ts, MAX_PTS_DELTA);
    }
    return ts;
  }

  inline uint64_t ShmCtx::AdjustVideoPktSeq(uint64_t seq)
  {
    if (this->media[1].last_rtp_seq != UINT64_UNSET) {
      sfushm_av_adjust_for_overflow_16_64(this->media[1].last_rtp_seq, &seq, MAX_SEQ_DELTA);
    }
    return seq;
  }
  
  inline void ShmCtx::UpdateRtpStats(uint64_t seq, uint64_t ts, Media kind)
  {
    //No checks, just update values since we have just written that pkt's data into shm
    if (kind == Media::AUDIO)
    {
      this->media[0].last_rtp_seq = seq;
      this->media[0].last_ts = ts;
    }
    else // video
    {
      this->media[1].last_rtp_seq = seq;
      this->media[1].last_ts = ts;
    }
  }

  inline bool ShmCtx::IsLastVideoSeqNotSet() const
  {
      return (this->media[1].last_rtp_seq == UINT64_UNSET);
  }

  inline uint64_t ShmCtx::LastVideoTs() const
  {
    return this->media[1].last_ts;
  }

  inline uint64_t ShmCtx::LastVideoSeq() const
  {
    return this->media[1].last_rtp_seq;
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
