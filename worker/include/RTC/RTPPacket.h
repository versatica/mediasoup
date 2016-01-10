#ifndef MS_RTC_RTP_PACKET_H
#define MS_RTC_RTP_PACKET_H

#include "common.h"

namespace RTC
{
	class RTPPacket
	{
	public:
		/* Struct for RTP header. */
		struct Header
		{
			#if defined(MS_LITTLE_ENDIAN)
				MS_BYTE csrc_count:4;
				MS_BYTE extension:1;
				MS_BYTE padding:1;
				MS_BYTE version:2;
				MS_BYTE payload_type:7;
				MS_BYTE marker:1;
			#elif defined(MS_BIG_ENDIAN)
				MS_BYTE version:2;
				MS_BYTE padding:1;
				MS_BYTE extension:1;
				MS_BYTE csrc_count:4;
				MS_BYTE marker:1;
				MS_BYTE payload_type:7;
			#endif
			MS_2BYTES sequence_number;
			MS_4BYTES timestamp;
			MS_4BYTES ssrc;
		};

		/* Struct for RTP header extension. */
		struct ExtensionHeader
		{
			MS_2BYTES id;
			MS_2BYTES length;  // Size of value in multiples of 4 bytes.
			MS_BYTE* value;
		};

	public:
		static bool IsRTP(const MS_BYTE* data, size_t len);
		static RTPPacket* Parse(const MS_BYTE* data, size_t len);

	public:
		RTPPacket(Header* header, ExtensionHeader* extensionHeader, const MS_BYTE* payload, size_t payloadLen, MS_BYTE payloadPadding, const MS_BYTE* raw, size_t length);
		~RTPPacket();

		void Dump();
		const MS_BYTE* GetRaw();
		size_t GetLength();
		MS_BYTE GetPayloadType();
		void SetPayloadType(MS_BYTE payload_type);
		bool HasMarker();
		void SetMarker(bool marker);
		MS_2BYTES GetSequenceNumber();
		MS_4BYTES GetTimestamp();
		MS_4BYTES GetSSRC();
		void SetSSRC(MS_4BYTES ssrc);
		bool HasExtensionHeader();
		MS_2BYTES GetExtensionHeaderId();
		size_t GetExtensionHeaderLength();
		MS_BYTE* GetPayload();
		size_t GetPayloadLength();
		void Serialize();

	private:
		// Passed by argument.
		Header* header = nullptr;
		MS_BYTE* csrcList = nullptr;
		ExtensionHeader* extensionHeader = nullptr;
		MS_BYTE* payload = nullptr;
		size_t payloadLength = 0;
		MS_BYTE payloadPadding = 0;
		MS_BYTE* raw = nullptr;  // Allocated when Serialize().
		size_t length = 0;
		// Others.
		bool isSerialized = false;
	};

	/* Inline static methods. */

	inline
	bool RTPPacket::IsRTP(const MS_BYTE* data, size_t len)
	{
		// NOTE: RTCPPacket::IsRTCP() must always be called before this method.

		Header* header = (Header*)data;

		return (
			(len >= sizeof(Header)) &&
			// DOC: https://tools.ietf.org/html/draft-petithuguenin-avtcore-rfc5764-mux-fixes-00
			(data[0] > 127 && data[0] < 192) &&
			// RTP Version must be 2.
			(header->version == 2)
		);
	}

	/* Inline instance methods. */

	inline
	const MS_BYTE* RTPPacket::GetRaw()
	{
		return this->raw;
	}

	inline
	size_t RTPPacket::GetLength()
	{
		return this->length;
	}

	inline
	MS_BYTE RTPPacket::GetPayloadType()
	{
		return this->header->payload_type;
	}

	inline
	void RTPPacket::SetPayloadType(MS_BYTE payload_type)
	{
		this->header->payload_type = payload_type;
	}

	inline
	bool RTPPacket::HasMarker()
	{
		return this->header->marker;
	}

	inline
	void RTPPacket::SetMarker(bool marker)
	{
		this->header->marker = marker;
	}

	inline
	MS_2BYTES RTPPacket::GetSequenceNumber()
	{
		return ntohs(this->header->sequence_number);
	}

	inline
	MS_4BYTES RTPPacket::GetTimestamp()
	{
		return ntohl(this->header->timestamp);
	}

	inline
	MS_4BYTES RTPPacket::GetSSRC()
	{
		return ntohl(this->header->ssrc);
	}

	// TODO temp
	inline
	void RTPPacket::SetSSRC(MS_4BYTES ssrc)
	{
		this->header->ssrc = htonl(ssrc);
	}

	inline
	bool RTPPacket::HasExtensionHeader()
	{
		return (this->extensionHeader ? true : false);
	}

	inline
	MS_2BYTES RTPPacket::GetExtensionHeaderId()
	{
		if (this->extensionHeader)
			return MS_2BYTES(ntohs(this->extensionHeader->id));
		else
			return 0;
	}

	inline
	size_t RTPPacket::GetExtensionHeaderLength()
	{
		if (this->extensionHeader)
			return size_t(ntohs(this->extensionHeader->length) * 4);
		else
			return 0;
	}

	inline
	MS_BYTE* RTPPacket::GetPayload()
	{
		return this->payload;
	}

	inline
	size_t RTPPacket::GetPayloadLength()
	{
		return this->payloadLength;
	}
}

#endif
