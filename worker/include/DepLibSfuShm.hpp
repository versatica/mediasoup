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
    SHM_WRT_INITIALIZED,
    SHM_WRT_CLOSED,
    SHM_WRT_UNDEFINED    
  };

  enum ShmChunkType {
    VIDEO,
    AUDIO,
    RTCP
  };

  // Contains shm configuration, writer context (if initialized), writer status 
  class SfuShmMapItem {
  public:
    SfuShmMapItem() = default;
    SfuShmMapItem(const char* shm_name);
    ~SfuShmMapItem();

    ShmWriterStatus status() { return wrt_status; }
  public:
    sfushm_av_writer_init_t  wrt_init;
    sfushm_av_wr_ctx_t      *wrt_ctx;
    ShmWriterStatus          wrt_status;
  };

public:
  // Find active writer for this shm, write data in, return non-0 error in case smth isn't quite right 
  static int WriteChunk(std::string shm_name, sfushm_av_frame_frag_t* data, DepLibSfuShm::ShmChunkType kind);
  static int WriteStreamMetadata(const char* shm, uint8_t *data, size_t len);

	static bool IsError(int err_code);
	static const char* GetErrorString(int err_code);

private:
  static int GetShmWriterCtx(const char* shm_name, sfushm_av_wr_ctx_t *ctx_out);

private:
  // help mapping error codes to diagnostic strings
	static std::unordered_map<int, const char*> errToString;
  // map writer contexts and shm names
  static std::unordered_map<std::string, DepLibSfuShm::SfuShmMapItem*> shmToWriterCtx;
};


inline bool DepLibSfuShm::IsError(int err_code)
{
	return (err_code != SFUSHM_AV_OK && err_code != SFUSHM_AV_AGAIN);
}


inline const char* DepLibSfuShm::GetErrorString(int err_code)
{
  static const std::string Unknown("unknown SFUSHM error");

  auto it = DepLibSfuShm::errToString.find(err_code);

  if (it == DepLibSfuShm::errToString.end())
    return Unknown.c_str();

  return it->second;
}

#endif // DEP_LIBSFUSHM_HPP
