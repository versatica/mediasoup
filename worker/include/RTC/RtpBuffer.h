#ifndef MS_RTC_RTP_BUFFER_H
#define MS_RTC_RTP_BUFFER_H

#include "common.h"
#include "RTC/RtpPacket.h"
#include <vector>

#define MS_RTP_PACKET_MAX_SIZE 65536

namespace RTC
{
	class RtpBuffer
	{
	public:
		struct Item
		{
			uint8_t         raw[MS_RTP_PACKET_MAX_SIZE];
			RTC::RtpPacket* packet = nullptr;
		};

	public:
		RtpBuffer(size_t capability);
		virtual ~RtpBuffer();

		void Add(RTC::RtpPacket* packet);

	private:
		std::vector<Item> items;
		size_t pos = 0;
		bool full = false;
	};
}

#endif
