#ifndef MS_RTC_RTP_LISTENER_H
#define MS_RTC_RTP_LISTENER_H

#include "common.h"
#include "RTC/RtpReceiver.h"
#include "RTC/RTPPacket.h"
#include <unordered_map>

namespace RTC
{
	class RtpListener
	{
	public:
		RtpListener();
		virtual ~RtpListener();

		void Close();
		void AddRtpReceiver(RTC::RtpReceiver* rtpReceiver);
		RTC::RtpReceiver* GetRtpReceiver(RTC::RTPPacket* rtpPacket);

	private:
		// Others.
		// - Map of rtpReceiverId / RtpReceiver pairs.
		std::unordered_map<uint32_t, RTC::RtpReceiver*> rtpReceivers;
		// - Table of SSRC / RtpReceiver pairs.
		std::unordered_map<MS_4BYTES, RTC::RtpReceiver*> ssrcTable;
		// - Table of MID RTP header extension / RtpReceiver pairs.
		// std::unordered_map<MS_2BYTES, RTC::RtpReceiver*> midTable;
		// - Table of RTP payload type / RtpReceiver pairs.
		std::unordered_map<MS_BYTE, RTC::RtpReceiver*> ptTable;
	};
}

#endif
