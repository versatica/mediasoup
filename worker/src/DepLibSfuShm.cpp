#define MS_CLASS "DepLibSfuShm"
// #define MS_LOG_DEV

#include "DepLibSfuShm.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "DepLibUV.hpp"
#include <time.h>

namespace DepLibSfuShm {

  std::unordered_map<int, const char*> ShmCtx::errToString =
    {
      { 0, "success (SFUSHM_AV_OK)"   },
      { -1, "error (SFUSHM_AV_ERR)"   },
      { -2, "again (SFUSHM_AV_AGAIN)" }
    };

  // 100ms in samples (90000 sample rate) 
  // TODO: redo to hold constant number of picture frames instead? 
  // This will not much better because it is just a different time unit,
  // and with exception of old queue items we try to wait to assemble the whole picture frame anyway
  static constexpr uint64_t MaxVideoPktDelay{ 9000 };


  ShmCtx::~ShmCtx()
  {
    MS_TRACE();

    // Call if writer is not closed
    if (SHM_WRT_READY == wrt_status)
    {
      if ( wrt_ctx == nullptr)
        MS_WARN_TAG(xcode, "Warning: shm writer context is null but shm is still in ready state.");
      else
        sfushm_av_close_writer(wrt_ctx, 0);
    }
  }


  void ShmCtx::CloseShmWriterCtx()
  {
    MS_TRACE();

    // Call if writer is not closed
    if (SHM_WRT_READY == wrt_status)
    {
      if ( wrt_ctx == nullptr)
        MS_WARN_TAG(xcode, "Warning: shm writer context is null but shm is still in ready state.");
      else
        sfushm_av_close_writer(wrt_ctx, 0);
    }

    wrt_status = SHM_WRT_CLOSED;
    memset( &wrt_init, 0, sizeof(sfushm_av_writer_init_t));
  }


  void ShmCtx::ConfigureMediaSsrc(uint32_t ssrc, Media kind)
  {
    MS_TRACE();

    //Assuming that ssrc does not change, shm writer is initialized, nothing else to do
    if (SHM_WRT_READY == this->wrt_status)
      return;
    
    if(kind == Media::AUDIO)
    {
      if (SHM_WRT_VIDEO_CHNL_CONF_MISSING == this->wrt_status)
        return;

      this->data[0].ssrc = ssrc; 
      this->wrt_init.conf.channels[0].audio = 1;
      this->wrt_init.conf.channels[0].ssrc = ssrc;
      this->wrt_status = (this->wrt_status == SHM_WRT_AUDIO_CHNL_CONF_MISSING) ? SHM_WRT_READY : SHM_WRT_VIDEO_CHNL_CONF_MISSING;
    }
    else // video
    {
      if (SHM_WRT_AUDIO_CHNL_CONF_MISSING == this->wrt_status)
        return;

      this->data[1].ssrc = ssrc;
      this->wrt_init.conf.channels[1].video = 1;
      this->wrt_init.conf.channels[1].ssrc = ssrc;
      this->wrt_status = (this->wrt_status == SHM_WRT_VIDEO_CHNL_CONF_MISSING) ? SHM_WRT_READY : SHM_WRT_AUDIO_CHNL_CONF_MISSING;
    }

    if (this->wrt_status == SHM_WRT_READY) {
      // Just switched into ready status, can open a writer
      int err = SFUSHM_AV_OK;
      if ((err = sfushm_av_open_writer( &wrt_init, &wrt_ctx)) != SFUSHM_AV_OK) {
        MS_DEBUG_TAG(xcode, "FAILED in sfushm_av_open_writer() to initialize sfu shm %s with error %s", this->wrt_init.stream_name, GetErrorString(err));
        this->wrt_status = SHM_WRT_UNDEFINED;
      }
      else
      { 
        // Write any stored sender reports into shm and possibly notify ShmConsumer
        if (this->data[0].sr_received)
        {
          WriteSR(Media::AUDIO);
          this->data[0].sr_written = true;
        }
        
        if (this->data[1].sr_received)
        {
          WriteSR(Media::VIDEO);
          this->data[1].sr_written = true;
        }

        if (this->CanWrite())
        {
          MS_DEBUG_TAG(xcode, "shm is ready and first SRs received and written");
          this->listener->OnShmWriterReady();
        }
      }
    }
  }


  void ShmCtx::InitializeShmWriterCtx(std::string shm, std::string log, int level, int stdio)
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
    
