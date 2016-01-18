#ifndef MS_RTC_RTP_LISTENER_H
#define MS_RTC_RTP_LISTENER_H

#include "common.h"
#include "RTC/RTPPacket.h"
#include "RTC/RtpReceiver.h"
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class RtpListener :
		public RTC::RtpReceiver::RtpListener
	{
	public:
		RtpListener();
		~RtpListener();

		Json::Value toJson();

	private:
		void RemoveRtpReceiverFromRtpListener(RTC::RtpReceiver* rtpReceiver);

	public:
		/* Pure virtual methods inherited from RTC::RtpReceiver::RtpListener. */
		virtual void onRtpListenerParameters(RTC::RtpReceiver* rtpReceiver, RTC::RtpParameters* rtpParameters) override;
		virtual void onRtpReceiverClosed(RTC::RtpReceiver* rtpReceiver) override;

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
