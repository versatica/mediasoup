#define MS_CLASS "Lively::StatsBinLog"

#include "DepLibUV.hpp"
#include "LivelyBinLogs.hpp"
#include "Logger.hpp"
#include "Utils.hpp"


namespace Lively
{

CallStatsRecordCtx::CallStatsRecordCtx(std::string callId, std::string objId, uint64_t objType)
{
  std::strncpy(log_record.call_id, callId.c_str(), 36);
  std::strncpy(log_record.object_id, objId.c_str(), 36);
  log_record.source = objType;
  log_record.start_tm = DepLibUV::GetTimeMs();
  log_record.filled = UINT16_UNSET;
}


void CallStatsRecordCtx::WriteIfFull(StatsBinLog* log)
{
  if (log_record.filled == UINT16_UNSET || log_record.filled < CALL_STATS_BIN_LOG_RECORDS_NUM)
    return;

  if (nullptr != log)
  {
    log->OnLogWrite(this);
  }

  // Wipe the data off
  std::memset(log_record.samples, 0, sizeof(log_record.samples));
  log_record.filled = UINT16_UNSET;
}


void CallStatsRecordCtx::AddStatsRecord(StatsBinLog* log, RTC::RtpStream* stream)
{
  WriteIfFull(log);

  uint64_t nowMs = DepLibUV::GetTimeMs();
  if (UINT16_UNSET == log_record.filled)
  {
    log_record.start_tm = nowMs;
    stream->FillStats(last.packetsCount, last.bytesCount, last.packetsLost, last.packetsDiscarded,
                      last.packetsRetransmitted, last.packetsRepaired, last.nackCount,
                      last.nackPacketCount, last.kfCount, last.rtt, last.maxPacketTs);
    log_record.filled = 0; // now start reading data
  }
  else
  {
    stream->FillStats(curr.packetsCount, curr.bytesCount, curr.packetsLost, curr.packetsDiscarded,
                      curr.packetsRetransmitted, curr.packetsRepaired, curr.nackCount,
                      curr.nackPacketCount, curr.kfCount, curr.rtt, curr.maxPacketTs);

    log_record.samples[log_record.filled].epoch_len = static_cast<uint16_t>(nowMs - log_record.start_tm);
    log_record.samples[log_record.filled].packets_count = static_cast<uint16_t>(curr.packetsCount - last.packetsCount);
    log_record.samples[log_record.filled].bytes_count = static_cast<uint32_t>(curr.bytesCount - last.bytesCount);
    log_record.samples[log_record.filled].packets_lost = static_cast<uint16_t>(curr.packetsLost - last.packetsLost);
    log_record.samples[log_record.filled].packets_discarded = static_cast<uint16_t>(curr.packetsDiscarded - last.packetsDiscarded);
    log_record.samples[log_record.filled].packets_repaired = static_cast<uint16_t>(curr.packetsRepaired - last.packetsRepaired);
    log_record.samples[log_record.filled].packets_retransmitted = static_cast<uint16_t>(curr.packetsRetransmitted - last.packetsRetransmitted);
    log_record.samples[log_record.filled].nack_count = static_cast<uint16_t>(curr.nackCount - last.nackCount);
    log_record.samples[log_record.filled].nack_pkt_count = static_cast<uint16_t>(curr.nackPacketCount - last.nackPacketCount);
    log_record.samples[log_record.filled].kf_count = static_cast<uint16_t>(curr.kfCount - last.kfCount);
    log_record.samples[log_record.filled].rtt = static_cast<uint16_t>(std::round(curr.rtt));
    log_record.samples[log_record.filled].max_pts = curr.maxPacketTs;
    
    this->last = this->curr;
    log_record.filled++;
  }
}


int StatsBinLog::LogOpen()
{
  int ret = 0;

  this->fd = std::fopen(this->bin_log_file_path.c_str(), "a");
  if (!this->fd)
  {
    MS_WARN_TAG(
      rtp,
      "bin log failed to open '%s'", this->bin_log_file_path.c_str()
    );
    ret = errno;
    initialized = false;
  }
  else
  {
    MS_DEBUG_TAG(
      rtp,
      "bin log opened '%s'", this->bin_log_file_path.c_str()
    );
  }

  return ret;
}


int StatsBinLog::LogClose(CallStatsRecordCtx* ctx)
{
  int ret = 0;

  if (this->fd)
  {
    if (ctx)
    {
      std::size_t rc = std::fwrite(&(ctx->log_record), sizeof(CallStatsRecord), 1, this->fd);
      if (rc <= 0 || rc != sizeof(CallStatsRecord))
      {
        ret = errno;
      }
      //rc = std::fputc('\n', this->fd);
      //if (rc <= 0) // TODO: different way of checking for errors, see std::ferror
      //  ret = errno;
    }
    std::fflush(this->fd);
    std::fclose(this->fd);
    this->fd = 0;
    
    MS_DEBUG_TAG(
      rtp,
      "bin log closed '%s'", this->bin_log_file_path.c_str()
    );
  }

  // Save the last sample just in case and clean up recorded data
  if (ctx)
  {
    std::memset(ctx->log_record.samples, 0, sizeof(ctx->log_record.samples));
    ctx->log_record.filled = UINT16_UNSET;
  }

  return ret;
}


int StatsBinLog::OnLogWrite(CallStatsRecordCtx* ctx)
{
  #define DAY_IN_MS (uint64_t)86400000

  int ret = 0;
  bool signal_set = false;

  uint64_t now = DepLibUV::GetTimeMs();

  if (now - this->log_start_ts > DAY_IN_MS)
  {
    this->log_start_ts = now;
    UpdateLogName();    
    signal_set = true;
  }

  if(signal_set || !this->fd)
  {
    if (this->fd)
      std::fclose(this->fd);

    if (this->initialized)
    {
      LogOpen();
    }
  }

  if (!this->fd)
  {
    MS_WARN_TAG(
      rtp,
      "bin log can't write, fd=0"
    );
  }
  if (!ctx)
  {
    MS_WARN_TAG(
      rtp,
      "bin log can't write, ctx=0"
    );
  }

  if(this->fd && ctx && ctx->log_record.filled)
  {
    std::size_t rc = std::fwrite(&(ctx->log_record), sizeof(CallStatsRecord), 1, this->fd);
    if(rc <= 0)
    {
      ret = errno;
      std::fclose(this->fd);
      this->fd = 0;
    }
    std::fflush(this->fd);
  }
  return ret;
}


void StatsBinLog::InitLog(std::string id1, std::string id2)
{
  char tmp[100];
  memset(tmp, '\0', 100);
  sprintf(tmp, "/var/log/sfu/%s_%s_%%u.bin.log", id1.c_str(), id2.c_str());
  this->bin_log_name_template.assign(tmp);
  
  //this->bin_log_name_template.assign("/var/log/sfu/");
  //this->bin_log_name_template += id1 + id2;

  uint64_t now = DepLibUV::GetTimeMs();
  this->log_start_ts = now;
  UpdateLogName();
  
  this->sampling_interval = CALL_STATS_BIN_LOG_SAMPLING;
  this->initialized = true;
}

void StatsBinLog::DeinitLog(CallStatsRecordCtx* recordCtx)
{
  if (this->fd)
  {
    LogClose(recordCtx);
  }
  this->initialized = false;
  this->log_start_ts = UINT64_UNSET;
  this->bin_log_name_template.clear();
  this->bin_log_file_path.clear();
}


void StatsBinLog::UpdateLogName()
{
  char buff[100];
  memset(buff, '\0', 100);
  sprintf(buff, this->bin_log_name_template.c_str(), this->log_start_ts);
  this->bin_log_file_path.assign(buff); 
}
} //Lively