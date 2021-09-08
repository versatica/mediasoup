#define MS_CLASS "DepLibSfuShm"
// #define MS_LOG_DEV

#include "DepLibSfuShm.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "DepLibUV.hpp"
#include <time.h>
#include <chrono>


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

  xcode_sfushm_bin_log_ctx_t ShmCtx::bin_log_ctx;

  std::unordered_map<int, const char*> ShmCtx::errToString =
    {
      { 0, "success (SFUSHM_AV_OK)"   },
      { -1, "error (SFUSHM_AV_ERR)"   },
      { -2, "again (SFUSHM_AV_AGAIN)" }
    };

  // 100ms in samples (90000 sample rate) 
  //static constexpr uint64_t MaxVideoPktDelay{ 9000 };


  ShmCtx::ShmCtx(): wrt_ctx(nullptr), wrt_status(SHM_WRT_UNDEFINED)
  {
    memset(&bin_log_ctx, 0, sizeof(bin_log_ctx));
  }


  ShmCtx::~ShmCtx()
  {
    MS_TRACE();
    
    CloseShmWriterCtx();
  }


  void ShmCtx::CloseShmWriterCtx()
  {
    MS_TRACE();

    MS_DEBUG_TAG(xcode, "shm[%s] writer will be closed", this->stream_name.c_str());

    // Call if writer is not closed
    if (SHM_WRT_READY == wrt_status)
    {
      if ( wrt_ctx == nullptr)
        MS_WARN_TAG(xcode, "shm[%s] warning: writer ctx is null but shm is in ready state", this->stream_name.c_str());
      else
      {
        MS_DEBUG_TAG(xcode, "shm[%s] writer closing", wrt_init.stream_name);
        sfushm_av_close_writer(wrt_ctx, 0);
      }
    }

    wrt_status = SHM_WRT_CLOSED;
    memset( &wrt_init, 0, sizeof(sfushm_av_writer_init_t));
  }


  void ShmCtx::InitializeShmWriterCtx(std::string shm, int queueAge, bool useReverse, int testNack, std::string log, int level)
  {
    MS_TRACE();

    memset( &wrt_init, 0, sizeof(sfushm_av_writer_init_t));

    stream_name.assign(shm);
    log_name.assign(log);

    maxVideoPktDelay = uint64_t(queueAge * 90); // queueAge in ms * 90,000 samples per sec / 1,000ms
    testNackEachMs = uint64_t(testNack);
    useReverseIterator = useReverse;

    wrt_init.stream_name = const_cast<char*>(stream_name.c_str());
    wrt_init.stats_win_size = 300;
    wrt_init.conf.log_file_name = log_name.c_str();
    wrt_init.conf.log_level = level;
    wrt_init.conf.redirect_stdio = false;
    
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

    MS_DEBUG_TAG(xcode, "shm[%s] ctx ssrc mapping: audio[%" PRIu32 "] video[%" PRIu32 "]",
      this->stream_name.c_str(),
      this->media[0].new_ssrc,
      this->media[1].new_ssrc);
    MS_DEBUG_DEV("shm[%s] ctx maxVideoPktDelay=%" PRIu64 " testNackEachMs=%" PRIu64 " useReverseIterator=%s",
      this->stream_name.c_str(),
      maxVideoPktDelay, 
      testNackEachMs, 
      useReverseIterator ? "true" : "false");

    int err = SFUSHM_AV_OK;
    if ((err = sfushm_av_open_writer( &wrt_init, &wrt_ctx)) != SFUSHM_AV_OK) {
      MS_WARN_TAG(xcode, "shm[%s] failed sfushm_av_open_writer() error='%s'", 
        this->wrt_init.stream_name, 
        GetErrorString(err));
      this->wrt_status = SHM_WRT_UNDEFINED;
    }
    else
    { 
      this->wrt_status = SHM_WRT_READY;
      MS_DEBUG_TAG(xcode, "shm[%s] writer ready", this->stream_name.c_str());

      // Write any stored sender reports into shm and possibly notify ShmConsumer(s)
      if (this->media[0].sr_received)
      {
        WriteSR(Media::AUDIO);
        this->media[0].sr_written = true;
        MS_DEBUG_TAG(xcode, "shm[%s] first audio SR received and written",
          this->stream_name.c_str());
      }
      if (this->media[1].sr_received)
      {
        WriteSR(Media::VIDEO);
        this->media[1].sr_written = true;
        MS_DEBUG_TAG(xcode, "shm[%s] first video SR received and written",
          this->stream_name.c_str());
        this->listener->OnNeedToSync();
      }
    }
  }

  
  // Use this to continue writing into the same shm when the actual shm consumer has been switched
  void ShmCtx::ResetShmMediaStatsAndQueue(Media kind)
  {
    MS_TRACE();

    if (kind == Media::AUDIO)
    {
      this->media[0].rtp_seq_offset = (this->media[0].last_rtp_seq == UINT64_UNSET) ? 0 : this->media[0].last_rtp_seq;
      this->media[0].sr_written = false;
      this->media[0].sr_received = false;
    }
    else // video
    {
      // drain video frames queue
      if (videoPktBuffer.size() > 0)
      {
        MS_DEBUG_TAG(xcode, "shm[%s] start draining video buffer qsize=%zu", this->stream_name.c_str(), videoPktBuffer.size());

        this->Dequeue();

        // remove whatever might have still be in the list
        this->videoPktBuffer.clear();
      }
      this->media[1].rtp_seq_offset = (this->media[1].last_rtp_seq == UINT64_UNSET) ? 0 : this->media[1].last_rtp_seq;
      this->media[1].sr_written = false;
      this->media[1].sr_received = false;

      MS_DEBUG_TAG(xcode, "shm[%s] new video seqid offset %" PRIu64, this->stream_name.c_str(), this->media[1].rtp_seq_offset);
    }

    ShmCtx::UpdateRtpStats(UINT64_UNSET, UINT64_UNSET, kind);
  }


  ShmQueueItem::ShmQueueItem(uint8_t* dataptr, size_t datalen, uint64_t seq, uint64_t timestamp, uint8_t nalu, bool isfragment, bool isfirstfrag, bool isendfrag, bool isbeginpicture, bool isendpicture, bool iskeyframe)
    : len(datalen), seqid(seq), ts(timestamp), nal(nalu), fragment(isfragment), firstfragment(isfirstfrag), endfragment(isendfrag), beginpicture(isbeginpicture), endpicture(isendpicture), keyframe(iskeyframe)
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
    uint8_t nal,
    bool isfragment,
    bool isfirstfrag,
    bool isendfrag,
    bool isbeginpicture,
    bool isendpicture,
    bool iskeyframe)
  {
    MS_TRACE();
    
    if (iskeyframe && (ts > this->lastKeyFrameTs || this->lastKeyFrameTs == UINT64_UNSET))
    {
      this->lastKeyFrameTs = ts;
      bin_log_record.v_last_kf_rtp_time = this->lastKeyFrameTs;
    }

    //auto startTime = std::chrono::system_clock::now();

    if (useReverseIterator)
    {
      // If the first item into empty queue, or should be placed at the end
      if (this->videoPktBuffer.empty() || seqid >= this->videoPktBuffer.rbegin()->seqid)
      {
        this->videoPktBuffer.emplace(this->videoPktBuffer.end(), data, len, seqid, ts, nal, isfragment, isfirstfrag, isendfrag, isbeginpicture, isendpicture, iskeyframe);
        /* TODO: replace with sliding average and maybe deviation
        auto endTime = std::chrono::system_clock::now();
        bin_log_record.v_last_enqueue_dur = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
        bin_log_record.v_last_queue_len = this->videoPktBuffer.size(); */
        return SHM_Q_PKT_QUEUED_OK;
      }

      auto it = this->videoPktBuffer.rbegin();
      auto prevIt = it;
      it++;
      for (; it != this->videoPktBuffer.rend() && seqid < it->seqid; it++, prevIt++) {}
      this->videoPktBuffer.emplace(prevIt.base(), data, len, seqid, ts, nal, isfragment, isfirstfrag, isendfrag, isbeginpicture, isendpicture, iskeyframe);
      /* TODO: replace with sliding average
      auto endTime2 = std::chrono::system_clock::now();
      bin_log_record.v_last_enqueue_dur = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime2 - startTime).count();
      bin_log_record.v_last_queue_len = this->videoPktBuffer.size();
      bin_log_record.v_enqueue_out_of_order += 1; */
    }
    else
    {
      // Enqueue pkt, newest at the end.
      auto it = this->videoPktBuffer.begin();
      for (; it != this->videoPktBuffer.end() && seqid >= it->seqid; it++) {}

      if (it != this->videoPktBuffer.end())
      {
        //MS_DEBUG_TAG(xcode, "out-of-order [ %" PRIu64 " ]", seqid);
        bin_log_record.v_enqueue_out_of_order += 1;
      }

      this->videoPktBuffer.emplace(it, data, len, seqid, ts, nal, isfragment, isfirstfrag, isendfrag, isbeginpicture, isendpicture, iskeyframe);

      /* TODO: replace with sliding average
      auto endTime = std::chrono::system_clock::now();
      bin_log_record.v_last_enqueue_dur = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
      bin_log_record.v_last_queue_len = this->videoPktBuffer.size();
      */
    }

    return SHM_Q_PKT_QUEUED_OK;
  }


  void ShmCtx::Dequeue()
  {
    MS_TRACE();
    
    if (this->videoPktBuffer.empty())
      return;

    bool gaps = false; // picture frame has missing chunks

    std::list<ShmQueueItem>::iterator firstIt, lastIt;
    firstIt = lastIt = this->videoPktBuffer.begin();

    // See if a frame is ready to be written i.e. either it has expired, or all chunks are present.
    while(GetNextFrame(firstIt, lastIt, gaps))
    {
      MS_DEBUG_TAG(xcode, "shm[%s] got frame [%" PRIu64 " - %" PRIu64 "](%" PRIu64 ") ts=%" PRIu64 " gaps=%d qsize=%zu", 
        this->stream_name.c_str(), firstIt->seqid, lastIt->seqid, this->media[1].rtp_seq_offset, firstIt->ts, gaps, videoPktBuffer.size());
      WriteFrame(firstIt, lastIt, gaps);
    }
  }


  bool ShmCtx::GetNextFrame(std::list<ShmQueueItem>::iterator& firstIt, 
                            std::list<ShmQueueItem>::iterator& lastIt,
                            bool& gaps)
  {
    MS_TRACE();
    
    if (this->videoPktBuffer.empty())
      return false;

    auto it = firstIt = lastIt = this->videoPktBuffer.begin();

    uint64_t prevseqid = it->seqid - 1;
    uint64_t frameTs = it->ts;
    uint64_t lastItemTs = this->videoPktBuffer.rbegin()->ts;
    bool frameTsExpired = (lastItemTs > frameTs) && (lastItemTs - frameTs > this->maxVideoPktDelay);
    bool keyframe = false;

    while(it != this->videoPktBuffer.end() && frameTs == it->ts)
    {
      if (it->keyframe)
        keyframe = true;
      
      if (it->seqid > prevseqid + 1)
      {
        MS_DEBUG_TAG(xcode, "shm[%s] gap [seq=%" PRIu64 " prev=%" PRIu64 " ts=%" PRIu64 " expired=%d] qsize=%zu",
          this->stream_name.c_str(), it->seqid, prevseqid, frameTs, frameTsExpired, videoPktBuffer.size());
        if (!frameTsExpired)
        {
          return false; // have time, let's wait for more pkts
        }
        else
        {
          gaps = true;
        }
      }

      prevseqid = it->seqid;
      lastIt = it;
      ++it;
    } // while

    // Missing pic end: wait if we can
    if(!lastIt->endpicture)
    {
      if (!frameTsExpired)
        return false; // can wait
      else
        gaps = true;
    }

    // Missing pic begin: wait, then try to repair, ShmConsumer cannot correctly label out of order pkts
    if (!firstIt->beginpicture)
    {
      if (!frameTsExpired)
      {
        return false; // can wait
      }
      else
      {
        if (firstIt->fragment && firstIt->firstfragment)
        {
          // a pic fragment is likely a pic begin if its start bit is set, and it is the first in the queue
          firstIt->beginpicture = true;
          MS_DEBUG_TAG(xcode, "shm[%s] beginpic fixed [seq=%" PRIu64 " ts=%" PRIu64 " nal=%" PRIu8 "]",
            this->stream_name.c_str(), firstIt->seqid, frameTs, firstIt->nal);
        }
        else if (keyframe)
        {
          // special guess for kf: pic begin can be SPS or PPS, they are usually not fragmented
          if (firstIt->nal == 7 || firstIt->nal == 8)
          {
            firstIt->beginpicture = true;
            MS_DEBUG_TAG(xcode, "shm[%s] kf beginpic fixed [seq=%" PRIu64 " ts=%" PRIu64 " nal=%" PRIu8 "]",
              this->stream_name.c_str(), firstIt->seqid, frameTs, firstIt->nal);
          }
        }
        else
        {
          gaps = true;
        }
      }
    }
    
    return (!gaps || frameTsExpired); // tell the caller if the frame can be written out, otherwise caller will wait
  }


  void ShmCtx::WriteFrame(std::list<ShmQueueItem>::iterator& firstIt, 
                          std::list<ShmQueueItem>::iterator& lastIt,
                          bool invalid)
  {
    MS_TRACE();

    if (this->videoPktBuffer.empty())
      return;

    bool keyframe = false; // while writing, see if any chunk has keyframe marking
    uint64_t frameTs = firstIt->ts;

    auto chunkIt = firstIt;
    while(chunkIt != this->videoPktBuffer.end())
    {
      if (chunkIt->keyframe)
        keyframe = true;
      
      ShmQueueItem &item = *chunkIt;
      this->WriteVideoChunk(&item, invalid); // TODO: shm writer should ack explicit "invalid" setting and not throw errors
      
      bool end = (chunkIt == lastIt); // before erasing an item check if it is the last one in the frame
      chunkIt = this->videoPktBuffer.erase(chunkIt); //move to next item
      if (end)
        break;
    }

    if (keyframe && frameTs == this->lastKeyFrameTs)
    {
      // we dequeued the last key frame
      this->lastKeyFrameTs = UINT64_UNSET;

      // If the last KF written into shm was incomplete, we can request a new one right away
      // TBD: should we also discard everything in videoPktBuffer and reset lastTs and lastSeqId while waiting for another keyframe?
      if (invalid)
      {
        MS_WARN_TAG(xcode, "shm[%s] invalid kf, request another", this->stream_name.c_str());
        this->listener->OnNeedToSync();
      }
    }
  }


  // Will write into the log, rotate... Need signal_set and current data
  void ShmCtx::DumpBinLogDataIfNeeded(bool signal_set)
  {
    MS_TRACE();
    /* 
    put into sfushm_bin_log.c
    
    uint64_t cur_tm;

    // assuming time update was called prior to this call
    cur_tm = ffngxshm_get_cur_timestamp();

    // check if it is time to write the current record
    if(bin_log_record.start_tm + bin_log_rec_intervals < cur_tm)
    {
        bin_log_record.epoch_len = (uint16_t)(cur_tm - bin_log_record.start_tm);

    }
    */
    int ret = sfushm_bin_log_rotate(&bin_log_ctx, bin_log_rec_intervals, signal_set);
    if (0 != ret)
    {
      MS_ERROR("failed to dump bin record. err='%s'", strerror(ret));
      // sfushm API should close file and zero out fd
    }
  }

  void ShmCtx::WriteAudioRtpDataToShm(uint8_t *data, size_t len, uint64_t seq, uint64_t ts)
  {
    MS_TRACE();

    // see if need to dump data into binary log
    //DumpBinLogDataIfNeeded();

    sfushm_av_frame_frag_t frag;
    frag.ssrc = media[0].new_ssrc;
    frag.data = data;
    frag.len = len;
    frag.rtp_time = ts;
    frag.first_rtp_seq = frag.last_rtp_seq = seq + media[0].rtp_seq_offset;
    frag.begin = frag.end = 1;

    this->WriteAudioChunk(&frag);
  }


  void ShmCtx::WriteAudioChunk(sfushm_av_frame_frag_t* data)
  {
    MS_TRACE();

    if (!this->CanWrite(Media::AUDIO))
    {
      MS_WARN_TAG(xcode, "shm[%s] cannot write audio ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " because writer is not initialized",
        this->stream_name.c_str(), data->ssrc, data->first_rtp_seq, data->rtp_time);
      return;
    }

    bin_log_record.a_egress_bytes += data->len;
    bin_log_record.a_last_rtp_time = data->rtp_time;
    bin_log_record.a_num_wr += 1;

    int err = sfushm_av_write_audio(wrt_ctx, data);
    if (IsError(err))
    {
      MS_WARN_TAG(xcode, "shm[%s] error writing audio ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " inv=%d: %d - %s", this->stream_name.c_str(), data->ssrc, data->first_rtp_seq, data->rtp_time, data->invalid, err, GetErrorString(err));
      bin_log_record.a_num_wr_err += 1;
    }
  }

  void ShmCtx::WriteVideoRtpDataToShm(
    uint8_t *data, 
    size_t len,
    uint64_t seqid,
    uint64_t ts,
    uint8_t nal,
    bool isfragment,
    bool isfirstfragment,
    bool isendfragment,
    bool ispicturebegin,
    bool ispictureend,
    bool iskeyframe)
  {
    MS_TRACE();

    //DumpBinLogDataIfNeeded();

    // Try to dequeue whatever data is already accumulated if:
    // 1) new pkt (not retransmitted) arrived, and it belongs to a new frame (based on timestamp diff)
    // 2) If there is expired data in a queue
    uint64_t tsIncrement = (this->videoPktBuffer.size() > 0  && seqid + media[1].rtp_seq_offset > this->videoPktBuffer.rbegin()->seqid) ? ts - this->videoPktBuffer.rbegin()->ts : 0;

    if (SHM_Q_PKT_QUEUED_OK != this->Enqueue(data, 
                                              len,
                                              seqid + media[1].rtp_seq_offset,
                                              ts,
                                              nal,
                                              isfragment,
                                              isfirstfragment,
                                              isendfragment,
                                              ispicturebegin,
                                              ispictureend,
                                              iskeyframe))
    {
      MS_WARN_TAG(xcode, "shm[%s] enqueue() put nothing in", this->stream_name.c_str()); // TBD: see if there is ever a reason for Enqueue() to reject an item, right now it always enqueues
      return;
    }

    if ( ts - this->videoPktBuffer.begin()->ts > this->maxVideoPktDelay || tsIncrement > 0)
    {
      //MS_DEBUG_DEV("shm [%s] Dequeue [ %" PRIu64 " - %" PRIu64 " ] age=%" PRIu64 " incr=%" PRIu64, 
      //  this->stream_name.c_str(), this->videoPktBuffer.begin()->seqid, this->videoPktBuffer.rbegin()->seqid, ts - this->videoPktBuffer.begin()->ts, tsIncrement);
      this->Dequeue();
    }
  }


  void ShmCtx::WriteVideoChunk(ShmQueueItem* item, bool invalid)
  {
    MS_TRACE();

    if (!this->CanWrite(Media::VIDEO))
    {
      MS_WARN_TAG(xcode, "shm[%s] cannot write video ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " because writer is not initialized",
        this->stream_name.c_str(), media[1].new_ssrc, item->seqid, item->ts);
      return;
    }

    // TODO: remove when debugged
    if (item->keyframe && invalid)
      MS_WARN_TAG(xcode, "shm[%s] got invalid kf seq=%" PRIu64 " ts=%" PRIu64 " lastkeyframets=%" PRIu64, this->stream_name.c_str(), item->seqid, item->ts, this->lastKeyFrameTs);

    sfushm_av_frame_frag_t frag;
    frag.ssrc = media[1].new_ssrc;
    frag.first_rtp_seq = frag.last_rtp_seq = item->seqid;
    frag.rtp_time = item->ts;
    frag.invalid = invalid;
    frag.begin = item->fragment ? (item->firstfragment && item->beginpicture) : item->beginpicture;
    frag.end = item->endpicture; // if this is ending fragment, it is always also end of picture

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

    bin_log_record.v_egress_bytes += frag.len;
    bin_log_record.v_last_rtp_time = frag.rtp_time;
    bin_log_record.v_num_wr += 1;
    if (frag.invalid)
      bin_log_record.v_num_wr_incomplete_chunk += 1;
    
    int err = sfushm_av_write_video(wrt_ctx, &frag);
    if (IsError(err))
    {
      MS_WARN_TAG(xcode, "shm[%s] error writing chunk ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " len=%zu inv=%d: %d - %s",
        this->stream_name.c_str(), frag.ssrc, frag.first_rtp_seq, frag.rtp_time, frag.len, frag.invalid, err, GetErrorString(err));
      bin_log_record.v_num_wr_err += 1;
    }
    /*else
    {
      uint8_t dump[100];
      memset(dump, 0, sizeof(dump));
      ShmCtx::hex_dump(dump, frag.data, frag.len < 20 ? frag.len : 20);

      MS_DEBUG_TAG(xcode, "OK ssrc=%" PRIu32 " seq=%" PRIu64 " ts=%" PRIu64 " begin=%d end=%d len=%zu [%s]",
        frag.ssrc, frag.first_rtp_seq, frag.rtp_time, frag.begin, frag.end, frag.len, dump);
    }*/
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

    if (this->media[idx].sr_written) // make note of the first SR
    {
      MS_DEBUG_DEV("shm[%s] received SR: SSRC=%" PRIu32 " ReportNTP(ms)=%" PRIu64 " RtpTs=%" PRIu32 " uv_hrtime(ms)=%" PRIu64 " clock_gettime(s)=%" PRIu64 " clock_gettime(ms)=%" PRIu64,
        this->stream_name.c_str(),
        this->media[idx].new_ssrc,
        lastSenderReportNtpMs,
        lastSenderReportTs,
        walltime,
        ct,
        clockTimeMs);
    }
    else
    {
      MS_DEBUG_TAG(xcode, "shm[%s] received SR: SSRC=%" PRIu32 " ReportNTP(ms)=%" PRIu64 " RtpTs=%" PRIu32 " uv_hrtime(ms)=%" PRIu64 " clock_gettime(s)=%" PRIu64 " clock_gettime(ms)=%" PRIu64,
        this->stream_name.c_str(),
        this->media[idx].new_ssrc,
        lastSenderReportNtpMs,
        lastSenderReportTs,
        walltime,
        ct,
        clockTimeMs);
    }

    if (SHM_WRT_READY != this->wrt_status)
      return; // shm writer is not set up yet

    WriteSR(kind);

    if (this->CanWrite(kind))
      return; // already submitted the first SRs into shm, and it is in ready state

    this->media[idx].sr_written = true;
    
    /*
    Caution is needed when requesting a keyframe:
    it is possible to have at least the first chunk of a keyframe in the queue before SR arrived,
    in that case do not re-request a keyframe
    */
    if (kind == Media::VIDEO
        && lastKeyFrameTs == UINT64_UNSET)
      this->listener->OnNeedToSync();
  }


  void ShmCtx::WriteSR(Media kind)
  {
    MS_TRACE();

    size_t idx = (kind == Media::AUDIO) ? 0 : 1;
    int err = sfushm_av_write_rtcp_sr_ts(wrt_ctx, this->media[idx].sr_ntp_msb, this->media[idx].sr_ntp_lsb, this->media[idx].sr_rtp_tm, this->media[idx].new_ssrc);
    if (IsError(err))
    {
      MS_WARN_TAG(xcode, "shm[%s] error writing RTCP SR (ssrc:%" PRIu32 ") %d - %s",
        this->stream_name.c_str(), this->media[idx].new_ssrc, err, GetErrorString(err));
    }
  }


  int ShmCtx::WriteStreamMeta(std::string metadata, std::string shm)
  {
    MS_TRACE();

    if (SHM_WRT_READY != this->wrt_status)
    {
      MS_WARN_TAG(xcode, "shm[%s] cannot write stream metadata because shm writer is not initialized", this->stream_name.c_str());
      return -1; // shm writer is not set up yet
    }

    int     err;
    uint8_t data[256]; // Currently this is overkill because just 1 byte will be written successfully into shm

    if (0 != this->stream_name.compare(shm))
    {
      MS_WARN_TAG(xcode, "shm[%s] mismatched shm name '%s' in metadata", this->stream_name.c_str(), shm.c_str());
      return -1;
    }

    if (metadata.length() > 255)
    {
      MS_WARN_TAG(xcode, "shm[%s] input metadata is too long: %s", this->stream_name.c_str(), metadata.c_str());
    }
    std::copy(metadata.begin(), metadata.end(), data);

    err = sfushm_av_write_stream_metadata(wrt_ctx, data, metadata.length());
    if (IsError(err))
    {
      MS_WARN_TAG(xcode, "shm[%s] error writing stream metadata: %d - %s", this->stream_name.c_str(), err, GetErrorString(err));
      return -1;
    }

    return 0;
  }


  void ShmCtx::WriteVideoOrientation(uint16_t rotation)
  {
    MS_TRACE();

    if (SHM_WRT_READY != this->wrt_status)
    {
      MS_WARN_TAG(xcode, "shm[%s] write not initialized, cannot write video rotation", this->stream_name.c_str());
      return; // shm writer is not set up yet
    }

    int err = sfushm_av_write_video_rotation(wrt_ctx, rotation);

    if (IsError(err))
      MS_WARN_TAG(xcode, "shm[%s] error writing video rotation: %d - %s", this->stream_name.c_str(), err, GetErrorString(err));
  }
} // namespace DepLibSfuShm