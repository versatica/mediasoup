#ifndef MS_RTC_RTP_PACKET_H
#define MS_RTC_RTP_PACKET_H

#include "common.h"

namespace RTC
{
	class RtpPacket
	{
	public:
		/* Struct for RTP header. */
		struct Header
		{
			#if defined(MS_LITTLE_ENDIAN)
				uint8_t csrc_count:4;
				uint8_t extension:1;
				uint8_t padding:1;
				uint8_t version:2;
				uint8_t payload_type:7;
				uint8_t marker:1;
			#elif defined(MS_BIG_ENDIAN)
				uint8_t version:2;
				uint8_t padding:1;
				uint8_t extension:1;
				uint8_t csrc_count:4;
				uint8_t marker:1;
				uint8_t payload_type:7;
			#endif
			uint16_t sequence_number;
			uint32_t timestamp;
			uint32_t ssrc;
		};

		/* Struct for RTP header extension. */
		struct ExtensionHeader
		{
			uint16_t id;
			uint16_t length;  // Size of value in multiples of 4 bytes.
			uint8_t* value;
		};

	public:
		static bool IsRtp(const uint8_t* data, size_t len);
		static RtpPacket* Parse(const uint8_t* data, size_t len);

	public:
		RtpPacket(Header* header, ExtensionHeader* extensionHeader, const uint8_t* payload, size_t payloadLen, uint8_t payloadPadding, const uint8_t* raw, size_t length);
		~RtpPacket();

		void Dump();
		const uint8_t* GetRaw();
		size_t GetLength();
		uint8_t GetPayloadType();
		void SetPayloadType(uint8_t payload_type);
		bool HasMarker();
		void SetMarker(bool marker);
		uint16_t GetSequenceNumber();
		uint32_t GetTimestamp();
		uint32_t GetSsrc();
		void SetSsrc(uint32_t ssrc);
		bool HasExtensionHeader();
		uint16_t GetExtensionHeaderId();
		size_t GetExtensionHeaderLength();
		uint8_t* GetPayload();
		size_t GetPayloadLength();
		void Serialize();

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* csrcList = nullptr;
		ExtensionHeader* extensionHeader = nullptr;
		uint8_t* payload = nullptr;
		size_t payloadLength = 0;
		uint8_t payloadPadding = 0;
		uint8_t* raw = nullptr;  // Allocated when Serialize().
		size_t length = 0;
		// Others.
		bool isSerialized = false;
	};

	/* Inline static methods. */

	inline
	bool RtpPacket::IsRtp(const uint8_t* data, size_t len)
	{
		// NOTE: RtcpPacket::IsRtcp() must always be called before this method.

		Header* header = (Header*)data;

		return (
			(len >= sizeof(Header)) &&
			// DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
			(data[0] > 127 && data[0] < 192) &&
			// RTP Version must be 2.
			(header->version == 2)
		);
	}

	/* Inline instance methods. */

	inline
	const uint8_t* RtpPacket::GetRaw()
	{
		// Return pointer to the serialized data or to the original raw data.
		return this->raw ? this->raw : (const uint8_t*)this->header;
	}

	inline
	size_t RtpPacket::GetLength()
	{
		return this->length;
	}

	inline
	uint8_t RtpPacket::GetPayloadType()
	{
		return this->header->payload_type;
	}

	inline
	void RtpPacket::SetPayloadType(uint8_t payload_type)
	{
		this->header->payload_type = payload_type;
	}

	inline
	bool RtpPacket::HasMarker()
	{
		return this->header->marker;
	}

	inline
	void RtpPacket::SetMarker(bool marker)
	{
		this->header->marker = marker;
	}

	inline
	uint16_t RtpPacket::GetSequenceNumber()
	{
		return (uint16_t)ntohs(this->header->sequence_number);
	}

	inline
	uint32_t RtpPacket::GetTimestamp()
	{
		return (uint32_t)ntohl(this->header->timestamp);
	}

	inline
	uint32_t RtpPacket::GetSsrc()
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	void RtpPacket::SetSsrc(uint32_t ssrc)
	{
		this->header->ssrc = (uint32_t)htonl(ssrc);
	}

	inline
	bool RtpPacket::HasExtensionHeader()
	{
		return (this->extensionHeader ? true : false);
	}

	inline
	uint16_t RtpPacket::GetExtensionHeaderId()
	{
		if (this->extensionHeader)
			return uint16_t(ntohs(this->extensionHeader->id));
		else
			return 0;
	}

	inline
	size_t RtpPacket::GetExtensionHeaderLength()
	{
		if (this->extensionHeader)
			return size_t(ntohs(this->extensionHeader->length) * 4);
		else
			return 0;
	}

	inline
	uint8_t* RtpPacket::GetPayload()
	{
		return this->payload;
	}

	inline
	size_t RtpPacket::GetPayloadLength()
	{
		return this->payloadLength;
	}
}

#endif
