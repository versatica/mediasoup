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

  std::unordered_map<int, const char*> DepLibSfuShm::SfuShmCtx::errToString =
    {
      { 0, "success (SFUSHM_AV_OK)"   },
      { -1, "error (SFUSHM_AV_ERR)"   },
      { -2, "again (SFUSHM_AV_AGAIN)" },
      { 100, "too old pkt ts (SHM_Q_PKT_TOO_OLD)"}
    };


  static constexpr uint64_t MaxVideoPktDelay{ 9000 }; // 100ms in samples (90000 sample rate) TODO: any way to better estimate this one?


  SfuShmCtx::~SfuShmCtx()
  {
    // Call if writer is not closed
    if (SHM_WRT_CLOSED != wrt_status && SHM_WRT_UNDEFINED != wrt_status)
    {
      sfushm_av_close_writer(wrt_ctx, 0);
    }
  }


  void SfuShmCtx::CloseShmWriterCtx()
  {
    // Call if writer is not closed
    if (SHM_WRT_CLOSED != wrt_status)
    {
      sfushm_av_close_writer(wrt_ctx, 0);
    }

    wrt_status = SHM_WRT_CLOSED;
    memset( &wrt_init, 0, sizeof(sfushm_av_writer_init_t));
  }


  ShmWriterStatus SfuShmCtx::SetSsrcInShmConf(uint32_t ssrc, DepLibSfuShm::ShmChunkType kind)
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


  void SfuShmCtx::InitializeShmWriterCtx(std::string shm, std::string log, int level, int stdio)
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


  ShmQueueStatus SfuShmCtx::Enqueue(sfushm_av_frame_frag_t* data, bool isChunkFragment)
  {
    uint64_t ts       = data->rtp_time;
    uint64_t seq      = data->first_rtp_seq;
    bool isChunkStart = data->begin;
    bool isChunkEnd   = data->end;
    
    // If queue is empty, seqid is in order, and pkt is not a fragment, there is no need to enqueue
    if (this->videoPktBuffer.empty()
      && !isChunkFragment
      && (LastSeq(ShmChunkType::VIDEO) == UINT64_UNSET 
        || data->first_rtp_seq - 1 == LastSeq(ShmChunkType::VIDEO)))
    {
      MS_DEBUG_TAG(rtp, "SHM_Q_PKT_CANWRITETHRU seqid=%" PRIu64 "(%" PRIu64 ") ts=%" PRIu64 " lastTs=%" PRIu64 " qsize=%zu",
          data->first_rtp_seq, LastSeq(ShmChunkType::VIDEO), data->rtp_time, LastTs(ShmChunkType::VIDEO), videoPktBuffer.size());
      return ShmQueueStatus::SHM_Q_PKT_CANWRITETHRU;
    }

    // Reject old pkt
    if (LastSeq(ShmChunkType::VIDEO) != UINT64_UNSET
        && LastTs(ShmChunkType::VIDEO) > ts
        && LastTs(ShmChunkType::VIDEO) - ts > MaxVideoPktDelay)
    {
      MS_DEBUG_TAG(rtp, "SHM_Q_PKT_TOO_OLD seqid=%" PRIu64 " ts=%" PRIu64 " lastTs=%" PRIu64 " qsize=%zu",
          data->first_rtp_seq, data->rtp_time, LastTs(ShmChunkType::VIDEO), videoPktBuffer.size());
      return ShmQueueStatus::SHM_Q_PKT_TOO_OLD;
    }
    
    // Enqueue pkt, newest at the end
    auto it = this->videoPktBuffer.begin();
    while (it != this->videoPktBuffer.end())
    {
      if (seq >= it->chunk.first_rtp_seq) // incoming seqId is newer, move further to the end
        ++it;
    }
    it = videoPktBuffer.emplace(it, data, isChunkFragment, isChunkStart, isChunkEnd);
    
    MS_DEBUG_TAG(rtp, "Successully enqueued data seqId=%" PRIu64 "%s%s%s len=%zu ts=%" PRIu64 " lastSeq=%" PRIu64 " lastTs=%" PRIu64 " qsize=%zu",
        seq, isChunkFragment ? " isFragment" : "", isChunkStart ? " chunkStart" : "", isChunkEnd ? " chunkEnd" : "", it->chunk.len,
        ts, LastSeq(ShmChunkType::VIDEO), LastTs(ShmChunkType::VIDEO), videoPktBuffer.size());

    return SHM_Q_PKT_QUEUED_OK;
  }

  
  ShmQueueStatus SfuShmCtx::Dequeue()
  {
    ShmQueueStatus ret = SHM_Q_PKT_DEQUEUED_NOTHING;
    int err;
    
    if (this->videoPktBuffer.empty())
      return SHM_Q_PKT_DEQUEUED_NOTHING;

    // TODO: this algorithm tracks previous seq id in a queue, confirm that it is always monotonous
    std::_List_iterator<DepLibSfuShm::ShmQueueItem> chunkStartIt;

    auto it = this->videoPktBuffer.begin();
    uint64_t prev_seq = it->chunk.first_rtp_seq - 1;
    bool chunkStartFound = false;

    while (it != this->videoPktBuffer.end())
    {
      // First, drop chunks with expired timestamps: last timestamp of a written or skipped packet
      if (LastSeq(ShmChunkType::VIDEO) != UINT64_UNSET
        && LastTs(ShmChunkType::VIDEO) > it->chunk.rtp_time
        && LastTs(ShmChunkType::VIDEO) - it->chunk.rtp_time > MaxVideoPktDelay) // TODO: review ts check and verify that LastTs() is updated properly in each scenario
      {
        MS_DEBUG_TAG(rtp, "dropping pkt seqId=%" PRIu64 " ts=%" PRIu64 " LastTs=%" PRIu64 " qsize=%zu",
            it->chunk.first_rtp_seq, it->chunk.rtp_time, LastTs(ShmChunkType::VIDEO), videoPktBuffer.size());
        prev_seq = it->chunk.first_rtp_seq;
        it = this->videoPktBuffer.erase(it);
        continue;
      }

      // Hole in seqIds, exit and wait for missing pkt to be retransmitted
      if (it->chunk.first_rtp_seq > prev_seq + 1)
      {
        MS_DEBUG_TAG(rtp, "HOLE in seqId: cur=%" PRIu64 " prev=%" PRIu64, it->chunk.first_rtp_seq, prev_seq);
        return SHM_Q_PKT_WAIT_FOR_NACK;
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
            MS_DEBUG_TAG(rtp, "found fragmented chunk but no start yet: [seq=%" PRIu64 " ts=%" PRIu64 "] qsize=%zu",
              it->chunk.first_rtp_seq, it->chunk.rtp_time, videoPktBuffer.size());
            return SHM_Q_PKT_CHUNKSTART_MISSING;
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
              MS_DEBUG_TAG(rtp, "writing fragment seq=%" PRIu64 " [%X%X%X%X]",
                chunkStartIt->chunk.first_rtp_seq,
                chunkStartIt->chunk.data[0],chunkStartIt->chunk.data[1],chunkStartIt->chunk.data[2],chunkStartIt->chunk.data[3]);
              err = this->WriteChunk(&chunkStartIt->chunk, ShmChunkType::VIDEO);
              ret = IsError(err) ? SHM_Q_PKT_SHMWRITE_ERR : SHM_Q_PKT_DEQUEUED_OK;
              chunkStartIt = this->videoPktBuffer.erase(chunkStartIt);
            }
            MS_DEBUG_TAG(rtp, "wrote fragmented chunk! start: [seq=%" PRIu64 " ts=%" PRIu64 "] end: [seq=%" PRIu64 " ts=%" PRIu64 "] qsize=%zu",
              startSeqId, startTs, endSeqId, endTs, videoPktBuffer.size());
            chunkStartFound = false;
            it = nextit; // restore iterator
          }
          else
          {
            MS_DEBUG_TAG(rtp, "skip over mid fragment [seq=%" PRIu64 " ts=%" PRIu64 "] qsize=%zu",
              it->chunk.first_rtp_seq, it->chunk.rtp_time, videoPktBuffer.size());
            prev_seq = it->chunk.first_rtp_seq;
            ++it;
          }
        }
      }
      else // non-fragmented chunk
      {
        err = this->WriteChunk(&it->chunk, ShmChunkType::VIDEO);
        MS_DEBUG_TAG(rtp, "wrote non-fragmented chunk! seqId=%" PRIu64 " ts=%" PRIu64 " qsize=%zu [%X%X%X%X]",
          it->chunk.first_rtp_seq, it->chunk.rtp_time, videoPktBuffer.size(),
          it->chunk.data[0],it->chunk.data[1],it->chunk.data[2],it->chunk.data[3]);
        ret = IsError(err) ? SHM_Q_PKT_SHMWRITE_ERR : SHM_Q_PKT_DEQUEUED_OK;
        prev_seq = it->chunk.first_rtp_seq;
        it = this->videoPktBuffer.erase(it);
        continue;
      }
    }
    return ret;
  }


  int SfuShmCtx::WriteRtpDataToShm(ShmChunkType type, sfushm_av_frame_frag_t* frag, bool isChunkFragment)
  {
    int err;
    ShmQueueStatus st;

    if (type == ShmChunkType::VIDEO)
    {
      st = this->Enqueue(frag, isChunkFragment);      
      if (SHM_Q_PKT_CANWRITETHRU == st) // Write it into shm, no need to queue anything
      {
        MS_DEBUG_TAG(rtp, "wrote whole chunk seqid=%" PRIu64 " ts=%" PRIu64 " [%X%X%X%X]",
          frag->first_rtp_seq, frag->rtp_time, frag->data[0],frag->data[1],frag->data[2],frag->data[3]);

        return this->WriteChunk(frag, ShmChunkType::VIDEO);
      }
      
      if (SHM_Q_PKT_TOO_OLD == st) // Packet is too old, cannot write it
      {
        return SHM_Q_PKT_TOO_OLD;
      }

      st = this->Dequeue();
      if (SHM_Q_PKT_DEQUEUED_OK != st)
      {
        MS_DEBUG_TAG(rtp,"Dequeue() returned state %d", st);
        return (SHM_Q_PKT_SHMWRITE_ERR == st) ? -1 : 0;
      }
    }
    else
    {
      err = this->WriteChunk(frag, ShmChunkType::AUDIO); // do not queue audio
    }

    return err;
  }


  int SfuShmCtx::WriteChunk(sfushm_av_frame_frag_t* data, ShmChunkType kind)
  {
    int err;

    if (Status() != SHM_WRT_READY)
    {
      return 0;
    }

    switch (kind)
    {
      case ShmChunkType::VIDEO:
        err = sfushm_av_write_video(wrt_ctx, data);
        break;

      case ShmChunkType::AUDIO:
        err = sfushm_av_write_audio(wrt_ctx, data);
        break;

      default:
        return -1;
    }

    if (IsError(err))
    {
      // TODO: also log first_rtp_seq, ssrc and timestamp
      MS_WARN_TAG(rtp, "ERROR writing chunk to shm: %d - %s", err, GetErrorString(err));
      return -1;
    }
    return 0;
  }


  int SfuShmCtx::WriteRtcpSenderReportTs(uint64_t lastSenderReportNtpMs, uint32_t lastSenderReporTs, DepLibSfuShm::ShmChunkType kind)
  {
    if (Status() != SHM_WRT_READY)
    {
      return 0;
    }

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

    /*
  .. c:function:: uint64_t uv_hrtime(void)

      Returns the current high-resolution real time. This is expressed in
      nanoseconds. It is relative to an arbitrary time in the past. It is not
      related to the time of day and therefore not subject to clock drift. The
      primary use is for measuring performance between intervals.  */
    auto ntp = Utils::Time::TimeMs2Ntp(lastSenderReportNtpMs);
    auto ntp_sec = ntp.seconds;
    auto ntp_frac = ntp.fractions;
  /*
    Uncomment for debugging
    uint64_t walltime = DepLibUV::GetTimeMs();
    struct timespec clockTime;
    time_t ct;
    if (clock_gettime( CLOCK_REALTIME, &clockTime) == -1) {
      ct = 0;
    }
    ct = clockTime.tv_sec;
    uint64_t clockTimeMs = (clockTime.tv_sec * (uint64_t) 1e9 + clockTime.tv_nsec) / 1000000.0;

    MS_DEBUG_TAG(rtp, "RTCP SR: SSRC=%d ReportNTP(ms)=%" PRIu64 " RtpTs=%" PRIu32 " uv_hrtime(ms)=%" PRIu64 " clock_gettime(s)=%" PRIu64 " clock_gettime(ms)=%" PRIu64,
      ssrc,
      lastSenderReportNtpMs,
      lastSenderReporTs,
      walltime,
      ct,
      clockTimeMs);
  */
    int err = sfushm_av_write_rtcp_sr_ts(wrt_ctx, ntp_sec, ntp_frac, lastSenderReporTs, ssrc);

    if (IsError(err))
    {
      MS_WARN_TAG(rtp, "ERROR writing RTCP SR: %d - %s", err, GetErrorString(err));
      return -1;
    }

    return 0;
  }


  int SfuShmCtx::WriteStreamMeta(std::string metadata, std::string shm)
  {
    int     err;
    uint8_t data[256]; // Currently this is overkill because just 1 byte will be written successfully into shm

    if (0 != this->stream_name.compare(shm))
    {
      MS_WARN_TAG(rtp, "input metadata shm name '%s' does not match '%s'", shm.c_str(), this->stream_name.c_str());
      return -1;
    }

    if (metadata.length() > 255)
    {
      MS_WARN_TAG(rtp, "input metadata is too long: %s", metadata.c_str());
    }
    std::copy(metadata.begin(), metadata.end(), data);

    err = sfushm_av_write_stream_metadata(wrt_ctx, data, metadata.length());

    if (IsError(err))
    {
      MS_WARN_TAG(rtp, "ERROR writing stream metadata: %d - %s", err, GetErrorString(err));
      return -1;
    }

    return 0;
  }


  int SfuShmCtx::WriteVideoOrientation(uint16_t rotation)
  {
    int err = sfushm_av_write_video_rotation(wrt_ctx, rotation);

    if (IsError(err))
    {
      MS_WARN_TAG(rtp, "ERROR writing video rotation: %d - %s", err, GetErrorString(err));
      return -1;
    }

    return 0;
  }
} // namespace DepLibSfuShm