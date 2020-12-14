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

  uint8_t* ShmCtx::hex_dump(uint8_t *dst, uint8_t *src, size_t len)
  {
      static uint8_t hx[] = "0123456789abcdef";

      while (len--) {
          *dst++ = hx[*src >> 4];
          *dst++ = hx[*src++ & 0xf];
      }

      return dst;
  }

  std::unordered_map<int, const char*> ShmCtx::errToString =
    {
      { 0, "success (SFUSHM_AV_OK)"   },
      { -1, "error (SFUSHM_AV_ERR)"   },
      { -2, "again (SFUSHM_AV_AGAIN)" }
    };

  // 100ms in samples (90000 sample rate) 
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
    
    // TODO: if needed, target_kbps may be passed as config param instead
    // and codec_id, sample_rate may be read from ShmConsumer in the same way as in ShmConsumer::CreateRtpStream()
    wrt_init.conf.channels[0].target_buf_ms = 20000;
    wrt_init.conf.channels[0].target_kbps   = 128;
    wrt_init.conf.channels[0].ssrc          = Utils::Crypto::GetRandomUInt(0,  0xFFFFFFFF);
    wrt_init.conf.channels[0].sample_rate   = 48000;
    wrt_init.conf.channels[0].num_chn       = 2;
    wrt_init.conf.channels[0].codec_id      = SFUSHM_AV_AUDIO_CODEC_OPUS;
    wrt_init.conf.channels[0].video         = 0; 
    wrt_init.conf.channels[0].audio         = 1;

    wrt_init.conf.channels[1].target_buf_ms = 20000; 
    wrt_init.conf.channels[1].target_kbps   = 2500;
    wrt_init.conf.channels[1].ssrc          = Utils::Crypto::GetRandomUInt(0,  0xFFFFFFFF);
    wrt_init.conf.channels[1].sample_rate   = 90000;
    wrt_init.conf.channels[1].codec_id      = SFUSHM_AV_VIDEO_CODEC_H264;
    wrt_init.conf.channels[1].video         = 1;
    wrt_init.conf.channels[1].audio         = 0;

    this->media[0].new_ssrc =  wrt_init.conf.channels[0].ssrc;
    this->media[1].new_ssrc =  wrt_init.conf.channels[1].ssrc;

    MS_DEBUG_TAG(xcode, "shm writer ssrc mapping: audio[%" PRIu32 "] video[%" PRIu32 "]",
      this->media[0].new_ssrc, this->media[1].new_ssrc);

    int err = SFUSHM_AV_OK;
    if ((err = sfushm_av_open_writer( &wrt_init, &wrt_ctx)) != SFUSHM_AV_OK) {
      MS_DEBUG_TAG(xcode, "FAILED in sfushm_av_open_writer() to initialize sfu shm %s with error %s", this->wrt_init.stream_name, GetErrorString(err));
      this->wrt_status = SHM_WRT_UNDEFINED;
    }
    else
    { 
      this->wrt_status = SHM_WRT_READY;

      // Write any stored sender reports into shm and possibly notify ShmConsumer(s)
      if (this->media[0].sr_received)
      {
        WriteSR(Media::AUDIO);
        this->media[0].sr_written = true;
        MS_DEBUG_TAG(xcode, "shm is ready and first audio SR received and written");
      }
      if (this->media[1].sr_received)
      {
        WriteSR(Media::VIDEO);
        this->media[1].sr_written = true;
        MS_DEBUG_TAG(xcode, "shm is ready and first video SR received and written");
        this->listener->OnNeedToSync();
      }
    }
  }


  ShmQueueItem::ShmQueueItem(uint8_t* dataptr, size_t datalen, uint64_t seq, uint64_t timestamp, bool isfragment, bool isfirstfrag, bool isbeginpicture, bool isendpicture, bool iskeyframe)
    : len(datalen), seqid(seq), ts(timestamp), fragment(isfragment), firstfragment(isfirstfrag), beginpicture(isbeginpicture), endpicture(isendpicture), keyframe(iskeyframe)
  {
    //space for Annex B header, will grab 3 or 4 bytes
    this->store[0] = 0x00;
    this->store[1] = 0x00;
    this->store[2] = 0x00;
    this->store[3] = 0x01;
    
    this->data = this->store + 4;
    std::memcpy(this->data, dataptr, len);
  }


  // seqId: strictly +1 unless NALUs came from STAP-A packet, then they are the same
  // timestamps: should increment for each new picture (shm chunk), otherwise they match
  // Can see two fragments coming in with the same seqId and timestamps: SPS and PPS from the same STAP-A packet are typical
  EnqueueResult ShmCtx::Enqueue(
    uint8_t *data, 
    size_t len,
    uint64_t seqid,
    uint64_t ts,
    bool isfragment,
    bool isfirstfrag,
    bool isbeginpicture,
    bool isendpicture,
    bool iskeyframe)
  {
    MS_TRACE();

    // If queue is empty, seqid is in order, and pkt is not a fragment, there is no need to enqueue
    if (this->videoPktBuffer.empty()
      && !isfragment
      && (IsLastVideoSeqNotSet() 
        || seqid - 1 == LastVideoSeq()))
    {
      MS_DEBUG_TAG(xcode, "WRITETHRU [seq=%" PRIu64 " ts=%" PRIu64 " len=%zu]", seqid, ts, len);
      return EnqueueResult::SHM_Q_PKT_CANWRITETHRU;
    }

    // Add a too old pkt in queue anyway TBD
    if (!IsLastVideoSeqNotSet()
        && LastVideoTs() > ts
        && LastVideoTs() - ts > MaxVideoPktDelay)
    {
      MS_DEBUG_TAG(xcode, "ENQUEUE OLD seqid=%" PRIu64 " delta=%" PRIu64 " lastTs=%" PRIu64 " qsize=%zu", 
        seqid, LastVideoTs() - ts, LastVideoTs(), videoPktBuffer.size());
    }
    
    // Enqueue pkt, newest at the end.
    // TODO: keep stats of how many frames had been fixed with retransmission
    auto it = this->videoPktBuffer.begin();
    for (; it != this->videoPktBuffer.end() && seqid >= it->seqid; it++);

    it = this->videoPktBuffer.emplace(it, data, len, seqid, ts, isfragment, isfirstfrag, isbeginpicture, isendpicture, iskeyframe);
    if (iskeyframe && ts > this->lastKeyFrameTs)
    {
      this->lastKeyFrameTs = ts;
    }

    return SHM_Q_PKT_QUEUED_OK;
  }


