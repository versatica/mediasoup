#define MS_CLASS "Lively::StatsBinLog"

#include "DepLibUV.hpp"
#include "LivelyBinLogs.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring>

namespace Lively
{

constexpr uint8_t hexVal[256] = {
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 0, 0, 0, 0, 0, 0,  // '0'=0x30 .. '9'
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'A'=0x41 .. 'F'
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 'a'=0x61 .. 'f'
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


CallStatsRecord::CallStatsRecord(uint64_t type, RTC::RtpCodecMimeType m, std::string call, std::string obj, std::string producer)
  : source(type), filled(UINT8_UNSET), call_id(call), object_id(obj), producer_id(producer) 
{
  start_tm = Utils::Time::currentStdEpochMs();
  mime = (RTC::RtpCodecMimeType::Type::AUDIO == m.type) ? 1 : 2;
  fillHeader();
}


uint8_t* CallStatsRecord::hexStrToBytes(const char* from, int num, uint8_t* to)
{
  for (int i = 0; i < num; i++)
  {
    *to++ = (hexVal[from[i * 2] & 0xFF] << 4) + (hexVal[from[i * 2 + 1] & 0xFF]);
  }
  return to;
}


bool CallStatsRecord::uuidToBytes(std::string uuid, uint8_t *out)
{
  //00000000-0000-0000-0000-000000000000 : remove '-' and convert digit chars into hex
  char cstr[37];
  //out should be 16 bytes long
  uint8_t *p = out;
  std::memset(cstr, 0, sizeof(cstr));
  std::strcpy(cstr, uuid.c_str());
  if (std::strlen(cstr) != 36)
    return false;

  char *ch = std::strtok(cstr, "-");
  while (ch)
  {
    p = hexStrToBytes(ch, std::strlen(ch)/2, p);
    ch = std::strtok(NULL,"-");
  }
  return true;
}


void CallStatsRecord::fillHeader()
{
  uint8_t *p;
  uint8_t uuidBytes[16];

  std::memset(write_buf, 0, sizeof(write_buf));
  p = &(write_buf[0]);
  std::memcpy(p, &start_tm, sizeof(uint64_t)); 
  p += sizeof(uint64_t);
  if(source == 1) // consumer
  {
    std::memset(uuidBytes, 0, 16);
    uuidToBytes(object_id, uuidBytes);
    std::memcpy(p, uuidBytes, 16);
    p += 16;

    std::memset(uuidBytes, 0, 16);
    uuidToBytes(producer_id, uuidBytes);
    std::memcpy(p, uuidBytes, 16);
    p += 16;
  }
  std::memcpy(p, &mime, 1);
  p++;
  std::memcpy(p, &filled, 1);
}

// fd is opened file handle; if returned false, a caller should close the file, etc.
bool CallStatsRecord::fwriteRecord(std::FILE* fd)
{
  size_t rec_sz = (source == 1) ? CONSUMER_REC_HEADER_SIZE : PRODUCER_REC_HEADER_SIZE;
  std::memcpy(write_buf, &start_tm, sizeof(uint64_t));  //update timestamp, the only variable field in a header
  
  //fillHeader();

  std::size_t rc = std::fwrite(write_buf, 1, rec_sz, fd);
  if (rc <= 0 || rc != rec_sz)
    return false;

  rc = std::fwrite(samples, sizeof(CallStatsSample), CALL_STATS_BIN_LOG_RECORDS_NUM, fd);
  if (rc <= 0 || rc != CALL_STATS_BIN_LOG_RECORDS_NUM)
    return false;

  return true;
}

/////////////////////////
//
void CallStatsRecordCtx::WriteIfFull(StatsBinLog* log)
{
  if (record.filled == UINT8_UNSET || record.filled < CALL_STATS_BIN_LOG_RECORDS_NUM)
    return;

  if (nullptr != log)
  {
    log->OnLogWrite(this);
  }

  // Wipe the data off
  record.resetSamples();
}


void CallStatsRecordCtx::AddStatsRecord(StatsBinLog* log, RTC::RtpStream* stream)
{
  WriteIfFull(log);

  uint64_t nowMs = Utils::Time::currentStdEpochMs();
  if (UINT8_UNSET == record.filled)
  {
    record.start_tm = nowMs;
    stream->FillStats(last.packetsCount, last.bytesCount, last.packetsLost, last.packetsDiscarded,
                      last.packetsRetransmitted, last.packetsRepaired, last.nackCount,
                      last.nackPacketCount, last.kfCount, last.rtt, last.maxPacketTs);
    record.filled = 0; // now start reading data
  }
  else
  {
    MS_ASSERT(record.filled >= 0 && record.filled < CALL_STATS_BIN_LOG_RECORDS_NUM, "Invalid record.filled=%" PRIu16, record.filled);
    stream->FillStats(curr.packetsCount, curr.bytesCount, curr.packetsLost, curr.packetsDiscarded,
                      curr.packetsRetransmitted, curr.packetsRepaired, curr.nackCount,
                      curr.nackPacketCount, curr.kfCount, curr.rtt, curr.maxPacketTs);

    record.samples[record.filled].epoch_len = static_cast<uint16_t>(nowMs - record.start_tm);
    record.samples[record.filled].packets_count = static_cast<uint16_t>(curr.packetsCount - last.packetsCount);
    record.samples[record.filled].bytes_count = static_cast<uint32_t>(curr.bytesCount - last.bytesCount);
    record.samples[record.filled].packets_lost = static_cast<uint16_t>(curr.packetsLost - last.packetsLost);
    record.samples[record.filled].packets_discarded = static_cast<uint16_t>(curr.packetsDiscarded - last.packetsDiscarded);
    record.samples[record.filled].packets_repaired = static_cast<uint16_t>(curr.packetsRepaired - last.packetsRepaired);
    record.samples[record.filled].packets_retransmitted = static_cast<uint16_t>(curr.packetsRetransmitted - last.packetsRetransmitted);
    record.samples[record.filled].nack_count = static_cast<uint16_t>(curr.nackCount - last.nackCount);
    record.samples[record.filled].nack_pkt_count = static_cast<uint16_t>(curr.nackPacketCount - last.nackPacketCount);
    record.samples[record.filled].kf_count = static_cast<uint16_t>(curr.kfCount - last.kfCount);
    record.samples[record.filled].rtt = static_cast<uint16_t>(std::round(curr.rtt));
    record.samples[record.filled].max_pts = curr.maxPacketTs;
    
    this->last = this->curr;
    record.filled++;
  }
}

/////////////////////////
//
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
      if (!ctx->record.fwriteRecord(this->fd))
        ret = errno;
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
    ctx->record.resetSamples();
  }

