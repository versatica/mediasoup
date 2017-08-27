#define MS_CLASS "RTC::RtpPacket"
#define MS_LOG_DEV // TODO: TMP

#include "RTC/RtpPacket.hpp"
#include "Logger.hpp"
#include <vector>
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream
#include <cstring> // std::memcpy()

namespace RTC
{
	/* Class methods. */

	RtpPacket* RtpPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!RtpPacket::IsRtp(data, len))
			return nullptr;

		auto* ptr = const_cast<uint8_t*>(data);

		// Get the header.
		auto* header = reinterpret_cast<Header*>(ptr);

		// Inspect data after the minimum header size.
		// size_t pos = sizeof(Header);
		ptr += sizeof(Header);

		// Check CSRC list.
		size_t csrcListSize{ 0 };

		if (header->csrcCount != 0u)
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
		ExtensionHeader* extensionHeader{ nullptr };
		size_t extensionValueSize{ 0 };

		if (header->extension != 0u)
		{
			// The extension header is at least 4 bytes.
			if (len < static_cast<size_t>(ptr - data) + 4)
			{
				MS_WARN_TAG(rtp, "not enough space for the announced extension header, packet discarded");

				return nullptr;
			}

			extensionHeader = reinterpret_cast<ExtensionHeader*>(ptr);

			// The header extension contains a 16-bit length field that counts the number of
			// 32-bit words in the extension, excluding the four-octet extension header.
			extensionValueSize = static_cast<size_t>(ntohs(extensionHeader->length) * 4);

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
		uint8_t* payload     = ptr;
		size_t payloadLength = len - (ptr - data);
		uint8_t payloadPadding{ 0 };

		MS_ASSERT(len >= static_cast<size_t>(ptr - data), "payload has negative size");

		// Check padding field.
		if (header->padding != 0u)
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

			if (payloadLength < static_cast<size_t>(payloadPadding))
			{
				MS_WARN_TAG(
				    rtp,
				    "number of padding octets is greater than available space for payload, packet "
				    "discarded");

				return nullptr;
			}
			payloadLength -= size_t{ payloadPadding };
		}

		MS_ASSERT(
		    len == sizeof(Header) + csrcListSize + (extensionHeader ? 4 + extensionValueSize : 0) +
		               payloadLength + static_cast<size_t>(payloadPadding),
		    "packet's computed size does not match received size");

		auto packet = new RtpPacket(header, extensionHeader, payload, payloadLength, payloadPadding, len);

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
	    : header(header), extensionHeader(extensionHeader), payload(const_cast<uint8_t*>(payload)),
	      payloadLength(payloadLength), payloadPadding(payloadPadding), size(size)
	{
		MS_TRACE();

		if (this->header->csrcCount != 0u)
			this->csrcList = reinterpret_cast<uint8_t*>(header) + sizeof(Header);
	}

	RtpPacket::~RtpPacket()
	{
		MS_TRACE();
	}

	void RtpPacket::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<RtpPacket>");
		MS_DUMP("  padding           : %s", this->header->padding ? "true" : "false");
		MS_DUMP("  extension header  : %s", HasExtensionHeader() ? "true" : "false");
		if (HasExtensionHeader())
		{
			MS_DUMP("    id      : %" PRIu16, GetExtensionHeaderId());
			MS_DUMP("    length  : %zu bytes", GetExtensionHeaderLength());
		}
		if (HasOneByteExtensions())
		{
			MS_DUMP("  RFC5285 ext style : One-Byte Header");
		}
		if (HasTwoBytesExtensions())
		{
			MS_DUMP("  RFC5285 ext style : Two-Bytes Header");
		}
		if (HasOneByteExtensions() || HasTwoBytesExtensions())
		{
			std::vector<std::string> extIds;
			std::ostringstream extIdsStream;

			if (HasOneByteExtensions())
			{
				for (const auto& pair : this->oneByteExtensions)
					extIds.push_back(std::to_string(pair.first));
			}
			else
			{
				for (const auto& pair : this->twoBytesExtensions)
					extIds.push_back(std::to_string(pair.first));
			}

			std::copy(
			    extIds.begin(), extIds.end() - 1, std::ostream_iterator<std::string>(extIdsStream, ","));
			extIdsStream << extIds.back();

			MS_DUMP("  RFC5285 ext ids   : %s", extIdsStream.str().c_str());
		}
		MS_DUMP("  csrc count        : %" PRIu8, this->header->csrcCount);
		MS_DUMP("  marker            : %s", HasMarker() ? "true" : "false");
		MS_DUMP("  payload type      : %" PRIu8, GetPayloadType());
		MS_DUMP("  sequence number   : %" PRIu16, GetSequenceNumber());
		MS_DUMP("  timestamp         : %" PRIu32, GetTimestamp());
		MS_DUMP("  ssrc              : %" PRIu32, GetSsrc());
		MS_DUMP("  payload size      : %zu bytes", GetPayloadLength());
		if (this->header->padding)
		{
			MS_DUMP("  padding size    : %" PRIu8 " bytes", this->payloadPadding);
		}
		MS_DUMP("</RtpPacket>");
	}

	void RtpPacket::MangleExtensionHeaderIds(std::map<uint8_t, uint8_t>& idMapping)
	{
		MS_TRACE();

		if (HasOneByteExtensions())
		{
			for (const auto& pair : this->oneByteExtensions)
			{
				auto& id = pair.first;
				auto& oneByteExtension = pair.second;

				if (idMapping.find(id) != idMapping.end())
				{
					oneByteExtension->id = idMapping[id];
				}
			}
		}
		else if (HasTwoBytesExtensions())
		{
			for (const auto& pair : this->twoBytesExtensions)
			{
				auto& id = pair.first;
				auto& twoBytesExtension = pair.second;

				if (idMapping.find(id) != idMapping.end())
				{
					twoBytesExtension->id = idMapping[id];
				}
			}
		}

		// Clear the URI to id map.
		this->extensionMap.clear();

		// Parse extensions again.
		ParseExtensions();
	}

	void RtpPacket::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		// First calculate the total required size for the entire message.
		this->size = sizeof(Header); // Minimum header.

		if (this->csrcList != nullptr)
			this->size += this->header->csrcCount * sizeof(header->ssrc);

		size_t extensionValueSize = GetExtensionHeaderLength();

		if (this->extensionHeader != nullptr)
			this->size += 4 + extensionValueSize;

		this->size += this->payloadLength;
		this->size += size_t{ this->payloadPadding };

		uint8_t* ptr = buffer;

		// Add minimum header.
		std::memcpy(buffer, this->header, sizeof(Header));

		// Update the header pointer.
		this->header = reinterpret_cast<Header*>(ptr);
		ptr += sizeof(Header);

		// Add CSRC list.
		if (this->csrcList != nullptr)
		{
			std::memcpy(ptr, this->csrcList, this->header->csrcCount * sizeof(this->header->ssrc));

			// Update the pointer.
			this->csrcList = ptr;
			ptr += this->header->csrcCount * sizeof(this->header->ssrc);
		}

		// Add extension header.
		if (this->extensionHeader != nullptr)
		{
			std::memcpy(ptr, this->extensionHeader, 4 + extensionValueSize);

			// Update the header extension pointer.
			this->extensionHeader = reinterpret_cast<ExtensionHeader*>(ptr);
			ptr += 4 + extensionValueSize;
		}

		// Parse and regenerate RFC 5285 extension header.
		ParseExtensions();

		// Add payload.
		if (this->payload != nullptr)
		{
			std::memcpy(ptr, this->payload, this->payloadLength);

			// Update the payload pointer.
			this->payload = ptr;
			ptr += this->payloadLength;
		}

		// Add payload padding.
		if (this->payloadPadding != 0u)
		{
			*(ptr + static_cast<size_t>(this->payloadPadding) - 1) = this->payloadPadding;
			ptr += size_t{ this->payloadPadding };
		}

		MS_ASSERT(static_cast<size_t>(ptr - buffer) == this->size, "ptr - buffer == this->size");
	}

	RtpPacket* RtpPacket::Clone(uint8_t* buffer) const
	{
		MS_TRACE();

		uint8_t* ptr = buffer;

		// Copy the full packet into the given buffer.
		std::memcpy(buffer, GetData(), GetSize());

		// Set header pointer pointing to the given buffer.
		auto* header = reinterpret_cast<Header*>(ptr);
		ptr += sizeof(Header);

		// Check CSRC list.
		if (this->csrcList != nullptr)
			ptr += header->csrcCount * sizeof(header->ssrc);

		// Check extension header.
		ExtensionHeader* extensionHeader{ nullptr };

		if (this->extensionHeader != nullptr)
		{
			// Set the header extension pointer.
			extensionHeader = reinterpret_cast<ExtensionHeader*>(ptr);
			ptr += 4 + GetExtensionHeaderLength();
		}

		// Check payload.
		uint8_t* payload{ nullptr };

		if (this->payload != nullptr)
		{
			// Set the payload pointer.
			payload = ptr;
			ptr += this->payloadLength;
		}

		// Check payload padding.
		if (this->payloadPadding != 0u)
		{
			*(ptr + static_cast<size_t>(this->payloadPadding) - 1) = this->payloadPadding;
			ptr += size_t{ this->payloadPadding };
		}

		MS_ASSERT(static_cast<size_t>(ptr - buffer) == this->size, "ptr - buffer == this->size");

		// Create the new RtpPacket instance and return it.
		auto packet = new RtpPacket(
		    header, extensionHeader, payload, this->payloadLength, this->payloadPadding, this->size);

		// Parse RFC 5285 extension header.
		packet->ParseExtensions();

		// Clone the extension map.
		packet->extensionMap = this->extensionMap;

		return packet;
	}

	void RtpPacket::RtxEncode(uint8_t payloadType, uint32_t ssrc, uint16_t seq)
	{
		MS_TRACE();

		// Rewrite the payload type.
		this->SetPayloadType(payloadType);

		// Rewrite the SSRC.
		this->SetSsrc(ssrc);

		// Write the original sequence number at the begining of the payload.
		std::memmove(this->payload + 2, this->payload, this->payloadLength);
		Utils::Byte::Set2Bytes(this->payload, 0, this->GetSequenceNumber());

		// Rewrite the sequence number.
		this->SetSequenceNumber(seq);

		// Fix the payload length.
		this->payloadLength += 2;

		// Fix the packet size.
		this->size += 2;

		// Remove padding.
		this->payloadPadding = 0u;
	}

	bool RtpPacket::RtxDecode(uint8_t payloadType, uint32_t ssrc)
	{
		MS_TRACE();

		if (this->payloadLength < 2)
		{
			MS_WARN_TAG(rtx, "not enough space for a RTX header field");
			return false;
		}

		// Rewrite the payload type.
		this->SetPayloadType(payloadType);

		// Rewrite the SSRC.
		this->SetSsrc(ssrc);

		// Rewrite the sequence number.
		this->SetSequenceNumber(Utils::Byte::Get2Bytes(this->payload, 0));

		// Fix the payload and payload length.
		this->payload += 2;
		this->payloadLength -= 2;

		// Fix the packet size.
		this->size -= 2;

		// Remove padding.
		this->payloadPadding = 0u;

		return true;
	}

	void RtpPacket::ParseExtensions()
	{
		MS_TRACE();

		// Parse One-Byte extension header.
		if (HasOneByteExtensions())
		{
			// Clear the One-Byte extension elements map.
			this->oneByteExtensions.clear();

			uint8_t* extensionStart = reinterpret_cast<uint8_t*>(this->extensionHeader) + 4;
			uint8_t* extensionEnd   = extensionStart + GetExtensionHeaderLength();
			uint8_t* ptr            = extensionStart;

			while (ptr < extensionEnd)
			{
				uint8_t id = (*ptr & 0xF0) >> 4;
				size_t len = static_cast<size_t>(*ptr & 0x0F) + 1;

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

			uint8_t* extensionStart = reinterpret_cast<uint8_t*>(this->extensionHeader) + 4;
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
} // namespace RTC