/*
  void ShmCtx::Dequeue2()
  {
    MS_TRACE();
    
    if (this->videoPktBuffer.empty())
      return;

    // First, check seqid gap (unless first item in a queue): if current ts is NOT old then return from Dequeue(), otherwise will continue;
    // Next, check for ts increment(or first item in queue): this is the start of new chunk.
    // If needed, write out previously accumulated chunk fragments (those will be invalid because end was not found...)
    // Else if ts did not change, check for beginFragment and endFragment markers.
    //
    // Also need to keep track of seqIf gaps inside a fragmented chunk - if it old and we are writing it out anyway.
    // Need to know if pic ended up as invalid or just sat in a queue blocked by previous pics waiting for RTX, becase we request a KF if we found an invalid one.

    std::list<ShmQueueItem>::iterator firstfragIt, lastfragIt; // first and last frags of chunk, regardless of whether they actually have data markers;
    bool foundBeginFragMarker, foundEndFragMarker;
    auto it = this->videoPktBuffer.begin();
    uint64_t prevseqid = it->seqid - 1; // fake to get the loop started
    uint64_t currentFragTs = it->ts;

  }*/

  
  void ShmCtx::Dequeue()
  {
    MS_TRACE();

    if (this->videoPktBuffer.empty())
      return;

    // seqId: with no pkts lost, strictly +1 unless NALUs came from STAP-A packet
    // timestamps: should increment for each new picture (shm chunk), otherwise they match
    // Can see two fragments coming in with the same seqId and timestamps: SPS and PPS from the same STAP-A packet are typical
    std::list<ShmQueueItem>::iterator firstfragIt;

    auto it = this->videoPktBuffer.begin();
    uint64_t prevseqid = it->seqid - 1;
    bool beginFound = false;

    while (it != this->videoPktBuffer.end())
    {
      // First, write out all chunks with expired timestamps
      if (!IsLastVideoSeqNotSet()
        && LastVideoTs() > it->ts
        && LastVideoTs() - it->ts > MaxVideoPktDelay)
      {
        // TODO: keep stats of invalid packets (better yet, invalid frames)
        // BUGBUG Just because this is old chunk and fragmented it does not mean it's invalid. It may be a complete pic waiting in queue and blocked by incomplete pics.
        bool invalid = it->fragment;
        prevseqid = it->seqid;
        ShmQueueItem& item = *it;
        MS_DEBUG_TAG(xcode, "Old chunk [seq=%" PRIu64 " Tsdelta=%" PRIu64 "] lastTs=%" PRIu64 " qsize=%zu", it->seqid, LastVideoTs() - it->ts, LastVideoTs(), videoPktBuffer.size());
        this->WriteVideoChunk(&item, false); //, invalid); Temporarily do not set "invalid" flag
        it = this->videoPktBuffer.erase(it);

        // Temp solution: if there is no other KF in the queue assume we should request a KF
        /* This check will not work until we reliably can tell that pic is invalid
        if (invalid && it->keyframe && this->lastKeyFrameTs == UINT64_UNSET)
        {
          this->listener->OnNeedToSync();
        }*/
        continue;
      }

      // Hole in seqIds: will wait for some time for missing pkt to be retransmitted
      if (it->seqid > prevseqid + 1)
      {
        MS_DEBUG_TAG(xcode, "Seqid gap [seq=%" PRIu64 " ts=%" PRIu64 "] prev=%" PRIu64 " lastTs=%" PRIu64 " qsize=%zu",
          it->seqid, it->ts, prevseqid, LastVideoTs(), videoPktBuffer.size());
        return;
      }

      // Fragment of a chunk: start, end or middle
      if (it->fragment)
      {
        if (it->firstfragment)
        {
          beginFound = true;
          firstfragIt = it;
          prevseqid = it->seqid;
          ++it;
        }
        else // mid or end
        {
          if (!beginFound)
          {
            // chunk incomplete, wait for retransmission
            MS_DEBUG_TAG(xcode, "Miss begin [seq=%" PRIu64 " ts=%" PRIu64 "] qsize=%zu",
              it->seqid, it->ts, videoPktBuffer.size());
            return;
          }

          // end, found, write the whole chunk
          if (it->endpicture)
          {
            auto nextIt = it;
            uint64_t beginseqid = firstfragIt->seqid;
            uint64_t begints = firstfragIt->ts;
            uint64_t endseqid = it->seqid;
            uint64_t endts = it->ts;
            prevseqid = it->seqid;

            nextIt++; // position past last fragment in a chunk
            while (firstfragIt != nextIt)
            {
              ShmQueueItem& item = *firstfragIt;
              this->WriteVideoChunk(&item, false);
              firstfragIt = this->videoPktBuffer.erase(firstfragIt);
            }

            MS_DEBUG_TAG(xcode, "FU begin [seq=%" PRIu64 " ts=%" PRIu64 "] end [seq=%" PRIu64 " ts=%" PRIu64 "] qsize=%zu",
              beginseqid, begints, endseqid, endts, videoPktBuffer.size());
            beginFound = false;
            prevseqid = endseqid;
            it = nextIt; // restore iterator
          }
          else
          {
            prevseqid = it->seqid;
            ++it;
          }
        }
      }
      else // non-fragmented chunk
      {
        ShmQueueItem& item = *it;

        MS_DEBUG_TAG(xcode, "NALU [seq=%" PRIu64 " ts=%" PRIu64 " len=%zu] lastTs=%" PRIu64 " qsize=%zu",
          item.seqid, item.ts, item.len, LastVideoTs(), videoPktBuffer.size());

        this->WriteVideoChunk(&item, false);
        prevseqid = it->seqid;
        it = this->videoPktBuffer.erase(it);
        continue;
      }
    }
  }

  void ShmCtx::WriteAudioRtpDataToShm(uint8_t *data, size_t len, uint64_t seq, uint64_t ts)
  {
    MS_TRACE();

    sfushm_av_frame_frag_t frag;
    frag.ssrc = media[0].new_ssrc;
    frag.data = data;
    frag.len = len;
    frag.rtp_time = ts;
    frag.first_rtp_seq = frag.last_rtp_seq = seq;
    frag.begin = frag.end = 1;

    this->WriteAudioChunk(&frag);
  }


  void ShmCtx::WriteAudioChunk(sfushm_av_frame_frag_t* data)
  {
    MS_TRACE();

    if (!this->CanWrite(Media::AUDIO))
    {
      MS_WARN_TAG(xcode, "Cannot write audio ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " because shm writer is not initialized",
        data->ssrc, data->first_rtp_seq, data->rtp_time);
      return;
    }

    int err = sfushm_av_write_audio(wrt_ctx, data);
    if (IsError(err))
      MS_WARN_TAG(xcode, "ERROR writing audio ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " inv=%d to shm: %d - %s", data->ssrc, data->first_rtp_seq, data->rtp_time, data->invalid, err, GetErrorString(err));
  }

  void ShmCtx::WriteVideoRtpDataToShm(
    uint8_t *data, 
    size_t len,
    uint64_t seqid,
    uint64_t ts,
    bool isfragment,
    bool isfirstfragment,
    bool ispicturebegin,
    bool ispictureend,
    bool iskeyframe)
  {
    MS_TRACE();

    if (SHM_Q_PKT_CANWRITETHRU == this->Enqueue(data, 
                                                len,
                                                seqid,
                                                ts,
                                                isfragment,
                                                isfirstfragment,
                                                ispicturebegin,
                                                ispictureend,
                                                iskeyframe))
    {
      // Write it into shm, no need to queue anything
      ShmQueueItem item; // Default ctor to avoid data copying and leave item.store buffer uninitialized
      item.len = len;
      item.data = data;
      item.ts = ts;
      item.seqid = seqid;
      item.fragment = isfragment; //  btw, we never write thru chunk fragments
      item.firstfragment = isfirstfragment;
      item.beginpicture = ispicturebegin;
      item.endpicture = ispictureend; 
      item.keyframe = iskeyframe;

      this->WriteVideoChunk(&item, true);
    }

    this->Dequeue();
  }
  

  void ShmCtx::WriteVideoChunk(ShmQueueItem* item, bool writethru, bool invalid)
  {
    MS_TRACE();

    if (item->keyframe && item->ts == this->lastKeyFrameTs)
    {
      // we are writing the last key frame out from a buffer queue
      this->lastKeyFrameTs = UINT64_UNSET;
    }

    if (!this->CanWrite(Media::VIDEO))
    {
      MS_WARN_TAG(xcode, "Cannot write video ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " because shm writer is not initialized",
        media[1].new_ssrc, item->seqid, item->ts);
      return;
    }

    sfushm_av_frame_frag_t frag;
    frag.ssrc = media[1].new_ssrc;
    frag.first_rtp_seq = frag.last_rtp_seq = item->seqid;
    frag.rtp_time = item->ts;
    frag.invalid = invalid;
    frag.begin = item->fragment ? (item->firstfragment && item->beginpicture) : item->beginpicture;
    frag.end = item->endpicture; // if this is ending fragment, it is always also end of picture

    uint8_t backup[4]; // backup for 3 or 4 temporarily overwritten bytes

    enum AnnexB header = AnnexB::NO_HEADER;
    // If whole NALU then depends on whether this is beginning of a pic i.e. RTP pkt timestamp has been incremented
    if (!item->fragment) 
    {
      header = item->beginpicture ? AnnexB::LONG_HEADER : AnnexB::SHORT_HEADER;
    }
    // If a fragment then only add AnnexB header to the first fragment depending on whether it is at the beginning of a pic, otherwise no header needed
    else if (item->firstfragment)
    {
      header = item->beginpicture ? AnnexB::LONG_HEADER : AnnexB::SHORT_HEADER;
    }

    // take care of AnnexB header selection, 3 or 4 bytes
    if (writethru)
    {
      // Use item->data ptr which directly points to RTP packet data, not item->store
      // Previous 3 or 4 bytes (RTP packet header extension) will be temporarily overwritten and then restored
      std::memcpy(&(backup[0]), &(item->data[-4]), sizeof(uint8_t) * 4);

      switch(header)
      {
        case LONG_HEADER:
          frag.data = &(item->data[-4]);
          frag.data[0] = 0x00;
          frag.data[1] = 0x00;
          frag.data[2] = 0x00;
          frag.data[3] = 0x01;
          frag.len = item->len + 4;
          break;

        case SHORT_HEADER:
          frag.data = &(item->data[-3]);
          frag.data[0] = 0x00;
          frag.data[1] = 0x00;
          frag.data[2] = 0x01;
          frag.len = item->len + 3;
          break;

        default:
          // writethru items are never fragments, always have AnnexB header
          break;
      }
    }
    else
    {
      // AnnexB header placed in the first 4 bytes of item->store by ShmQueueItem::ctor, and item->data points to item->store's 5th byte
      switch(header)
      {
        case LONG_HEADER:
          frag.data = &(item->store[0]);
          frag.len = item->len + 4;
          break;

        case SHORT_HEADER:
          frag.data = &(item->store[1]);
          frag.len = item->len + 3;
          break;

        default:
          frag.data = item->data;
          frag.len = item->len;
          break;
      }
    }
    
    int err = sfushm_av_write_video(wrt_ctx, &frag);
    if (IsError(err))
    {
      MS_WARN_TAG(xcode, "ERROR writing chunk ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 "len=%zu inv=%d: %d - %s",
        frag.ssrc, frag.first_rtp_seq, frag.rtp_time, frag.len, frag.invalid, err, GetErrorString(err));
    }
    /*else
    {
      uint8_t dump[100];
      memset(dump, 0, sizeof(dump));
      ShmCtx::hex_dump(dump, frag.data, frag.len < 20 ? frag.len : 20);

      MS_DEBUG_TAG(xcode, "OK ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " begin=%d end=%d len=%zu [%s]",
        frag.ssrc, frag.first_rtp_seq, frag.rtp_time, frag.begin, frag.end, frag.len, dump);
    }*/

    // restore temp overwritten data if not using ShmQueueItem::store
    if (writethru)
    {
      std::memcpy(&(item->data[-4]), &(backup[0]), sizeof(uint8_t) * 4);
    }
  }


  void ShmCtx::WriteRtcpSenderReportTs(uint64_t lastSenderReportNtpMs, uint32_t lastSenderReportTs, Media kind)
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

    this->media[idx].sr_received = true;
    this->media[idx].sr_ntp_msb = ntp.seconds;
    this->media[idx].sr_ntp_lsb = ntp.fractions;
    this->media[idx].sr_rtp_tm = lastSenderReportTs;

    MS_DEBUG_TAG(xcode, "Received SR: SSRC=%" PRIu32 " ReportNTP(ms)=%" PRIu64 " RtpTs=%" PRIu32 " uv_hrtime(ms)=%" PRIu64 " clock_gettime(s)=%" PRIu64 " clock_gettime(ms)=%" PRIu64,
      this->media[idx].new_ssrc,
      lastSenderReportNtpMs,
      lastSenderReportTs,
      walltime,
      ct,
      clockTimeMs);

    if (SHM_WRT_READY != this->wrt_status)
      return; // shm writer is not set up yet

    WriteSR(kind);

    if (this->CanWrite(kind))
      return; // already submitted the first SRs into shm, and it is in ready state

    this->media[idx].sr_written = true;
    if (kind == Media::VIDEO)
      this->listener->OnNeedToSync();
  }


  void ShmCtx::WriteSR(Media kind)
  {
    MS_TRACE();

    size_t idx = (kind == Media::AUDIO) ? 0 : 1;
    int err = sfushm_av_write_rtcp_sr_ts(wrt_ctx, this->media[idx].sr_ntp_msb, this->media[idx].sr_ntp_lsb, this->media[idx].sr_rtp_tm, this->media[idx].new_ssrc);
    if (IsError(err))
    {
      MS_WARN_TAG(xcode, "ERROR writing RTCP SR (ssrc:%" PRIu32 ") %d - %s", this->media[idx].new_ssrc, err, GetErrorString(err));
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