    wrt_init.conf.channels[0].target_buf_ms = 20000;
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
    wrt_init.conf.channels[1].codec_id      = SFUSHM_AV_VIDEO_CODEC_H264;
    wrt_init.conf.channels[1].video         = 1;
    wrt_init.conf.channels[1].audio         = 0;
  }


  ShmQueueItem::ShmQueueItem(sfushm_av_frame_frag_t* chunk, bool isfragment, bool isfragmentstart, bool isfragmentend)
    : isChunkFragment(isfragment), isChunkStart(isfragmentstart), isChunkEnd(isfragmentend)
  {
    std::memcpy(&(this->chunk), chunk, sizeof(sfushm_av_frame_frag_t));
    this->chunk.data = (uint8_t*)std::addressof(this->store);
    std::memcpy(this->chunk.data, chunk->data, chunk->len);
  }


  // seqId: strictly +1 unless NALUs came from STAP-A packet, then they are the same
  // timestamps: should increment for each new picture (shm chunk), otherwise they match
  // Can see two fragments coming in with the same seqId and timestamps: SPS and PPS from the same STAP-A packet are typical
  EnqueueResult ShmCtx::Enqueue(sfushm_av_frame_frag_t* data, bool isChunkFragment)
  {
    MS_TRACE();

    uint64_t ts       = data->rtp_time;
    uint64_t seq      = data->first_rtp_seq;
    bool isChunkStart = data->begin;
    bool isChunkEnd   = data->end;
    
    // If queue is empty, seqid is in order, and pkt is not a fragment, there is no need to enqueue
    if (this->videoPktBuffer.empty()
      && !isChunkFragment
      && (IsLastVideoSeqNotSet() 
        || data->first_rtp_seq - 1 == LastVideoSeq()))
    {
      MS_DEBUG_TAG(xcode, "WRITETHRU [seq=%" PRIu64 " ts=%" PRIu64 "]", data->first_rtp_seq, data->rtp_time);
      return EnqueueResult::SHM_Q_PKT_CANWRITETHRU;
    }

    // Add a too old pkt in queue anyway
    if (!IsLastVideoSeqNotSet()
        && LastVideoTs() > ts
        && LastVideoTs() - ts > MaxVideoPktDelay)
    {
      MS_DEBUG_TAG(xcode, "ENQUEUE OLD seqid=%" PRIu64 " delta=%" PRIu64 " lastTs=%" PRIu64 " qsize=%zu", data->first_rtp_seq, LastVideoTs() - data->rtp_time, LastVideoTs(), videoPktBuffer.size());
    }
    
    // Enqueue pkt, newest at the end
    auto it = this->videoPktBuffer.begin();
    while (it != this->videoPktBuffer.end())
    {
      if (seq >= it->chunk.first_rtp_seq) // incoming seqId is newer, move further to the end
        ++it;
    }
    it = videoPktBuffer.emplace(it, data, isChunkFragment, isChunkStart, isChunkEnd);

    return SHM_Q_PKT_QUEUED_OK;
  }

  
  void ShmCtx::Dequeue()
  {
    MS_TRACE();

    if (this->videoPktBuffer.empty())
      return;

    // seqId: strictly +1 unless NALUs came from STAP-A packet, then they are the same
    // timestamps: should increment for each new picture (shm chunk), otherwise they match
    // Can see two fragments coming in with the same seqId and timestamps: SPS and PPS from the same STAP-A packet are typical
    std::_List_iterator<ShmQueueItem> chunkStartIt;

    auto it = this->videoPktBuffer.begin();
    uint64_t prev_seq = it->chunk.first_rtp_seq - 1;
    bool chunkStartFound = false;

    while (it != this->videoPktBuffer.end())
    {
      // First, write out all chunks with expired timestamps. TODO: mark incomplete fragmented pictures as corrupted (feature TBD in shm writer)
      if (!IsLastVideoSeqNotSet()
        && LastVideoTs() > it->chunk.rtp_time
        && LastVideoTs() - it->chunk.rtp_time > MaxVideoPktDelay)
      {
        MS_DEBUG_TAG(xcode, "OLD [seq=%" PRIu64 " Tsdelta=%" PRIu64 "] lastTs=%" PRIu64 " qsize=%zu", it->chunk.first_rtp_seq, LastVideoTs() - it->chunk.rtp_time, LastVideoTs(), videoPktBuffer.size());
        prev_seq = it->chunk.first_rtp_seq;
        this->WriteChunk(&it->chunk, Media::VIDEO);
        it = this->videoPktBuffer.erase(it);
        continue;
      }

      // Hole in seqIds: wait some time for missing pkt to be retransmitted
      if (it->chunk.first_rtp_seq > prev_seq + 1)
      {
        MS_DEBUG_TAG(xcode, "HOLE [seq=%" PRIu64 " ts=%" PRIu64 "] prev=%" PRIu64 " lastTs=%" PRIu64 " qsize=%zu", it->chunk.first_rtp_seq, it->chunk.rtp_time, prev_seq, LastVideoTs(), videoPktBuffer.size());
        return;
      }

      // Fragment of a chunk: start, end or middle
      if (it->isChunkFragment)
      {
        if (it->isChunkStart) // start: remember where it is and keep iterating
        {
          chunkStartFound = true;
          chunkStartIt = it;
          prev_seq = it->chunk.first_rtp_seq;
          ++it;
        }
        else // mid or end
        {
          if (!chunkStartFound)
          {
            // chunk incomplete, wait for retransmission
          //  MS_DEBUG_TAG(rtp, "NO START fragment, wait: [seq=%" PRIu64 " ts=%" PRIu64 "] qsize=%zu", it->chunk.first_rtp_seq, it->chunk.rtp_time, videoPktBuffer.size());
            return;
          }

          // write the whole chunk if we have it
          if (it->isChunkEnd)
          {
            auto nextit = it;
            uint64_t startSeqId = chunkStartIt->chunk.first_rtp_seq;
            uint64_t startTs = chunkStartIt->chunk.rtp_time;
            uint64_t endSeqId = it->chunk.first_rtp_seq;
            uint64_t endTs = it->chunk.rtp_time;
            prev_seq = it->chunk.first_rtp_seq;

            nextit++; // position past last fragment in a chunk
            while (chunkStartIt != nextit)
            {
              /*MS_DEBUG_TAG(rtp, "writing fragment seq=%" PRIu64 " [%X%X%X%X]",
                chunkStartIt->chunk.first_rtp_seq,
                chunkStartIt->chunk.data[0],chunkStartIt->chunk.data[1],chunkStartIt->chunk.data[2],chunkStartIt->chunk.data[3]); */
              this->WriteChunk(&chunkStartIt->chunk, Media::VIDEO);
              chunkStartIt = this->videoPktBuffer.erase(chunkStartIt);
            }
            MS_DEBUG_TAG(xcode, "FRAG start [seq=%" PRIu64 " ts=%" PRIu64 "] end [seq=%" PRIu64 " ts=%" PRIu64 "] qsize=%zu", startSeqId, startTs, endSeqId, endTs, videoPktBuffer.size());
            chunkStartFound = false;
            prev_seq = endSeqId;
            it = nextit; // restore iterator
          }
          else
          {
            prev_seq = it->chunk.first_rtp_seq;
            ++it;
          }
        }
      }
      else // non-fragmented chunk
      {
        this->WriteChunk(&it->chunk, Media::VIDEO);
        MS_DEBUG_TAG(xcode, "FULL [seq=%" PRIu64 " ts=%" PRIu64 "] lastTs=%" PRIu64 " qsize=%zu", it->chunk.first_rtp_seq, it->chunk.rtp_time, LastVideoTs(), videoPktBuffer.size());
        prev_seq = it->chunk.first_rtp_seq;
        it = this->videoPktBuffer.erase(it);
        continue;
      }
    }
  }


  void ShmCtx::WriteRtpDataToShm(Media type, sfushm_av_frame_frag_t* frag, bool isChunkFragment)
  {
    MS_TRACE();

    if (type == Media::VIDEO)
    {
      if (SHM_Q_PKT_CANWRITETHRU == this->Enqueue(frag, isChunkFragment)) // Write it into shm, no need to queue anything
      {
        this->WriteChunk(frag, Media::VIDEO);
      }

      this->Dequeue();
    }
    else
    {
      this->WriteChunk(frag, Media::AUDIO); // do not queue audio
    }
  }


  void ShmCtx::WriteChunk(sfushm_av_frame_frag_t* data, Media kind)
  {
    MS_TRACE();

    if (!this->CanWrite())
    {
      MS_WARN_TAG(xcode, "Cannot write chunk ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " because shm writer is not initialized",
        data->ssrc, data->first_rtp_seq, data->rtp_time);
      return;
    }

    int err;
    if(kind == Media::AUDIO)
    {
      err = sfushm_av_write_audio(wrt_ctx, data);
    }
    else
    {
      err = sfushm_av_write_video(wrt_ctx, data);
    }

    if (IsError(err))
      MS_WARN_TAG(xcode, "ERROR writing chunk ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " to shm: %d - %s", data->ssrc, data->first_rtp_seq, data->rtp_time, err, GetErrorString(err));
  }


  void ShmCtx::WriteRtcpSenderReportTs(uint64_t lastSenderReportNtpMs, uint32_t lastSenderReporTs, Media kind)
  {
    MS_TRACE();

    /*
  .. c:function:: uint64_t uv_hrtime(void)

      Returns the current high-resolution real time. This is expressed in
      nanoseconds. It is relative to an arbitrary time in the past. It is not
      related to the time of day and therefore not subject to clock drift. The
      primary use is for measuring performance between intervals.  */
  
    uint64_t walltime = DepLibUV::GetTimeMs();
    struct timespec clockTime;
    time_t ct;
    if (clock_gettime( CLOCK_REALTIME, &clockTime) == -1) {
      ct = 0;
    }
    ct = clockTime.tv_sec;
    uint64_t clockTimeMs = (clockTime.tv_sec * (uint64_t) 1e9 + clockTime.tv_nsec) / 1000000.0;

    auto ntp = Utils::Time::TimeMs2Ntp(lastSenderReportNtpMs);
    size_t idx = (kind == Media::AUDIO) ? 0 : 1;

    this->data[idx].sr_received = true;
    this->data[idx].sr_ntp_msb = ntp.seconds;
    this->data[idx].sr_ntp_lsb = ntp.fractions;
    this->data[idx].sr_rtp_tm = lastSenderReporTs;

    MS_DEBUG_TAG(xcode, "Received RTCP SR: SSRC=%" PRIu32 " ReportNTP(ms)=%" PRIu64 " RtpTs=%" PRIu32 " uv_hrtime(ms)=%" PRIu64 " clock_gettime(s)=%" PRIu64 " clock_gettime(ms)=%" PRIu64,
      this->data[idx].ssrc,
      lastSenderReportNtpMs,
      lastSenderReporTs,
      walltime,
      ct,
      clockTimeMs);

    if (SHM_WRT_READY != this->wrt_status)
      return; // shm writer is not set up yet

    WriteSR(kind);

    if (this->CanWrite())
      return; // already submitted the first SRs into shm, and it is in ready state

    bool shouldNotify = false;
    if(kind == Media::AUDIO)
    {
      if (this->data[1].sr_written && !this->data[0].sr_written)
        shouldNotify = true;
      this->data[0].sr_written = true;
    }
    else
    {
      if (this->data[0].sr_written && !this->data[1].sr_written)
        shouldNotify = true;
      this->data[1].sr_written = true;    
    }

    if (shouldNotify)
    {
      MS_DEBUG_TAG(xcode, "First SRs received, and shm is ready");
      this->listener->OnShmWriterReady();
    }
  }


  void ShmCtx::WriteSR(Media kind)
  {
    MS_TRACE();

    size_t idx = (kind == Media::AUDIO) ? 0 : 1;
    int err = sfushm_av_write_rtcp_sr_ts(wrt_ctx, this->data[idx].sr_ntp_msb, this->data[idx].sr_ntp_lsb, this->data[idx].sr_rtp_tm, this->data[idx].ssrc);
    if (IsError(err))
    {
      MS_WARN_TAG(xcode, "ERROR writing RTCP SR for ssrc %" PRIu32 " %d - %s", this->data[idx].ssrc, err, GetErrorString(err));
    }
  }


  int ShmCtx::WriteStreamMeta(std::string metadata, std::string shm)
  {
    MS_TRACE();

    if (SHM_WRT_READY != this->wrt_status)
    {
      MS_WARN_TAG(xcode, "Cannot write stream metadata because shm writer is not initialized");
      return -1; // shm writer is not set up yet
    }

    int     err;
    uint8_t data[256]; // Currently this is overkill because just 1 byte will be written successfully into shm

    if (0 != this->stream_name.compare(shm))
    {
      MS_WARN_TAG(xcode, "input metadata shm name '%s' does not match '%s'", shm.c_str(), this->stream_name.c_str());
      return -1;
    }

    if (metadata.length() > 255)
    {
      MS_WARN_TAG(xcode, "input metadata is too long: %s", metadata.c_str());
    }
    std::copy(metadata.begin(), metadata.end(), data);

    err = sfushm_av_write_stream_metadata(wrt_ctx, data, metadata.length());
    if (IsError(err))
    {
      MS_WARN_TAG(xcode, "ERROR writing stream metadata: %d - %s", err, GetErrorString(err));
      return -1;
    }

    return 0;
  }


  void ShmCtx::WriteVideoOrientation(uint16_t rotation)
  {
    MS_TRACE();

    if (SHM_WRT_READY != this->wrt_status)
    {
      MS_WARN_TAG(xcode, "Cannot write video rotation because shm writer is not initialized");
      return; // shm writer is not set up yet
    }

    int err = sfushm_av_write_video_rotation(wrt_ctx, rotation);

    if (IsError(err))
      MS_WARN_TAG(xcode, "ERROR writing video rotation: %d - %s", err, GetErrorString(err));
  }
} // namespace DepLibSfuShm