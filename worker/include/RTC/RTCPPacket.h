#ifndef MS_RTC_RTCP_PACKET_H
#define MS_RTC_RTCP_PACKET_H

#include "common.h"

namespace RTC
{
	class RTCPPacket
	{
	public:
		/* Struct for RTCP common header. */
		typedef struct CommonHeader
		{
			#if defined(MS_LITTLE_ENDIAN)
				MS_BYTE count:5;
				MS_BYTE padding:1;
				MS_BYTE version:2;
			#elif defined(MS_BIG_ENDIAN)
				MS_BYTE version:2;
				MS_BYTE padding:1;
				MS_BYTE count:5;
			#endif
			MS_BYTE packet_type:8;
			MS_2BYTES length:16;
		} CommonHeader;

	public:
		static bool IsRTCP(const MS_BYTE* data, size_t len);
		static RTCPPacket* Parse(const MS_BYTE* data, size_t len);

	public:
		RTCPPacket(CommonHeader* header, const MS_BYTE* raw, size_t length);
		~RTCPPacket();

		const MS_BYTE* GetRaw();
		size_t GetLength();

	private:
		// Passed by argument:
		CommonHeader* header = nullptr;
		MS_BYTE* raw = nullptr;
		size_t length = 0;
	};

	/* Inline static methods. */

	inline
	bool RTCPPacket::IsRTCP(const MS_BYTE* data, size_t len)
	{
		CommonHeader* header = (CommonHeader*)data;

		return (
			(len >= sizeof(CommonHeader)) &&
			// DOC: https://tools.ietf.org/html/draft-petithuguenin-avtcore-rfc5764-mux-fixes-00
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
	const MS_BYTE* RTCPPacket::GetRaw()
	{
		return this->raw;
	}

	inline
	size_t RTCPPacket::GetLength()
	{
		return this->length;
	}
}  // namespace RTC

#endif
