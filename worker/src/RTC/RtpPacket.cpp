#define MS_CLASS "RTC::RtpPacket"
// #define MS_LOG_DEV

#include "RTC/RtpPacket.hpp"
#include "Logger.hpp"
#include <cstring> // std::memcpy()

namespace RTC
{
	/* Class methods. */

	RtpPacket* RtpPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!RtpPacket::IsRtp(data, len))
			return nullptr;

		uint8_t* ptr = (uint8_t*)data;

		// Get the header.
		Header* header = reinterpret_cast<Header*>(ptr);

		// Inspect data after the minimum header size.
		// size_t pos = sizeof(Header);
		ptr += sizeof(Header);

		// Check CSRC list.
		size_t csrcListSize = 0;

		if (header->csrcCount)
		{
			csrcListSize = header->csrcCount * sizeof(header->ssrc);

			// Packet size must be >= header size + CSRC list.
			if (len < (ptr - data) + csrcListSize)
			{
				MS_WARN_TAG(rtp, "not enough space for the announced CSRC list, packet discarded");

				return nullptr;
			}
			ptr += csrcListSize;
		}

		// Check header extension.
		ExtensionHeader* extensionHeader = nullptr;
		size_t extensionValueSize        = 0;

		if (header->extension)
		{
			// The extension header is at least 4 bytes.
			if (len < size_t(ptr - data) + 4)
			{
				MS_WARN_TAG(rtp, "not enough space for the announced extension header, packet discarded");

				return nullptr;
			}

			extensionHeader = reinterpret_cast<ExtensionHeader*>(ptr);

			// The header extension contains a 16-bit length field that counts the number of
			// 32-bit words in the extension, excluding the four-octet extension header.
			extensionValueSize = (size_t)(ntohs(extensionHeader->length) * 4);

			// Packet size must be >= header size + CSRC list + header extension size.
			if (len < (ptr - data) + 4 + extensionValueSize)
			{
				MS_WARN_TAG(
				    rtp, "not enough space for the announced header extension value, packet discarded");

				return nullptr;
			}
			ptr += 4 + extensionValueSize;
		}

		// Get payload.
		uint8_t* payload       = ptr;
		size_t payloadLength   = len - (ptr - data);
		uint8_t payloadPadding = 0;

		MS_ASSERT(len >= size_t(ptr - data), "payload has negative size");

		// Check padding field.
		if (header->padding)
		{
			// Must be at least a single payload byte.
			if (payloadLength == 0)
			{
				MS_WARN_TAG(rtp, "padding bit is set but no space for a padding byte, packet discarded");

				return nullptr;
			}

			payloadPadding = data[len - 1];
			if (payloadPadding == 0)
			{
				MS_WARN_TAG(rtp, "padding byte cannot be 0, packet discarded");

				return nullptr;
			}

			if (payloadLength < (size_t)payloadPadding)
			{
				MS_WARN_TAG(
				    rtp,
				    "number of padding octets is greater than available space for payload, packet "
				    "discarded");

				return nullptr;
			}
			payloadLength -= (size_t)payloadPadding;
		}

		if (payloadLength == 0)
			payload = nullptr;

		MS_ASSERT(
		    len == sizeof(Header) + csrcListSize + (extensionHeader ? 4 + extensionValueSize : 0) +
		               payloadLength + (size_t)payloadPadding,
		    "packet's computed size does not match received size");

		RtpPacket* packet =
		    new RtpPacket(header, extensionHeader, payload, payloadLength, payloadPadding, len);

		// Parse RFC 5285 extension header.
		packet->ParseExtensions();

		return packet;
	}

	/* Instance methods. */

	RtpPacket::RtpPacket(
	    Header* header,
	    ExtensionHeader* extensionHeader,
	    const uint8_t* payload,
	    size_t payloadLength,
	    uint8_t payloadPadding,
	    size_t size)
	    : header(header), extensionHeader(extensionHeader), payload((uint8_t*)payload),
	      payloadLength(payloadLength), payloadPadding(payloadPadding), size(size)
	{
		MS_TRACE();

		if (this->header->csrcCount)
			this->csrcList = (uint8_t*)header + sizeof(Header);
	}

	RtpPacket::~RtpPacket()
	{
		MS_TRACE();
	}

	void RtpPacket::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<RtpPacket>");
		MS_DUMP("  padding          : %s", this->header->padding ? "true" : "false");
		MS_DUMP("  extension header : %s", HasExtensionHeader() ? "true" : "false");
		if (HasExtensionHeader())
		{
			MS_DUMP("    id     : %" PRIu16, GetExtensionHeaderId());
			MS_DUMP("    length : %zu bytes", GetExtensionHeaderLength());
		}
		MS_DUMP("  csrc count       : %" PRIu8, this->header->csrcCount);
		MS_DUMP("  marker           : %s", HasMarker() ? "true" : "false");
		MS_DUMP("  payload type     : %" PRIu8, GetPayloadType());
		MS_DUMP("  sequence number  : %" PRIu16, GetSequenceNumber());
		MS_DUMP("  timestamp        : %" PRIu32, GetTimestamp());
		MS_DUMP("  ssrc             : %" PRIu32, GetSsrc());
		MS_DUMP("  payload size     : %zu bytes", GetPayloadLength());
		MS_DUMP("</RtpPacket>");
	}

	void RtpPacket::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		// First calculate the total required size for the entire message.
		this->size = sizeof(Header); // Minimum header.

		if (this->csrcList)
			this->size += this->header->csrcCount * sizeof(header->ssrc);

		size_t extensionValueSize = GetExtensionHeaderLength();

		if (this->extensionHeader)
			this->size += 4 + extensionValueSize;

		this->size += this->payloadLength;
		this->size += (size_t)this->payloadPadding;

		uint8_t* ptr = buffer;

		// Add minimum header.
		std::memcpy(buffer, this->header, sizeof(Header));

		// Update the header pointer.
		this->header = reinterpret_cast<Header*>(ptr);
		ptr += sizeof(Header);

		// Add CSRC list.
		if (this->csrcList)
		{
			std::memcpy(ptr, this->csrcList, this->header->csrcCount * sizeof(this->header->ssrc));

			// Update the pointer.
			this->csrcList = ptr;
			ptr += this->header->csrcCount * sizeof(this->header->ssrc);
		}

		// Add extension header.
		if (this->extensionHeader)
		{
			std::memcpy(ptr, this->extensionHeader, 4 + extensionValueSize);

			// Update the header extension pointer.
			this->extensionHeader = reinterpret_cast<ExtensionHeader*>(ptr);
			ptr += 4 + extensionValueSize;
		}

		// Parse and regenerate RFC 5285 extension header.
		ParseExtensions();

		// Add payload.
		if (this->payload)
		{
			std::memcpy(ptr, this->payload, this->payloadLength);

			// Update the payload pointer.
			this->payload = ptr;
			ptr += this->payloadLength;
		}

		// Add payload padding.
		if (this->payloadPadding)
		{
			*(ptr + (size_t)this->payloadPadding - 1) = this->payloadPadding;
			ptr += (size_t)this->payloadPadding;
		}

		MS_ASSERT(size_t(ptr - buffer) == this->size, "size_t(ptr - buffer) == this->size");
	}

	RtpPacket* RtpPacket::Clone(uint8_t* buffer) const
	{
		MS_TRACE();

		uint8_t* ptr = buffer;

		// Copy the full packet into the given buffer.
		std::memcpy(buffer, GetData(), GetSize());

		// Set header pointer pointing to the given buffer.
		Header* header = reinterpret_cast<Header*>(ptr);
		ptr += sizeof(Header);

		// Check CSRC list.
		if (this->csrcList)
			ptr += header->csrcCount * sizeof(header->ssrc);

		// Check extension header.
		ExtensionHeader* extensionHeader = nullptr;

		if (this->extensionHeader)
		{
			// Set the header extension pointer.
			extensionHeader = reinterpret_cast<ExtensionHeader*>(ptr);
			ptr += 4 + GetExtensionHeaderLength();
		}

		// Check payload.
		uint8_t* payload = nullptr;

		if (this->payload)
		{
			// Set the payload pointer.
			payload = ptr;
			ptr += this->payloadLength;
		}

		// Check payload padding.
		if (this->payloadPadding)
		{
			*(ptr + (size_t)this->payloadPadding - 1) = this->payloadPadding;
			ptr += (size_t)this->payloadPadding;
		}

		MS_ASSERT(size_t(ptr - buffer) == this->size, "size_t(ptr - buffer) == this->size");

		// Create the new RtpPacket instance and return it.
		RtpPacket* packet = new RtpPacket(
		    header, extensionHeader, payload, this->payloadLength, this->payloadPadding, this->size);

		// Parse RFC 5285 extension header.
		packet->ParseExtensions();

		// Clone the extension map.
		packet->extensionMap = this->extensionMap;

		return packet;
	}

	void RtpPacket::ParseExtensions()
	{
		MS_TRACE();

		// Parse One-Byte extension header.
		if (HasOneByteExtensions())
		{
			// Clear the One-Byte extension elements map.
			this->oneByteExtensions.clear();

			uint8_t* extensionStart = (uint8_t*)this->extensionHeader + 4;
			uint8_t* extensionEnd   = extensionStart + GetExtensionHeaderLength();
			uint8_t* ptr            = extensionStart;

			while (ptr < extensionEnd)
			{
				uint8_t id = (*ptr & 0xF0) >> 4;
				size_t len = (*ptr & 0x0F) + 1;

				if (ptr + 1 + len > extensionEnd)
				{
					MS_WARN_TAG(
					    rtp, "not enough space for the announced One-Byte header extension element value");

					break;
				}

				// Store the One-Byte extension element in a map.
				this->oneByteExtensions[id] = reinterpret_cast<OneByteExtension*>(ptr);

				ptr += 1 + len;

				// Counting padding bytes.
				while ((ptr < extensionEnd) && (*ptr == 0))
					++ptr;
			}
		}
		// Parse Two-Bytes extension header.
		else if (HasTwoBytesExtensions())
		{
			// Clear the Two-Bytes extension elements map.
			this->twoBytesExtensions.clear();

			uint8_t* extensionStart = (uint8_t*)this->extensionHeader + 4;
			uint8_t* extensionEnd   = extensionStart + GetExtensionHeaderLength();
			uint8_t* ptr            = extensionStart;

			while (ptr < extensionEnd)
			{
				uint8_t id = *ptr;
				size_t len = *(++ptr);

				if (ptr + len > extensionEnd)
				{
					MS_WARN_TAG(
					    rtp, "not enough space for the announced Two-Bytes header extension element value");

					break;
				}

				// Store the Two-Bytes extension element in a map.
				this->twoBytesExtensions[id] = reinterpret_cast<TwoBytesExtension*>(ptr);

				ptr += len;

				// Counting padding bytes.
				while ((ptr < extensionEnd) && (*ptr == 0))
					++ptr;
			}
		}
	}
}
