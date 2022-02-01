#ifndef MS_LIVELY_BIN_LOGS_HPP
#define MS_LIVELY_BIN_LOGS_HPP

#include "common.hpp"
#include <cstring>
#include "RTC/RtpStream.hpp"

#define CALL_STATS_BIN_LOG_RECORDS_NUM 30
#define CALL_STATS_BIN_LOG_SAMPLING    2000

namespace Lively
{
  struct CallStatsSample
  {
    uint16_t epoch_len;             // epoch duration in milliseconds
    uint16_t packets_count;         // RTP packets per epoch
    uint16_t packets_lost;
    uint16_t packets_discarded;
    uint16_t packets_retransmitted;
    uint16_t packets_repaired;
    uint16_t nack_count;            // number of NACK requests
    uint16_t nack_pkt_count;        // number of NACK packets requested
    uint16_t kf_count;              // Requests for key frame, PLI or FIR, see RequestKeyFrame()
    uint16_t rtt;
    uint32_t max_pts;
    uint32_t bytes_count;
  };

  struct CallStatsRecord
  {
    uint64_t                    start_tm;      // the record start timestamp in milliseconds
    char                        call_id[36];   // call id: uuid4() without \0
    char                        object_id[36]; // uuid4() no \0, producer or consumer id
    uint8_t                     source {0};        // 0 for producer or 1 for consumer
    uint8_t                     mime {0};          // mime type: unset, audio, video
    uint16_t                    filled {0};        // number of filled records in the array below
    CallStatsSample             samples[CALL_STATS_BIN_LOG_RECORDS_NUM];
  };

  class StatsBinLog;

  // BinLogRecordsCtx represents samples record with the previous data sample 
  // Producers and Consumers' streams each have one.
  class CallStatsRecordCtx
  {
  public:
    CallStatsRecord log_record {};
    CallStatsSample last_sample {};

  public:
    CallStatsRecordCtx(std::string callId, std::string objId, uint64_t objType);
    
    void AddStatsRecord(StatsBinLog* log, RTC::RtpStream* stream); // either recv or send stream

  private:
    void WriteIfFull(StatsBinLog* log); // writes out data into a log if enough records where gathered, and resets filled counter
  };


// Bin log presentation, object in Transport
  class StatsBinLog
  {
  public:
    std::string bin_log_file_path;                               // binary log full file name: combo of call id, timestamp and "version"
    std::FILE*  fd {0};                                          // the file into which samples are written once the record is full
    uint64_t    sampling_interval {CALL_STATS_BIN_LOG_SAMPLING}; // frequency of collecting samples
    
  public:
    StatsBinLog() = default;

    // TBD: review if old log should be reopened when broadcast is paused
    // TODO: add timestamp to log name
    void InitLog(std::string path)
    {
      this->bin_log_file_path.assign(path);
      this->sampling_interval = CALL_STATS_BIN_LOG_SAMPLING;
      initialized = true;
    }

    void DeinitLog(CallStatsRecordCtx* recordCtx)
    {
      if (this->fd)
      {
        this->LogClose(recordCtx);
      }
    }

    int OnLogRotateSignal(CallStatsRecordCtx* ctx, bool signal_set);
    int LogOpen();
    int LogClose(CallStatsRecordCtx* recordCtx);

  private:
    bool initialized {false};
  };
};

#endif // MS_LIVELY_BIN_LOGS_HPP