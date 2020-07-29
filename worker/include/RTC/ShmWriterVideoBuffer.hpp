#ifndef MS_RTC_SHM_WRITER_VIDEO_BUFFER_HPP
#define MS_RTC_SHM_WRITER_VIDEO_BUFFER_HPP

#include "RTC/RtpPacket.hpp"
#include <vector>

namespace RTC
{
	class ShmWriterVideoBuffer // public RTC::RtpStreamSend::Listener
	{
	public:
  		struct VideoBufferItem
		{
			// Cloned packet. TODO: maybe null if currently missing
			RTC::RtpPacket* packet{ nullptr };
			// Memory to hold the cloned packet (with extra space for RTX encoding).
			uint8_t store[RTC::MtuSize + 100];
      //if it is key frame (or part of key frame in case of fragmented pic)
      bool keyFrame;
      bool picBegin;
      bool picEnd;
      bool NaluBegin;
      bool NaluEnd;
      // written out to shm or discarded; either way, this item can now be removed from the buffer
      bool done;
      //TODO: keep timestamps and seqIds separately, or rely on packet field?
		};
    //ShmWriterVideoBuffer();

  private:
    uint64_t oldestTs;
    uint64_t newestTs;
    uint16_t oldestSeqId;
    uint16_t newestSeqId;
    uint32_t ssrc;
    std::vector<VideoBufferItem> buffer; // TODO: maybe map by seqId? 
  };
}

#endif //MS_RTC_SHM_WRITER_VIDEO_BUFFER_HPP