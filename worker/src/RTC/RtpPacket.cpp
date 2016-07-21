#define MS_CLASS "RTC::RtpPacket"

#include "RTC/RtpPacket.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
	/* Class methods. */

	RtpPacket* RtpPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!RtpPacket::IsRtp(data, len))
			return nullptr;

		// Get the header.
		Header* header = (Header*)data;

		// Inspect data after the minimum header size.
		size_t pos = sizeof(Header);

		// Check CSRC list.
		size_t csrc_list_size = 0;

		if (header->csrc_count)
		{
			csrc_list_size = header->csrc_count * sizeof(header->ssrc);

			// Packet size must be >= header size + CSRC list.
			if (len < pos + csrc_list_size)
			{
				MS_DEBUG("not enough space for the announced CSRC list, packet discarded");

				return nullptr;
			}
			pos += csrc_list_size;
		}

		// Check header extension.
		ExtensionHeader* extensionHeader = nullptr;
		size_t extension_value_size = 0;

		if (header->extension)
		{
			// The extension header is at least 4 bytes.
			if (len < pos + 4)
			{
				MS_DEBUG("not enough space for the announced extension header, packet discarded");

				return nullptr;
			}

			extensionHeader = (ExtensionHeader*)(data + pos);

			// The header extension contains a 16-bit length field that counts the number of
			// 32-bit words in the extension, excluding the four-octet extension header.
			extension_value_size = (size_t)(ntohs(extensionHeader->length) * 4);

			// Packet size must be >= header size + CSRC list + header extension size.
			if (len < pos + 4 + extension_value_size)
			{
				MS_DEBUG("not enough space for the announced header extension value, packet discarded");

				return nullptr;
			}
			pos += 4 + extension_value_size;
		}

		// Get payload.
		uint8_t* payload = (uint8_t*)data + pos;
		size_t payloadLength = len - pos;
		uint8_t payloadPadding = 0;

		MS_ASSERT(len >= pos, "payload has negative size");

		// Check padding field.
		if (header->padding)
		{
			// Must be at least a single payload byte.
			if (payloadLength == 0)
			{
				MS_DEBUG("padding bit is set but no space for a padding byte, packet discarded");

				return nullptr;
			}

			payloadPadding = data[len - 1];
			if (payloadPadding == 0)
			{
				MS_DEBUG("padding byte cannot be 0, packet discarded");

				return nullptr;
			}

			if (payloadLength < (size_t)payloadPadding)
			{
				MS_DEBUG("number of padding octets is greater than available space for payload, packet discarded");

				return nullptr;
			}
			payloadLength -= (size_t)payloadPadding;
		}

		if (payloadLength == 0)
			payload = nullptr;

		MS_ASSERT(len == sizeof(Header) + csrc_list_size + (extensionHeader ? 4 + extension_value_size : 0) + payloadLength + (size_t)payloadPadding, "packet's computed length does not match received length");

		return new RtpPacket(header, extensionHeader, payload, payloadLength, payloadPadding, data, len);
	}

	/* Instance methods. */

	RtpPacket::RtpPacket(Header* header, ExtensionHeader* extensionHeader, const uint8_t* payload, size_t payloadLength, uint8_t payloadPadding, const uint8_t* raw, size_t length) :
		header(header),
		extensionHeader(extensionHeader),
		payload((uint8_t*)payload),
		payloadLength(payloadLength),
		payloadPadding(payloadPadding),
		raw((uint8_t*)raw),
		length(length)
	{
		MS_TRACE();

		if (this->header->csrc_count)
			this->csrcList = (uint8_t*)raw + sizeof(Header);
	}

	RtpPacket::~RtpPacket()
	{
		MS_TRACE();

		if (this->isSerialized)
			delete this->raw;
	}

	void RtpPacket::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		MS_DEBUG("<RtpPacket>");
		MS_DEBUG("padding: %s", this->header->padding ? "true" : "false");
		MS_DEBUG("extension: %s", HasExtensionHeader() ? "true" : "false");
		if (HasExtensionHeader())
		{
			MS_DEBUG("extension ID: %" PRIu16, GetExtensionHeaderId());
			MS_DEBUG("extension length: %zu bytes", GetExtensionHeaderLength());
		}
		MS_DEBUG("CSRC count: %" PRIu8, this->header->csrc_count);  // TODO: method?
		MS_DEBUG("marker: %s", HasMarker() ? "true" : "false");
		MS_DEBUG("payload type: %" PRIu8, GetPayloadType());
		MS_DEBUG("sequence number: %" PRIu16, GetSequenceNumber());
		MS_DEBUG("timestamp: %" PRIu32, GetTimestamp());
		MS_DEBUG("SSRC: %" PRIu32, GetSsrc());
		MS_DEBUG("payload size: %zu bytes", GetPayloadLength());
		MS_DEBUG("</RtpPacket>");
	}

	void RtpPacket::Serialize()
	{
		MS_TRACE();

		if (this->isSerialized)
			delete this->raw;

		// Set this->isSerialized so the destructor will free the buffer.
		this->isSerialized = true;

		// Some useful variables.
		size_t extension_value_size = 0;

		// First calculate the total required size for the entire message.
		this->length = sizeof(Header);  // Minimum header.

		if (this->csrcList)
			this->length += this->header->csrc_count * sizeof(header->ssrc);

		if (this->extensionHeader)
		{
			extension_value_size = (size_t)(ntohs(this->extensionHeader->length) * 4);
			this->length += 4 + extension_value_size;
		}

		this->length += this->payloadLength;

		this->length += (size_t)this->payloadPadding;

		// Allocate it.
		this->raw = new uint8_t[this->length];

		// Add minimum header.
		std::memcpy(this->raw, this->header, sizeof(Header));
		size_t pos = sizeof(Header);

		// Update the header pointer.
		this->header = (Header*)(this->raw);

		// Add CSRC list.
		if (this->csrcList)
		{
			std::memcpy(this->raw + pos, this->csrcList, this->header->csrc_count * sizeof(header->ssrc));

			// Update the pointer.
			this->csrcList = this->raw + pos;
			pos += this->header->csrc_count * sizeof(header->ssrc);
		}

		// Add extension header.
		if (this->extensionHeader)
		{
			std::memcpy(this->raw + pos, this->extensionHeader, 4 + extension_value_size);

			// Update the header extension pointer.
			this->extensionHeader = (ExtensionHeader*)(this->raw + pos);
			pos += 4 + extension_value_size;
		}

		// Add payload.
		if (this->payload)
		{
			std::memcpy(this->raw + pos, this->payload, this->payloadLength);

			// Update the payload pointer.
			this->payload = this->raw + pos;
			pos += this->payloadLength;
		}

		// Add payload padding.
		if (this->payloadPadding)
		{
			this->raw[pos + (size_t)this->payloadPadding - 1] = this->payloadPadding;
			pos += (size_t)this->payloadPadding;
		}

		MS_ASSERT(pos == this->length, "pos != this->length");
	}
}
