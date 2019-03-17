#ifndef MS_RTC_RTP_HEADER_EXTENSION_IDS
#define MS_RTC_RTP_HEADER_EXTENSION_IDS

namespace RTC
{
	// RTP header extension ids. Some of these are shared by all Producers using
	// the same Transport. Others are different for each Producer.
	struct RtpHeaderExtensionIds
	{
		uint8_t ssrcAudioLevel{ 0u };   // 0 means no ssrc-audio-level id.
		uint8_t videoOrientation{ 0u }; // 0 means no video-orientation id.
		uint8_t absSendTime{ 0u };      // 0 means no abs-send-time id.
		uint8_t mid{ 0u };              // 0 means no MID id.
		uint8_t rid{ 0u };              // 0 means no RID id.
		uint8_t rrid{ 0u };             // 0 means no RRID id.
	};
} // namespace RTC

#endif
