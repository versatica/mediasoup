#ifndef MS_RTC_RTCP_PACKET_H
#define MS_RTC_RTCP_PACKET_H

#include "common.h"

namespace RTC
{
	class RtcpPacket
	{
	public:
		/* Struct for RTCP common header. */
		struct CommonHeader
		{
			#if defined(MS_LITTLE_ENDIAN)
				uint8_t count:5;
				uint8_t padding:1;
				uint8_t version:2;
			#elif defined(MS_BIG_ENDIAN)
				uint8_t version:2;
				uint8_t padding:1;
				uint8_t count:5;
			#endif
			uint8_t packet_type:8;
			uint16_t length:16;
		};

	public:
		static bool IsRtcp(const uint8_t* data, size_t len);
		static RtcpPacket* Parse(const uint8_t* data, size_t len);

	public:
		RtcpPacket(CommonHeader* header, const uint8_t* raw, size_t length);
		~RtcpPacket();

		const uint8_t* GetRaw();
		size_t GetLength();

	private:
		// Passed by argument.
		CommonHeader* header = nullptr;
		uint8_t* raw = nullptr;
		size_t length = 0;
	};

	/* Inline static methods. */

	inline
	bool RtcpPacket::IsRtcp(const uint8_t* data, size_t len)
	{
		CommonHeader* header = (CommonHeader*)data;

		return (
			(len >= sizeof(CommonHeader)) &&
			// DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
			(data[0] > 127 && data[0] < 192) &&
			// RTP Version must be 2.
			(header->version == 2) &&
			// RTCP packet types defined by IANA:
			// http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4
			// RFC 5761 (RTCP-mux) states this range for secure RTCP/RTP detection.
			(header->packet_type >= 192 && header->packet_type <= 223)
		);
	}

	/* Inline instance methods. */

	inline
	const uint8_t* RtcpPacket::GetRaw()
	{
		return this->raw;
	}

	inline
	size_t RtcpPacket::GetLength()
	{
		return this->length;
	}
}

#endif
