#ifndef MS_RTC_RTP_STREAM_H
#define MS_RTC_RTP_STREAM_H

#include "common.h"
#include "RTC/RtpPacket.h"
#include <vector>
#include <list>

namespace RTC
{
	class RtpStream
	{
	private:
		struct StorageItem
		{
			uint8_t store[65536];
		};

	public:
		RtpStream(size_t bufferSize);
		~RtpStream();

		bool AddPacket(RTC::RtpPacket* packet);

	private:
		std::vector<StorageItem> storage;
		typedef std::list<RTC::RtpPacket*> Packets;
		Packets packets;
	};
}

#endif
