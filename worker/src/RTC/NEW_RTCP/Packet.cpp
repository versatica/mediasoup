#define MS_CLASS "RTC::RTCP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/Packet.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace NEW_RTCP
	{
		bool Packet::IsRtcp(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			return (
			  bufferLength >= Packet::CommonHeaderLength &&
			  // @see RFC 7983.
			  (buffer[0] > 127 && buffer[0] < 192) &&
			  // RTP Version must be 2.
			  (buffer[0] >> 6) == 2 &&
			  // RTCP packet types defined by IANA:
			  // http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4
			  // RFC 5761 (RTCP-mux) states this range for secure RTCP/RTP detection.
			  (buffer[1] >= 192 && buffer[1] <= 223) &&
			  // RTCP packets must have a length that is a multiple of 4 bytes.
			  Utils::Byte::IsPaddedTo4Bytes(bufferLength));
		}
	} // namespace NEW_RTCP
} // namespace RTC
