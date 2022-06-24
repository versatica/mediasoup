#ifndef MS_LIVELY_BIN_LOGS_HPP
#define MS_LIVELY_BIN_LOGS_HPP

#include "common.hpp"
#include <cstring>
#include "RTC/RtpStream.hpp"

#define BINLOG_MIN_TIMESPAN   20000
#define BINLOG_FORMAT_VERSION "e58c1e"

// CALL_STATS_BIN_LOG_RECORDS_NUM * sizeof(CallStatsSample) should be
// dividible by 16 b/c of alignment concerns;
// otherwise, there will be random sized padding added
// at the end of the array of records to align
// ConsumerRecord and ProducerRecord structs to 16 bytes.
// sizeof(CallStatsSample)== 28 bytes, CALL_STATS_BIN_LOG_RECORDS_NUM should be divisible by 4.
// Alternative is to increase it to 32 at a cost of wasting 4 bytes per data record.

#define CALL_STATS_BIN_LOG_RECORDS_NUM 8
#define CALL_STATS_BIN_LOG_SAMPLING    2000

#define UINT32_UNSET                   ((uint32_t)-1)
#define UINT64_UNSET                   ((uint64_t)-1)
#define ZERO_UUID "00000000-0000-0000-0000-000000000000"
#define UUID_BYTE_LEN 16
#define UUID_CHAR_LEN 36

namespace Lively
{
struct StreamStats;
class StatsBinLog;

// Data sample
struct CallStatsSample
{
  uint16_t epoch_len;             // epoch duration in milliseconds counting from the previous sample
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

// Record headers are aligned to 16 bytes.
// timestamp filled payload
// 8         4      4
// timestamp filled payload consumer_uuid producer_uuid
// 8          4     4       16            16           
struct ConsumerRecord
{
  uint64_t        start_tm {UINT64_UNSET};                   // the record start timestamp in milliseconds
  uint32_t        filled {UINT32_UNSET};                      // number of filled records in the array below
  uint32_t        payload {0};                     // payload as in original RTP stream
  uint8_t         consumer_id [UUID_BYTE_LEN]; // 
  uint8_t         producer_id [UUID_BYTE_LEN]; //
  CallStatsSample samples[CALL_STATS_BIN_LOG_RECORDS_NUM]; // collection of data samples
};

struct ProducerRecord
{
  uint64_t        start_tm {UINT64_UNSET};                   // the record start timestamp in milliseconds
  uint32_t        filled {UINT32_UNSET};                      // number of filled records in the array below
  uint32_t        payload {0};                     // payload as in original RTP stream
  CallStatsSample samples[CALL_STATS_BIN_LOG_RECORDS_NUM]; // collection of data samples
};

class CallStatsRecord
{
  public:
    uint8_t     type {0};     // 0 for producer or 1 for consumer
    std::string call_id;      // call id: uuid4() string
    std::string object_id;    // uuid4(), producer or consumer id, depending on source
    std::string producer_id;  // uuid4(), undef if source is producer, or consumer's corresponding producer id

  public:
    CallStatsRecord(uint64_t objType, uint8_t payload, std::string callId, std::string objId, std::string producerId);

    bool fwriteRecord(std::FILE* fd);
    uint32_t filled() const {return type ? record.c.filled : record.p.filled; }
    void resetSamples();
    void zeroSamples(uint64_t nowMs);
    bool addSample(StreamStats& last, StreamStats& curr);    

  private:
    // Binary data record
    union Record
    {
      ConsumerRecord c;
      ProducerRecord p;
      Record() { memset( this, 0, sizeof( Record ) ); }
    } record;

    bool uuidToBytes(std::string uuid, uint8_t *out);
    uint8_t* hexStrToBytes(const char* from, int num, uint8_t *to);
  };

  struct StreamStats
  {
    uint64_t ts {UINT64_UNSET}; // ts when data was received from a stream        
    size_t packetsCount {0};
    size_t bytesCount {0};
    uint32_t packetsLost {0};
    size_t packetsDiscarded {0};
    size_t packetsRetransmitted {0};
    size_t packetsRepaired {0};
    size_t nackCount {0};
    size_t nackPacketCount {0};
    size_t kfCount {0};
    float rtt {0};
    uint32_t maxPacketTs {0};
  };

  class CallStatsRecordCtx
  {
  public:
    CallStatsRecord record;
  
  private:
    StreamStats last {}; // before very first sample is recorded, ts is unset, then it will always be set into some valid time
    StreamStats curr {};

  public:
    CallStatsRecordCtx(uint64_t objType, uint8_t payload, std::string callId, std::string objId, std::string producerId) : record(objType, payload, callId, objId, producerId) {}
    void AddStatsRecord(StatsBinLog* log, RTC::RtpStream* stream); // either recv or send stream
    uint64_t LastTs() const { return last.ts; }
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
    std::string   bin_log_name_template;          // Log name template, use to rotate log, keep same name except for timestamp
    const char    version[7] = BINLOG_FORMAT_VERSION;

    uint64_t      log_start_ts {UINT64_UNSET};    // Timestamp included into log's name; used to rotate logs daily
    uint64_t      log_last_ts {UINT64_UNSET};     // Timestamp of the last record in the log; various sources may share same logfile   

  public:
    StatsBinLog() = default;

    void InitLog(char type, std::string id1, std::string id2); // if type is producer, then log name is a combo of callid, producerid and timestamp
    int OnLogWrite(CallStatsRecordCtx* ctx);
    void DeinitLog();   // Closes log file and deinitializes state variables

  private:
    int LogOpen();
    int LogClose();
    void UpdateLogName();
    bool CreateBinlogDirsIfMissing();
  };
} //Lively

#endif // MS_LIVELY_BIN_LOGS_HPP