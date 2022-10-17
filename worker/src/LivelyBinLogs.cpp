#define MS_CLASS "Lively::StatsBinLog"

#include "DepLibUV.hpp"
#include "LivelyBinLogs.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring>
#include <sys/stat.h>

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


CallStatsRecord::CallStatsRecord(uint64_t type, uint16_t ssrc, char payloadType, std::string callId, std::string obj, std::string producer)
  : type(type), call_id(callId), object_id(obj), producer_id(producer)
{
  uint64_t ts = Utils::Time::currentStdEpochMs();
  set_start_tm(ts);

  set_filled(0);

  if (type) // consumer
  {
    record.c.ssrc = ssrc;
    record.c.payload_type = payloadType;
    std::memset(record.c.consumer_id, 0, UUID_BYTE_LEN);
    uuidToBytes(obj, record.c.consumer_id);
    
    std::memset(record.c.producer_id, 0, UUID_BYTE_LEN);
    uuidToBytes(producer, record.c.producer_id);

    std::memset(record.c.samples, 0, sizeof(record.c.samples));

    MS_DEBUG_TAG(rtp, "CallStatsRecord ctor(): consumer start_tm=%" PRIu64 " ssrc=%" PRIu16 " type=%c callId=%s consumerId=%s producerId=%s", 
      ts, ssrc, payloadType, call_id.c_str(), object_id.c_str(), producer_id.c_str());
  }
  else // producer
  {
    record.p.ssrc = ssrc;
    record.p.payload_type = payloadType;
    std::memset(record.p.samples, 0, sizeof(record.p.samples));

    MS_DEBUG_TAG(rtp, "CallStatsRecord ctor(): producer start_tm=%" PRIu64 " ssrc=%" PRIu16 " type=%c callId=%s producerId=%s", 
      ts, ssrc, payloadType, call_id.c_str(), object_id.c_str());
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

void CallStatsRecord::resetSamples(uint64_t ts)
{
  // Wipe off samples data
  if (type) // consumer
  {
    std::memset(&record.c.samples, 0, sizeof(record.c.samples));
  }
  else
  {
    std::memset(record.p.samples, 0, sizeof(record.p.samples));
  }
  set_filled(0);
  set_start_tm(ts);
}

void CallStatsRecord::addSample(StreamStats& last, StreamStats& curr)
{
  MS_ASSERT(filled() >= 0 && filled() < CALL_STATS_BIN_LOG_RECORDS_NUM, 
            "Cannot have %" PRIu32 " samples in record, quitting...", filled());

  MS_ASSERT(last.ts != UINT64_UNSET,
            "Timestamp of a previous sample is unset, quitting...");
            
  MS_ASSERT(curr.ts != UINT64_UNSET,
            "Timestamp of a current sample is unset, quitting...");

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

  set_filled(idx + 1);
}

/////////////////////////
//
void CallStatsRecordCtx::AddStatsRecord(StatsBinLog* log, RTC::RtpStream* stream)
{
  // Write data if record is full, then continue collecting samples
  if (record.filled() == CALL_STATS_BIN_LOG_RECORDS_NUM)
  {
    if (nullptr != log)
    {
      log->OnLogWrite(this);
    }

    // Wipe the data off and set record's start timestamp into the last sample's
    record.resetSamples(last.ts);
  }

  uint64_t nowMs = Utils::Time::currentStdEpochMs(); // Timestamp of a current measurement

  if (UINT64_UNSET == last.ts) // the first measurement ever during this session
  {
    stream->FillStats(last.packetsCount, last.bytesCount, last.packetsLost, last.packetsDiscarded,
                      last.packetsRetransmitted, last.packetsRepaired, last.nackCount,
                      last.nackPacketCount, last.kfCount, last.rtt, last.maxPacketTs);
    last.ts = nowMs;
    record.resetSamples(nowMs);
    return; // done, wait till the next measurement
  }

  // Should have a room to add a sample
  MS_ASSERT(record.filled() >= 0 && record.filled() < CALL_STATS_BIN_LOG_RECORDS_NUM,
    "Invalid record.filled=%" PRIu32, record.filled());

  curr.ts = nowMs;
  stream->FillStats(curr.packetsCount, curr.bytesCount, curr.packetsLost, curr.packetsDiscarded,
                    curr.packetsRetransmitted, curr.packetsRepaired, curr.nackCount,
                    curr.nackPacketCount, curr.kfCount, curr.rtt, curr.maxPacketTs);

  record.addSample(last, curr);
  this->last = this->curr;
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
    this->fd = 0;
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


void StatsBinLog::LogClose()
{
  if (this->fd)
  {
    std::fflush(this->fd);
    std::fclose(this->fd);
    this->fd = 0;

    MS_DEBUG_TAG(
      rtp,
      "binlog closed '%s'", this->bin_log_file_path.c_str()
    );
  }

  if (!this->initialized)
    return;

  // Do not preserve binlogs with too little data
  if (log_start_ts == UINT64_UNSET 
      || log_last_ts == UINT64_UNSET 
      || BINLOG_MIN_TIMESPAN > log_last_ts - log_start_ts)
  {
    std::remove(bin_log_file_path.c_str());
    MS_DEBUG_TAG(rtp, "binlog %s removed, short timespan (%" PRIu64 "-%" PRIu64 ")", 
                  this->bin_log_file_path.c_str(), this->log_start_ts, log_last_ts);
    return;
  }

  // Move a closed file into "done" directory
  std::string bin_log_done_dir = "/var/log/sfu/bin/done/";

  std::string logname; // get filename only out of full path
  std::size_t found = this->bin_log_file_path.find_last_of("/");
  if (std::string::npos == found)
  {
    MS_WARN_TAG(rtp, "Failed to extract binlog filename from %s, cannot move it", this->bin_log_file_path.c_str());
  }
  else
  {
    auto logname = this->bin_log_file_path.substr(found + 1);
    auto dst = bin_log_done_dir;
    dst.append(logname);
    if (std::rename(this->bin_log_file_path.c_str(), dst.c_str()))
    {
      MS_WARN_TAG(rtp, "failed to move %s to %s", this->bin_log_file_path.c_str(), bin_log_done_dir.c_str());
    }
    else
    {
      MS_DEBUG_TAG(rtp, "moved binlog %s to %s", this->bin_log_file_path.c_str(), bin_log_done_dir.c_str());
    }
  }
}


int StatsBinLog::OnLogWrite(CallStatsRecordCtx* ctx)
{
  #define DAY_IN_MS (uint64_t)86400000
  int ret = 0;
  bool signal_set = false;

  uint64_t now = Utils::Time::currentStdEpochMs();

  if (!this->initialized)
    return ret;

  if (now - this->log_start_ts > DAY_IN_MS)
  {
    this->log_start_ts = now;
    UpdateLogName();
    signal_set = true;
  }

  if(signal_set || !this->fd)
  {
    if (this->fd)
    {
      std::fclose(this->fd);
      this->fd = 0;
    }

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
    else
    {
      this->log_last_ts = ctx->LastTs(); // to tell later if the binlog is too short to be valuable
    }
    std::fflush(this->fd);
  }
  return ret;
}


bool StatsBinLog::CreateBinlogDirsIfMissing()
{
  std::string bin_log_dir      = "/var/log/sfu/bin/";
  std::string bin_log_curr_dir = "/var/log/sfu/bin/current/";
  std::string bin_log_done_dir = "/var/log/sfu/bin/done/";
  
  struct stat info;
  int ret = 0;
  if( stat( bin_log_dir.c_str(), &info ) != 0 )
  {
    if (errno == ENOENT)
    {
      ret = mkdir(bin_log_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (ret != 0 && errno != EEXIST)
      {
        MS_WARN_TAG(rtp, "failed to create folder %s for binlog files management: %s", bin_log_dir.c_str(), std::strerror(errno));
        return false;
      }
    }
  }
  else
  {
    if (!S_ISDIR(info.st_mode))
    {
      MS_WARN_TAG(rtp, "found %s but it is not a directory", bin_log_dir.c_str());
      return false;
    }
  }

  // Now that /var/log/sfu/bin/ exists, take care of "current" and "done" directories
  if( stat( bin_log_curr_dir.c_str(), &info ) != 0 )
  {
    if (errno == ENOENT)
    {
      ret = mkdir(bin_log_curr_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      if (ret != 0 && errno != EEXIST)
      {
        MS_WARN_TAG(rtp, "failed to create folder %s for writing binlogs: %s", bin_log_curr_dir.c_str(), std::strerror(errno));
        return false;
      }
    }
  }
  else
  {
    if (!S_ISDIR(info.st_mode))
    {
      MS_WARN_TAG(rtp, "found %s but it is not a directory", bin_log_curr_dir.c_str());
      return false;
    }
  }

  if( stat( bin_log_done_dir.c_str(), &info ) != 0 )
  {
    if (errno == ENOENT)
    {
      ret = mkdir(bin_log_done_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // | S_IXOTH?
      if (ret != 0 && errno != EEXIST)
      {
        MS_WARN_TAG(rtp, "failed to create folder %s for moving complete binlogs: %s", bin_log_done_dir.c_str(), std::strerror(errno));
        return false;
      }
    }
  }
  else
  {
    if (!S_ISDIR(info.st_mode))
    {
      MS_WARN_TAG(rtp, "found %s but it is not a directory", bin_log_done_dir.c_str());
      return false;
    }
  }

  return true;
}


void StatsBinLog::InitLog(char type, std::string id1, std::string id2)
{
  #define FILENAME_LEN_MAX sizeof("/var/log/sfu/bin/current/ms_p_00000000-0000-0000-0000-000000000000_00000000-0000-0000-0000-000000000000_1652210519459.123abc.bin") * 2
  char tmp[FILENAME_LEN_MAX];
  std::memset(tmp, '\0', FILENAME_LEN_MAX);
  switch(type)
  {
    case 'c':
      sprintf(tmp, "/var/log/sfu/bin/current/ms_c_%s_%%llu.%s.bin", id1.c_str(), version);
      this->bin_log_name_template.assign(tmp);
      MS_DEBUG_TAG(rtp, "consumers binlog %s [transportId: %s]", this->bin_log_name_template.c_str(), id2.c_str());
      break;
    case 'p':
      sprintf(tmp, "/var/log/sfu/bin/current/ms_p_%s_%s_%%llu.%s.bin", id1.c_str(), id2.c_str(), version);
      this->bin_log_name_template.assign(tmp);
      MS_DEBUG_TAG(rtp, "producer binlog %s", this->bin_log_name_template.c_str());
      break;
    default:
      break;
  }

  uint64_t now = Utils::Time::currentStdEpochMs();
  this->log_start_ts = now;
  UpdateLogName();

   MS_ASSERT(CreateBinlogDirsIfMissing(), "Cannot create binlog directories!");
  
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


void StatsBinLog::DeinitLog()
{
  LogClose();

  this->initialized = false;
  this->log_start_ts = UINT64_UNSET;
  this->bin_log_name_template.clear();
  this->bin_log_file_path.clear();
}
} //Lively