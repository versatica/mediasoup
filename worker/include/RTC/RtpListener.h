#ifndef MS_RTC_RTP_LISTENER_H
#define MS_RTC_RTP_LISTENER_H

#include "common.h"
#include "RTC/RtpReceiver.h"
#include "RTC/RtpParameters.h"
// #include "RTC/RTPPacket.h"  // TODO: yes? let's see
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class RtpListener
	{
	public:
		RtpListener();
		~RtpListener();

		Json::Value toJson();
		void AddRtpReceiver(RTC::RtpReceiver *rtpReceiver, RTC::RtpParameters* rtpParameters);
		void RemoveRtpReceiver(RTC::RtpReceiver* rtpReceiver);

	// private:
		// void RemoveRtpReceiverFromRtpListener(RTC::RtpReceiver* rtpReceiver);

	private:
		// Others.
		// - Table of SSRC / RtpReceiver pairs.
		std::unordered_map<uint32_t, RTC::RtpReceiver*> ssrcTable;
		// - Table of MID RTP header extension / RtpReceiver pairs.
		// std::unordered_map<uint16_t, RTC::RtpReceiver*> midTable;
		// - Table of RTP payload type / RtpReceiver pairs.
		std::unordered_map<uint8_t, RTC::RtpReceiver*> ptTable;
	};
}

#endif
