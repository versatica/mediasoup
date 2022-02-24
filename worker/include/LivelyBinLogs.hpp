#ifndef MS_LIVELY_BIN_LOGS_HPP
#define MS_LIVELY_BIN_LOGS_HPP

#include "common.hpp"
#include <cstring>
#include "RTC/RtpStream.hpp"


#define CALL_STATS_BIN_LOG_RECORDS_NUM 30
#define CALL_STATS_BIN_LOG_SAMPLING    2000
#define UINT16_UNSET                   ((uint16_t)-1)
#define UINT64_UNSET                   ((uint64_t)-1)
#define ZERO_UUID "00000000-0000-0000-0000-000000000000"


namespace Lively
{
  // Data sample
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

  // Data record: a header followed array of data samples
  struct CallStatsRecord
  {
    uint64_t          start_tm;                                // the record start timestamp in milliseconds
    char              call_id[36];                             // call id: uuid4() without \0
    char              object_id[36];                           // uuid4() no \0, producer or consumer id, depending on source
    char              producer_id[36];                         // uuid4() no \0, ZERO_UUID if source is producer, or consumer's corresponding producer id
    uint8_t           source {0};                              // 0 for producer or 1 for consumer
    uint8_t           mime {0};                                // mime type: unset, audio, video
    uint16_t          filled {UINT16_UNSET};                   // number of filled records in the array below
    CallStatsSample   samples[CALL_STATS_BIN_LOG_RECORDS_NUM]; // collection of data samples
  };


  class StatsBinLog;


  // Data collecting management:
  // Collects data samples,
  // forms data records,
  // dumps data out into a file
  class CallStatsRecordCtx
  {
  public:
    CallStatsRecord record {};
  
  private:
    class InputStats
    {
    public:
      size_t packetsCount;
      size_t bytesCount;
      uint32_t packetsLost;
      size_t packetsDiscarded;
      size_t packetsRetransmitted;
      size_t packetsRepaired;
      size_t nackCount;
      size_t nackPacketCount;
      size_t kfCount;
      float rtt;
      uint32_t maxPacketTs;
    };

    InputStats last {};
    InputStats curr {};

  public:
    CallStatsRecordCtx(uint64_t objType, std::string callId, std::string objId, std::string producerId);

    void AddStatsRecord(StatsBinLog* log, RTC::RtpStream* stream); // either recv or send stream

  private:
    void WriteIfFull(StatsBinLog* log);
  };


  // Binary log presentation
  class StatsBinLog
  {
  public:
    std::string   bin_log_file_path;                               // binary log's full file name: combo of call id, timestamp and "version"
    std::FILE*    fd {0};                                          
    uint64_t      sampling_interval {CALL_STATS_BIN_LOG_SAMPLING}; // frequency of collecting samples, non-configurable
  
  private:
    bool          initialized {false};
    std::string   bin_log_name_template;       // Log name template, use to rotate log, keep same name except for timestamp
    uint64_t      log_start_ts {UINT64_UNSET}; // Timestamp included into log's name; used to rotate logs daily

  public:
    StatsBinLog() = default;

    int LogOpen();
    int OnLogWrite(CallStatsRecordCtx* ctx);
    int LogClose(CallStatsRecordCtx* recordCtx);

    void InitLog(char type, std::string id1, std::string id2); // if type is producer, then 
    void DeinitLog(CallStatsRecordCtx* recordCtx);  // Closes log file and deinitializes state variables

  private:
    void UpdateLogName();
  };
} //Lively

#endif // MS_LIVELY_BIN_LOGS_HPP