#ifndef MS_RTC_RTP_EXTENSION_MAPPING_HPP
#define MS_RTC_RTP_EXTENSION_MAPPING_HPP

#include "common.hpp"

namespace RTC
{
	namespace RTP
	{
		// Mapping of RTP extension types and their corresponding RTP extension
		// ids. Some of these are shared by all Producers using the same Transport.
		// Others are different for each Producer.
		struct ExtensionMapping
		{
			// 0 means no id.
			uint8_t mid{ 0 };
			uint8_t rid{ 0 };
			uint8_t rrid{ 0 };
			uint8_t absSendTime{ 0 };
			uint8_t transportWideCc01{ 0 };
			uint8_t ssrcAudioLevel{ 0 };
			uint8_t dependencyDescriptor{ 0 };
			uint8_t videoOrientation{ 0 };
			uint8_t absCaptureTime{ 0 };
			uint8_t playoutDelay{ 0 };
			uint8_t mediasoupPacketId{ 0 };
		};
	} // namespace RTP
} // namespace RTC

#endif
