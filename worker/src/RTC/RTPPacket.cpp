#define MS_CLASS "RTC::RTPPacket"

#include "RTC/RTPPacket.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
	/* Static methods. */

	RTPPacket* RTPPacket::Parse(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!RTPPacket::IsRTP(data, len))
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
				MS_DEBUG("not enough space for the announced CSRC list | packet discarded");
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
				MS_DEBUG("not enough space for the announced extension header | packet discarded");
				return nullptr;
			}

			extensionHeader = (ExtensionHeader*)(data + pos);

			// The header extension contains a 16-bit length field that counts the number of
			// 32-bit words in the extension, excluding the four-octet extension header.
			// extension_value_size = (size_t)(Utils::Byte::Get2Bytes(data, pos + 2) * 4);
			extension_value_size = (size_t)(ntohs(extensionHeader->length) * 4);

			// Packet size must be >= header size + CSRC list + header extension size.
			if (len < pos + 4 + extension_value_size)
			{
				MS_DEBUG("not enough space for the announced header extension value | packet discarded");
				return nullptr;
			}
			pos += 4 + extension_value_size;
		}

		// Get payload.
		MS_BYTE* payload = (MS_BYTE*)data + pos;
		size_t payloadLength = len - pos;
		MS_BYTE payloadPadding = 0;

		MS_ASSERT(len >= pos, "payload has negative size");

		// Check padding field.
		if (header->padding)
		{
			// Must be at least a single payload byte.
			if (payloadLength == 0)
			{
				MS_DEBUG("padding bit is set but no space for a padding byte | packet discarded");
				return nullptr;
			}

			payloadPadding = data[len - 1];
			if (payloadPadding == 0)
			{
				MS_DEBUG("padding byte cannot be 0  packet discarded");
				return nullptr;
			}

			if (payloadLength < (size_t)payloadPadding)
			{
				MS_DEBUG("number of padding octets is greater than available space for payload | packet discarded");
				return nullptr;
			}
			payloadLength -= (size_t)payloadPadding;
		}

		if (payloadLength == 0)
			payload = nullptr;

		MS_ASSERT(len == sizeof(Header) + csrc_list_size + (extensionHeader ? 4 + extension_value_size : 0) + payloadLength + (size_t)payloadPadding, "packet's computed length does not match received length");

		return new RTPPacket(header, extensionHeader, payload, payloadLength, payloadPadding, data, len);
	}

	/* Instance methods. */

	RTPPacket::RTPPacket(Header* header, ExtensionHeader* extensionHeader, const MS_BYTE* payload, size_t payloadLength, MS_BYTE payloadPadding, const MS_BYTE* raw, size_t length) :
		header(header),
		extensionHeader(extensionHeader),
		payload((MS_BYTE*)payload),
		payloadLength(payloadLength),
		payloadPadding(payloadPadding),
		raw((MS_BYTE*)raw),
		length(length)
	{
		MS_TRACE();

		if (this->header->csrc_count)
			this->csrcList = (MS_BYTE*)raw + sizeof(Header);
	}

	RTPPacket::~RTPPacket()
	{
		MS_TRACE();

		if (this->isSerialized)
			delete this->raw;
	}

	void RTPPacket::Serialize()
	{
		MS_TRACE();

		if (this->isSerialized)
			delete this->raw;

		// Set this->isSerialized so the destructor will free the buffer.
		this->isSerialized = true;

		// Some useful variables.
		size_t extension_value_size = 0;

		// First calculate the total required size for the entire message.
		this->length = sizeof(Header);  // Minimun header.

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
		this->raw = new MS_BYTE[this->length];

		// Add minimun header.
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

			// Update the pointer.
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

	void RTPPacket::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		MS_DEBUG("[RTPPacket]");
		MS_DEBUG("- Padding: %s", this->header->padding ? "true" : "false");
		MS_DEBUG("- Extension: %s", HasExtensionHeader() ? "true" : "false");
		if (HasExtensionHeader())
		{
			MS_DEBUG("- Extension ID: %u", (unsigned int)GetExtensionHeaderId());
			MS_DEBUG("- Extension length: %zu bytes", GetExtensionHeaderLength());
		}
		MS_DEBUG("- CSRC count: %u", (unsigned int)this->header->csrc_count);  // TODO: method?
		MS_DEBUG("- Marker: %s", HasMarker() ? "true" : "false");
		MS_DEBUG("- Payload type: %u", (unsigned int)GetPayloadType());
		MS_DEBUG("- Sequence number: %u", (unsigned int)GetSequenceNumber());
		MS_DEBUG("- Timestamp: %lu", (unsigned long)GetTimestamp());
		MS_DEBUG("- SSRC: %lu", (unsigned long)GetSSRC());
		MS_DEBUG("- Payload size: %zu bytes", GetPayloadLength());
		MS_DEBUG("[/RTPPacket]");
	}
}
