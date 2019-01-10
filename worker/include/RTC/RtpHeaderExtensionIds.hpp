#ifndef MS_RTC_RTP_HEADER_EXTENSION_IDS
#define MS_RTC_RTP_HEADER_EXTENSION_IDS

namespace RTC
{
	// RTP header extension ids that must be shared by all Producers using
	// the same Transport.
	// NOTE: These ids are the original ids in the RTP packet (before the Producer
	// maps them to the corresponding ids in the Router).
	struct HeaderExtensionIds
	{
		uint8_t ssrcAudioLevel{ 0 }; // 0 means no ssrc-audio-level id.
		uint8_t absSendTime{ 0 };    // 0 means no abs-send-time id.
		uint8_t mid{ 0 };            // 0 means no MID id.
		uint8_t rid{ 0 };            // 0 means no RID id.
	};
} // namespace RTC

#endif
