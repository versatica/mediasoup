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


CallStatsRecord::CallStatsRecord(uint64_t type, uint8_t payload, std::string callId, std::string obj, std::string producer)
  : type(type), call_id(callId), object_id(obj), producer_id(producer) 
{
  uint64_t ts = Utils::Time::currentStdEpochMs();

  if (type) // consumer
  {
    record.c.start_tm = ts;
    record.c.payload = payload;
    record.c.filled=UINT32_UNSET;
    
    std::memset(record.c.consumer_id, 0, UUID_BYTE_LEN);
    uuidToBytes(obj, record.c.consumer_id);
    
    std::memset(record.c.producer_id, 0, UUID_BYTE_LEN);
    uuidToBytes(producer, record.c.producer_id);

    std::memset(record.c.samples, 0, sizeof(record.c.samples));
  }
  else // producer
  {
    record.p.start_tm = ts;
    record.p.filled=UINT32_UNSET;
    record.p.payload = payload;
    
    std::memset(record.p.samples, 0, sizeof(record.p.samples));
  }
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
  if (std::strlen(cstr) != UUID_CHAR_LEN)
    return false;

  char *ch = std::strtok(cstr, "-");
  while (ch)
  {
    p = hexStrToBytes(ch, std::strlen(ch)/2, p);
    ch = std::strtok(NULL,"-");
  }
  return true;
}

// fd is opened file handle; if returned false, a caller should close the file, etc.
bool CallStatsRecord::fwriteRecord(std::FILE* fd)
{
  int rc;
  if (type) // consumer
  {
    rc = std::fwrite(&record.c, sizeof(ConsumerRecord), 1, fd);
  }
  else
  {
    rc = std::fwrite(&record.p, sizeof(ProducerRecord), 1, fd);
  }
  return  (rc > 0);
}

void CallStatsRecord::resetSamples()
{
  // Wipe off samples data
  if (type) // consumer
  {
    std::memset(&record.c.samples, 0, sizeof(record.c.samples));
    record.c.filled = UINT32_UNSET;
    record.c.start_tm = UINT64_UNSET;
  }
  else
  {
    std::memset(record.p.samples, 0, sizeof(record.p.samples));
    record.p.filled = UINT32_UNSET;
    record.p.start_tm = UINT64_UNSET;
  }
}


void CallStatsRecord::zeroSamples(uint64_t nowMs)
{
  if (type)
  {
    record.c.start_tm = nowMs;
    record.c.filled = 0;
  }
  else
  {
    record.p.start_tm = nowMs;
    record.p.filled = 0;
  }
}


bool CallStatsRecord::addSample(StreamStats& last, StreamStats& curr)
{
  if (filled() == UINT32_UNSET || filled() >= CALL_STATS_BIN_LOG_RECORDS_NUM)
    return false;
  
  uint32_t idx = filled();
  CallStatsSample* s = type ? &(record.c.samples[0]) : &(record.p.samples[0]);

  s[idx].epoch_len = static_cast<uint16_t>(curr.ts - last.ts);
  s[idx].packets_count = static_cast<uint16_t>(curr.packetsCount - last.packetsCount);
  s[idx].bytes_count = static_cast<uint32_t>(curr.bytesCount - last.bytesCount);
  s[idx].packets_lost = (curr.packetsLost > last.packetsLost) ? static_cast<uint16_t>(curr.packetsLost - last.packetsLost) : 0;
  s[idx].packets_discarded = static_cast<uint16_t>(curr.packetsDiscarded - last.packetsDiscarded);
  s[idx].packets_repaired = static_cast<uint16_t>(curr.packetsRepaired - last.packetsRepaired);
  s[idx].packets_retransmitted = static_cast<uint16_t>(curr.packetsRetransmitted - last.packetsRetransmitted);
  s[idx].nack_count = static_cast<uint16_t>(curr.nackCount - last.nackCount);
  s[idx].nack_pkt_count = static_cast<uint16_t>(curr.nackPacketCount - last.nackPacketCount);
  s[idx].kf_count = static_cast<uint16_t>(curr.kfCount - last.kfCount);
  s[idx].rtt = static_cast<uint16_t>(std::round(curr.rtt));
  s[idx].max_pts = curr.maxPacketTs;
  
  if (type)
    record.c.filled++;
  else
    record.p.filled++;
  return true;
}

/////////////////////////
//
void CallStatsRecordCtx::WriteIfFull(StatsBinLog* log)
{
  if (record.filled() == UINT32_UNSET || record.filled() < CALL_STATS_BIN_LOG_RECORDS_NUM)
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
  if (UINT32_UNSET == record.filled())
  {
    stream->FillStats(last.packetsCount, last.bytesCount, last.packetsLost, last.packetsDiscarded,
                      last.packetsRetransmitted, last.packetsRepaired, last.nackCount,
                      last.nackPacketCount, last.kfCount, last.rtt, last.maxPacketTs);
    last.ts = nowMs;
    record.zeroSamples(nowMs);
  }
  else
  {
    MS_ASSERT(record.filled() >= 0 && record.filled() < CALL_STATS_BIN_LOG_RECORDS_NUM, "Invalid record.filled=%" PRIu16, record.filled());
    stream->FillStats(curr.packetsCount, curr.bytesCount, curr.packetsLost, curr.packetsDiscarded,
                      curr.packetsRetransmitted, curr.packetsRepaired, curr.nackCount,
                      curr.nackPacketCount, curr.kfCount, curr.rtt, curr.maxPacketTs);
    curr.ts = nowMs;
    record.addSample(last, curr);    
    this->last = this->curr;
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
      "binlog failed to open '%s'", this->bin_log_file_path.c_str()
    );
    ret = errno;
    initialized = false;
  }
  else
  {
    MS_DEBUG_TAG(
      rtp,
      "binlog opened '%s'", this->bin_log_file_path.c_str()
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
      "binlog closed '%s'", this->bin_log_file_path.c_str()
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

  uint64_t now = Utils::Time::currentStdEpochMs();

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
      this->fd = 0;

    if (this->initialized)
    {
      LogOpen();
    }
  }

  if (!this->fd)
  {
    MS_WARN_TAG(
      rtp,
      "binlog can't write, fd=0"
    );
  }
  if (!ctx)
  {
    MS_WARN_TAG(
      rtp,
      "binlog can't write, ctx=0"
    );
  }

  if(this->fd && ctx && ctx->record.filled())
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
  #define FILENAME_LEN_MAX sizeof("/var/log/sfu/ms_p_00000000-0000-0000-0000-000000000000_00000000-0000-0000-0000-000000000000_1652210519459.123abc.bin") * 2
  char tmp[FILENAME_LEN_MAX];
  std::memset(tmp, '\0', FILENAME_LEN_MAX);
  switch(type)
  {
    case 'c':
      sprintf(tmp, "/var/log/sfu/ms_c_%s_%%llu.%s.bin", id1.c_str(), version);
      this->bin_log_name_template.assign(tmp);
      MS_DEBUG_TAG(rtp, "consumers binlog transport id %s", id2.c_str());
      break;
    case 'p':
      sprintf(tmp, "/var/log/sfu/ms_p_%s_%s_%%llu.%s.bin", id1.c_str(), id2.c_str(), version);
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