#ifndef DEP_LIBSFUSHM_HPP
#define DEP_LIBSFUSHM_HPP

#include <sfushm_av_media.h>
#include <vector>
#include <string>
#include <unordered_map>

class DepLibSfuShm
{
public:
  enum ShmWriterStatus {
    SHM_WRT_UNDEFINED = 1,               // default
    SHM_WRT_CLOSED,                  // writer closed
    SHM_WRT_VIDEO_CHNL_CONF_MISSING, // when info on audio content has arrived but no video yet
    SHM_WRT_AUDIO_CHNL_CONF_MISSING, // when no info on audio is available yet, but video consumer has already started
    SHM_WRT_READY                   // writer initialized and ready to be used
    // TODO: do I need an explicit value for a writer in error state?
  };

  enum ShmChunkType {
    VIDEO,
    AUDIO,
    RTCP,
    UNDEFINED
  };

  // Contains shm configuration, writer context (if initialized), writer status 
  class SfuShmMapItem {
  public:
    SfuShmMapItem(): wrt_ctx(nullptr), wrt_status(SHM_WRT_UNDEFINED) {}
    ~SfuShmMapItem();

    ShmWriterStatus Status() const { return (this->wrt_ctx != nullptr ? this->wrt_status : SHM_WRT_UNDEFINED); }

    uint32_t AudioSsrc() const { return (this->wrt_init.conf.channels[0].audio == 1) ? this->wrt_init.conf.channels[0].ssrc : 0; }
    uint32_t VideoSsrc() const { return (this->wrt_init.conf.channels[1].video == 1) ? this->wrt_init.conf.channels[1].ssrc : 0; }
    
    ShmWriterStatus SetSsrcInShmConf(uint32_t ssrc, DepLibSfuShm::ShmChunkType kind);

  public:
    std::string              stream_name;
    sfushm_av_wr_ctx_t      *wrt_ctx;
    sfushm_av_writer_init_t  wrt_init;
    ShmWriterStatus          wrt_status;
  };

public:
  // Check if shm with this name has active writer, this is needed to determine status of ShmTransport entity
  //static bool IsConnected(const char* shm);

  static int WriteChunk(DepLibSfuShm::SfuShmMapItem *shmCtx, sfushm_av_frame_frag_t* data, DepLibSfuShm::ShmChunkType kind = DepLibSfuShm::ShmChunkType::UNDEFINED, uint32_t ssrc = 0);
  static int WriteStreamMetadata(DepLibSfuShm::SfuShmMapItem *shmCtx, uint8_t *data, size_t len);

	static bool IsError(int err_code);
	static const char* GetErrorString(int err_code);
  
  static void InitializeShmWriterCtx(std::string shm_name, std::string log_name, int log_level, DepLibSfuShm::SfuShmMapItem *shmCtx);
  static ShmWriterStatus ConfigureShmWriterCtx(DepLibSfuShm::SfuShmMapItem *shmCtx, DepLibSfuShm::ShmChunkType kind, uint32_t ssrc);

private:
	static std::unordered_map<int, const char*> errToString; // help mapping error codes to diagnostic strings

  //static std::unordered_map<const char*, DepLibSfuShm::SfuShmMapItem*> c;
};


inline bool DepLibSfuShm::IsError(int err_code)
{
	return (err_code != SFUSHM_AV_OK && err_code != SFUSHM_AV_AGAIN);
}


inline const char* DepLibSfuShm::GetErrorString(int err_code)
{
  static const char* unknownErr = "unknown SFUSHM error";

  auto it = DepLibSfuShm::errToString.find(err_code);

  if (it == DepLibSfuShm::errToString.end())
    return unknownErr;

  return it->second;
}

#endif // DEP_LIBSFUSHM_HPP