  return ret;
}


int StatsBinLog::OnLogWrite(CallStatsRecordCtx* ctx)
{
  #define DAY_IN_MS (uint64_t)86400000

  int ret = 0;
  bool signal_set = false;

  uint64_t now = Utils::Time::currentStdEpochMs(); //DepLibUV::GetTimeMs();

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

  if(this->fd && ctx && ctx->record.filled)
  {
    if (!ctx->record.fwriteRecord(this->fd))
    {
      ret = errno;
      std::fclose(this->fd);
      this->fd = 0;
    }
    std::fflush(this->fd);
  }
  return ret;
}


void StatsBinLog::InitLog(char type, std::string id1, std::string id2)
{
  char tmp[100];
  memset(tmp, '\0', 100);
  switch(type)
  {
    case 'c':
      sprintf(tmp, "/var/log/sfu/c_%s_%%llu.bin.log", id1.c_str());
      this->bin_log_name_template.assign(tmp);
      MS_DEBUG_TAG(rtp, "consumers binlog transport id %s", id2.c_str());
      break;
    case 'p':
      sprintf(tmp, "/var/log/sfu/p_%s_%s_%%llu.bin.log", id1.c_str(), id2.c_str());
      this->bin_log_name_template.assign(tmp);
      break;
    default:
      break;
  }

  uint64_t now = Utils::Time::currentStdEpochMs();
  this->log_start_ts = now;
  UpdateLogName();
  
  this->sampling_interval = CALL_STATS_BIN_LOG_SAMPLING;
  this->initialized = true;
}


void StatsBinLog::UpdateLogName()
{
  char buff[100];
  memset(buff, '\0', 100);
  sprintf(buff, this->bin_log_name_template.c_str(), this->log_start_ts);
  this->bin_log_file_path.assign(buff);
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
} //Lively