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
    SHM_WRT_READY,                   // writer initialized and ready to be used
    SHM_WRT_CLOSED,                  // writer closed
    SHM_WRT_VIDEO_CHNL_CONF_MISSING, // when info on audio content has arrived but no video yet
    SHM_WRT_AUDIO_CHNL_CONF_MISSING, // when no info on audio is available yet, but video consumer has already started
    SHM_WRT_UNDEFINED,               // default
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
    SfuShmMapItem(const char* shm_name);
    ~SfuShmMapItem();

    ShmWriterStatus Status() const { return (this->wrt_ctx ? wrt_status : SHM_WRT_UNDEFINED); }

    uint32_t AudioSsrc() const { return (this->wrt_init.conf.channels[0].audio == 1) ? this->wrt_init.conf.channels[0].ssrc : 0; }
    uint32_t VideoSsrc() const { return (this->wrt_init.conf.channels[1].video == 1) ? this->wrt_init.conf.channels[1].ssrc : 0; }
    
    bool SetSsrcInShmConf(uint32_t ssrc, DepLibSfuShm::ShmChunkType kind); // returns true if shm writer was initialized and opened, otherwise false (error or just not enough information to initialize a writer yet)

  public:
    sfushm_av_wr_ctx_t      *wrt_ctx;
    sfushm_av_writer_init_t  wrt_init;
    ShmWriterStatus          wrt_status;
  };

public:
  // Check if shm with this name has active writer, this is needed to determine status of ShmTransport entity
  static bool IsConnected(const char* shm);

  static int WriteChunk(const char* shm_name, DepLibSfuShm::SfuShmMapItem *shmCtx, sfushm_av_frame_frag_t* data, DepLibSfuShm::ShmChunkType kind = DepLibSfuShm::ShmChunkType::UNDEFINED, uint32_t ssrc = 0);
  static int WriteStreamMetadata(const char* shm_name, DepLibSfuShm::SfuShmMapItem *shmCtx, uint8_t *data, size_t len);

	static bool IsError(int err_code);
	static const char* GetErrorString(int err_code);

  //
  static DepLibSfuShm::SfuShmMapItem* GetShmWriterCtx(const char* shm_name);
  static int ConfigureShmWriterCtx(const char* shm_name, DepLibSfuShm::SfuShmMapItem **shmCtx, DepLibSfuShm::ShmChunkType kind = DepLibSfuShm::ShmChunkType::UNDEFINED, uint32_t ssrc = 0);

private:
	static std::unordered_map<int, const char*> errToString;                               // help mapping error codes to diagnostic strings
  static std::unordered_map<const char*, DepLibSfuShm::SfuShmMapItem*> shmToWriterCtx;   // map writer contexts and shm names
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
