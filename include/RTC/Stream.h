#ifndef MS_RTC_STREAM_H
#define MS_RTC_STREAM_H


#include "common.h"
#include "RTC/RTPPacket.h"
#include "RTC/RTCPPacket.h"
#include <unordered_set>


namespace RTC {


class Stream {
public:
	Stream();
	~Stream();

	void Close();

private:
	MS_4BYTES ssrc = 0;
	std::unordered_set<MS_BYTE> payloads;
};


}  // namespace RTC


#endif
