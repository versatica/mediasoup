#ifndef MS_RTC_RTP_PACKET_HPP
#define MS_RTC_RTP_PACKET_HPP

#include "common.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include <map>

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

	private:
		/* Struct for RTP extension header. */
		struct ExtensionHeader
		{
			uint16_t id;
			uint16_t length; // Size of value in multiples of 4 bytes.
			uint8_t value[1];
		};

	private:
		/* Struct for One-Byte extension. */
		struct OneByteExtension
		{
			#if defined(MS_LITTLE_ENDIAN)
				uint8_t len:4;
				uint8_t id:4;
			#elif defined(MS_BIG_ENDIAN)
				uint8_t id:4;
				uint8_t len:4;
			#endif
			uint8_t value[1];
		};

	private:
		/* Struct for Two-Bytes extension. */
		struct TwoBytesExtension
		{
			#if defined(MS_LITTLE_ENDIAN)
				uint8_t len:8;
				uint8_t id:8;
			#elif defined(MS_BIG_ENDIAN)
				uint8_t id:8;
				uint8_t len:8;
			#endif
			uint8_t value[1];
		};

	public:
		static bool IsRtp(const uint8_t* data, size_t len);
		static RtpPacket* Parse(const uint8_t* data, size_t len);

	public:
		RtpPacket(Header* header, ExtensionHeader* extensionHeader, const uint8_t* payload, size_t payloadLen, uint8_t payloadPadding, size_t size);
		~RtpPacket();

		void Dump() const;
		const uint8_t* GetData() const;
		size_t GetSize() const;
		uint8_t GetPayloadType() const;
		void SetPayloadType(uint8_t payload_type);
		bool HasMarker() const;
		void SetMarker(bool marker);
		uint16_t GetSequenceNumber() const;
		void SetSequenceNumber(uint16_t seq);
		uint32_t GetTimestamp() const;
		void SetTimestamp(uint32_t timestamp);
		uint32_t GetSsrc() const;
		void SetSsrc(uint32_t ssrc);
		bool HasExtensionHeader() const;
		uint16_t GetExtensionHeaderId() const;
		size_t GetExtensionHeaderLength() const;
		uint8_t* GetExtensionHeaderValue() const;
		bool HasOneByteExtensions() const;
		bool HasTwoBytesExtensions() const;
		void AddExtensionMapping(RtpHeaderExtensionUri::Type uri, uint8_t id);
		uint8_t* GetExtension(RtpHeaderExtensionUri::Type uri, uint8_t* len) const;
		uint32_t GetAbsSendTime() const;
		uint8_t* GetPayload() const;
		size_t GetPayloadLength() const;
		void Serialize(uint8_t* buffer);
		RtpPacket* Clone(uint8_t* buffer) const;

	private:
		void ParseExtensions();

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* csrcList = nullptr;
		ExtensionHeader* extensionHeader = nullptr;
		std::map<uint8_t, OneByteExtension*> oneByteExtensions;
		std::map<uint8_t, TwoBytesExtension*> twoBytesExtensions;
		std::map<RtpHeaderExtensionUri::Type, uint8_t> extensionMap;
		uint8_t* payload = nullptr;
		size_t payloadLength = 0;
		uint8_t payloadPadding = 0;
		size_t size = 0; // Full size of the packet in bytes.
	};

	/* Inline static methods. */

	inline
	bool RtpPacket::IsRtp(const uint8_t* data, size_t len)
	{
		// NOTE: RtcpPacket::IsRtcp() must always be called before this method.

		Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

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
	const uint8_t* RtpPacket::GetData() const
	{
		return (const uint8_t*)this->header;
	}

	inline
	size_t RtpPacket::GetSize() const
	{
		return this->size;
	}

	inline
	uint8_t RtpPacket::GetPayloadType() const
	{
		return this->header->payload_type;
	}

	inline
	void RtpPacket::SetPayloadType(uint8_t payload_type)
	{
		this->header->payload_type = payload_type;
	}

	inline
	bool RtpPacket::HasMarker() const
	{
		return this->header->marker;
	}

	inline
	void RtpPacket::SetMarker(bool marker)
	{
		this->header->marker = marker;
	}

	inline
	uint16_t RtpPacket::GetSequenceNumber() const
	{
		return (uint16_t)ntohs(this->header->sequence_number);
	}

	inline
	void RtpPacket::SetSequenceNumber(uint16_t seq)
	{
		this->header->sequence_number = (uint16_t)htons(seq);
	}

	inline
	uint32_t RtpPacket::GetTimestamp() const
	{
		return (uint32_t)ntohl(this->header->timestamp);
	}

	inline
	void RtpPacket::SetTimestamp(uint32_t timestamp)
	{
		this->header->timestamp = (uint32_t)htonl(timestamp);
	}

	inline
	uint32_t RtpPacket::GetSsrc() const
	{
		return (uint32_t)ntohl(this->header->ssrc);
	}

	inline
	void RtpPacket::SetSsrc(uint32_t ssrc)
	{
		this->header->ssrc = (uint32_t)htonl(ssrc);
	}

	inline
	bool RtpPacket::HasExtensionHeader() const
	{
		return (this->extensionHeader ? true : false);
	}

	inline
	uint16_t RtpPacket::GetExtensionHeaderId() const
	{
		if (!this->extensionHeader)
			return 0;

		return uint16_t(ntohs(this->extensionHeader->id));
	}

	inline
	size_t RtpPacket::GetExtensionHeaderLength() const
	{
		if (!this->extensionHeader)
			return 0;

		return size_t(ntohs(this->extensionHeader->length) * 4);
	}

	inline
	uint8_t* RtpPacket::GetExtensionHeaderValue() const
	{
		if (!this->extensionHeader)
			return nullptr;

		return this->extensionHeader->value;
	}

	inline
	bool RtpPacket::HasOneByteExtensions() const
	{
		return GetExtensionHeaderId() == 0xBEDE;
	}

	inline
	bool RtpPacket::HasTwoBytesExtensions() const
	{
		return (GetExtensionHeaderId() & 0b1111111111110000) == 0b0001000000000000;
	}

	inline
	void RtpPacket::AddExtensionMapping(RtpHeaderExtensionUri::Type uri, uint8_t id)
	{
		this->extensionMap[uri] = id;
	}

	inline
	uint8_t* RtpPacket::GetExtension(RtpHeaderExtensionUri::Type uri, uint8_t* len) const
	{
		*len = 0;

		if (this->extensionMap.find(uri) == this->extensionMap.end())
			return nullptr;

		uint8_t id = this->extensionMap.at(uri);

		if (HasOneByteExtensions())
		{
			if (this->oneByteExtensions.find(id) == this->oneByteExtensions.end())
				return nullptr;

			*len = this->oneByteExtensions.at(id)->len + 1;
			return this->oneByteExtensions.at(id)->value;
		}
		else if (HasTwoBytesExtensions())
		{
			if (this->twoBytesExtensions.find(id) == this->twoBytesExtensions.end())
				return nullptr;

			*len = this->oneByteExtensions.at(id)->len;
			return this->twoBytesExtensions.at(id)->value;
		}
		else
		{
			return nullptr;
		}
	}

	inline
	uint32_t RtpPacket::GetAbsSendTime() const
	{
		uint8_t exten_len;
		uint8_t* exten_value;

		exten_value = GetExtension(RtpHeaderExtensionUri::Type::ABS_SEND_TIME, &exten_len);

		if (!exten_value || exten_len != 3)
			return 0;

		return Utils::Byte::Get3Bytes(exten_value, 0);
	}

	inline
	uint8_t* RtpPacket::GetPayload() const
	{
		return this->payload;
	}

	inline
	size_t RtpPacket::GetPayloadLength() const
	{
		return this->payloadLength;
	}
}

#endif
