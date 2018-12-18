#define MS_CLASS "RTC::RtpPacket"
// #define MS_LOG_DEV

#include "RTC/RtpPacket.hpp"
#include "Logger.hpp"
#include <cstring>  // std::memcpy(), std::memmove()
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream
#include <vector>

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

			if (payloadLength < size_t{ payloadPadding })
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
		           payloadLength + size_t{ payloadPadding },
		  "packet's computed size does not match received size");

		auto packet = new RtpPacket(header, extensionHeader, payload, payloadLength, payloadPadding, len);

		// Parse RFC 5285 extension header.
		packet->ParseExtensions();

		return packet;
	}

	RtpPacket* RtpPacket::CreateProbationPacket(const uint8_t* buffer, uint8_t payloadPadding)
	{
		MS_TRACE();

		MS_ASSERT(buffer != nullptr, "no buffer given");
		MS_ASSERT(payloadPadding > 0, "padding cannot be 0");

		auto* ptr = const_cast<uint8_t*>(buffer);

		// Set the header.
		auto* header = reinterpret_cast<Header*>(ptr);

		// Fill some header fields.
		header->csrcCount      = 0;
		header->extension      = 0;
		header->padding        = 1;
		header->version        = 2;
		header->payloadType    = 0; // To be set by the caller.
		header->marker         = 0;
		header->sequenceNumber = 0; // To be set by the caller.
		header->timestamp      = 0; // To be set by the caller.
		header->ssrc           = 0; // To be set by the caller.

		ptr += sizeof(Header);

		// Header extension.
		ExtensionHeader* extensionHeader{ nullptr };

		// Set payload.
		uint8_t* payload     = ptr;
		size_t payloadLength = 0;

		// Add padding bytes.
		for (uint8_t i = 0; i < payloadPadding - 1; ++i)
		{
			Utils::Byte::Set1Byte(ptr++, 0, 0);
		}

		// Add a final byte with the padding count (including itself).
		Utils::Byte::Set1Byte(ptr++, 0, payloadPadding);

		// Set the packet size.
		auto size = static_cast<size_t>(ptr - buffer);

		MS_ASSERT(
		  size == sizeof(Header) + payloadLength + size_t{ payloadPadding },
		  "packet's computed size does not match received size");

		auto packet =
		  new RtpPacket(header, extensionHeader, payload, payloadLength, payloadPadding, size);

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

		MS_DEBUG_DEV("<RtpPacket>");
		MS_DEBUG_DEV("  padding           : %s", this->header->padding ? "true" : "false");
		MS_DEBUG_DEV("  extension header  : %s", HasExtensionHeader() ? "true" : "false");
		if (HasExtensionHeader())
		{
			MS_DEBUG_DEV("    id      : %" PRIu16, GetExtensionHeaderId());
			MS_DEBUG_DEV("    length  : %zu bytes", GetExtensionHeaderLength());
		}
		if (HasOneByteExtensions())
		{
			MS_DEBUG_DEV("  RFC5285 ext style : One-Byte Header");
		}
		if (HasTwoBytesExtensions())
		{
			MS_DEBUG_DEV("  RFC5285 ext style : Two-Bytes Header");
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

			if (!extIds.empty())
			{
				std::copy(
				  extIds.begin(), extIds.end() - 1, std::ostream_iterator<std::string>(extIdsStream, ","));
				extIdsStream << extIds.back();

				MS_DEBUG_DEV("  RFC5285 ext ids   : %s", extIdsStream.str().c_str());
			}
		}
		MS_DEBUG_DEV("  csrc count        : %" PRIu8, this->header->csrcCount);
		MS_DEBUG_DEV("  marker            : %s", HasMarker() ? "true" : "false");
		MS_DEBUG_DEV("  payload type      : %" PRIu8, GetPayloadType());
		MS_DEBUG_DEV("  sequence number   : %" PRIu16, GetSequenceNumber());
		MS_DEBUG_DEV("  timestamp         : %" PRIu32, GetTimestamp());
		MS_DEBUG_DEV("  ssrc              : %" PRIu32, GetSsrc());
		MS_DEBUG_DEV("  payload size      : %zu bytes", GetPayloadLength());
		if (this->header->padding != 0u)
		{
			MS_DEBUG_DEV("  padding size      : %" PRIu8 " bytes", this->payloadPadding);
		}
		MS_DEBUG_DEV("  packet size       : %zu bytes", GetSize());
		MS_DEBUG_DEV("</RtpPacket>");
	}

	void RtpPacket::MangleExtensionHeaderIds(const std::map<uint8_t, uint8_t>& idMapping)
	{
		MS_TRACE();

		if (HasOneByteExtensions())
		{
			for (const auto& pair : this->oneByteExtensions)
			{
				auto& id               = pair.first;
				auto& oneByteExtension = pair.second;

				if (idMapping.find(id) != idMapping.end())
				{
					oneByteExtension->id = idMapping.at(id);
				}
			}
		}
		else if (HasTwoBytesExtensions())
		{
			for (const auto& pair : this->twoBytesExtensions)
			{
				auto& id                = pair.first;
				auto& twoBytesExtension = pair.second;

				if (idMapping.find(id) != idMapping.end())
				{
					twoBytesExtension->id = idMapping.at(id);
				}
			}
		}

		// Clear the URI to id map.
		this->extensionMap.clear();

		// Parse extensions again.
		ParseExtensions();
	}

	RtpPacket* RtpPacket::Clone(const uint8_t* buffer) const
	{
		MS_TRACE();

		auto* ptr = const_cast<uint8_t*>(buffer);
		size_t numBytes{ 0 };

		// Copy the minimum header.

		numBytes = sizeof(Header);
		std::memcpy(ptr, GetData(), numBytes);

		// Set header pointer.
		auto* newHeader = reinterpret_cast<Header*>(ptr);

		// Update pointer.
		ptr += numBytes;

		// Copy CSRC list.

		if (this->csrcList != nullptr)
		{
			numBytes = this->header->csrcCount * sizeof(this->header->ssrc);
			std::memcpy(ptr, this->csrcList, numBytes);

			// Update pointer.
			ptr += numBytes;
		}

		// Copy extension header.

		ExtensionHeader* newExtensionHeader{ nullptr };

		if (this->extensionHeader != nullptr)
		{
			numBytes = 4 + GetExtensionHeaderLength();
			std::memcpy(ptr, this->extensionHeader, numBytes);

			// Set the header extension pointer.
			newExtensionHeader = reinterpret_cast<ExtensionHeader*>(ptr);

			// Update pointer.
			ptr += numBytes;
		}

		// Copy payload.

		uint8_t* newPayload{ nullptr };

		if (this->payload != nullptr)
		{
			numBytes = GetPayloadLength();
			std::memcpy(ptr, this->payload, numBytes);

			// Set the payload pointer.
			newPayload = ptr;

			// Update pointer.
			ptr += numBytes;
		}

		// Copy payload padding.

		if (this->payloadPadding != 0u)
		{
			*(ptr + static_cast<size_t>(this->payloadPadding) - 1) = this->payloadPadding;
			ptr += size_t{ this->payloadPadding };
		}

		MS_ASSERT(static_cast<size_t>(ptr - buffer) == this->size, "ptr - buffer == this->size");

		// Create the new RtpPacket instance and return it.
		auto packet = new RtpPacket(
		  newHeader, newExtensionHeader, newPayload, this->payloadLength, this->payloadPadding, this->size);

		// Parse RFC 5285 extension header.
		packet->ParseExtensions();

		// Clone the extension map.
		packet->extensionMap = this->extensionMap;

		return packet;
	}

	// NOTE: The caller must ensure that the buffer/memmory of the packet has
	// space enough for adding 2 extra bytes.
	void RtpPacket::RtxEncode(uint8_t payloadType, uint32_t ssrc, uint16_t seq)
	{
		MS_TRACE();

		// Rewrite the payload type.
		SetPayloadType(payloadType);

		// Rewrite the SSRC.
		SetSsrc(ssrc);

		// Write the original sequence number at the begining of the payload.
		std::memmove(this->payload + 2, this->payload, this->payloadLength);
		Utils::Byte::Set2Bytes(this->payload, 0, GetSequenceNumber());

		// Rewrite the sequence number.
		SetSequenceNumber(seq);

		// Fix the payload length.
		this->payloadLength += 2;

		// Fix the packet size.
		this->size += 2;

		// Remove padding if present.
		if (this->payloadPadding != 0u)
		{
			SetPayloadPaddingFlag(false);

			this->size -= size_t{ this->payloadPadding };
			this->payloadPadding = 0u;
		}
	}

	bool RtpPacket::RtxDecode(uint8_t payloadType, uint32_t ssrc)
	{
		MS_TRACE();

		// Chrome sends some RTX packets with no payload when the stream is started.
		// Just ignore them.
		if (this->payloadLength < 2)
			return false;

		// Rewrite the payload type.
		SetPayloadType(payloadType);

		// Rewrite the sequence number.
		SetSequenceNumber(Utils::Byte::Get2Bytes(this->payload, 0));

		// Rewrite the SSRC.
		SetSsrc(ssrc);

		// Shift the payload to its original place.
		std::memmove(this->payload, this->payload + 2, this->payloadLength - 2);

		// Fix the payload length.
		this->payloadLength -= 2;

		// Fix the packet size.
		this->size -= 2;

		// Remove padding if present.
		if (this->payloadPadding != 0u)
		{
			SetPayloadPaddingFlag(false);

			this->size -= size_t{ this->payloadPadding };
			this->payloadPadding = 0u;
		}

		return true;
	}

	bool RtpPacket::EncodePayload(RTC::Codecs::EncodingContext* context)
	{
		MS_TRACE();

		if (!this->payloadDescriptorHandler)
			return true;

		return this->payloadDescriptorHandler->Encode(context, this->payload);
	}

	void RtpPacket::RestorePayload()
	{
		MS_TRACE();

		if (!this->payloadDescriptorHandler)
			return;

		this->payloadDescriptorHandler->Restore(this->payload);
	}

	void RtpPacket::ShiftPayload(size_t payloadOffset, size_t shift, bool expand)
	{
		MS_TRACE();

		if (shift == 0u)
			return;

		MS_ASSERT(payloadOffset < this->payloadLength, "payload offset bigger than payload size");

		if (!expand)
		{
			MS_ASSERT(shift <= (this->payloadLength - payloadOffset), "shift to big");
		}

		uint8_t* payloadOffsetPtr = this->payload + payloadOffset;
		size_t shiftedLen;

		if (expand)
		{
			shiftedLen = this->payloadLength + size_t{ this->payloadPadding } - payloadOffset;

			std::memmove(payloadOffsetPtr + shift, payloadOffsetPtr, shiftedLen);

			this->payloadLength += shift;
			this->size += shift;
		}
		else
		{
			shiftedLen = this->payloadLength + size_t{ this->payloadPadding } - payloadOffset - shift;

			std::memmove(payloadOffsetPtr, payloadOffsetPtr + shift, shiftedLen);

			this->payloadLength -= shift;
			this->size -= shift;
		}
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

				// Store the One-Byte extension element in a map. Ignore if 0.
				if (id != 0u)
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

			while (ptr + 1 < extensionEnd)
			{
				uint8_t id = *ptr;
				size_t len = *(++ptr);

				if (ptr + len > extensionEnd)
				{
					MS_WARN_TAG(
					  rtp, "not enough space for the announced Two-Bytes header extension element value");

					break;
				}

				// Store the Two-Bytes extension element in a map. Ignore if 0.
				if (id != 0u)
					this->twoBytesExtensions[id] = reinterpret_cast<TwoBytesExtension*>(ptr);

				ptr += len;

				// Counting padding bytes.
				while ((ptr < extensionEnd) && (*ptr == 0))
					++ptr;
			}
		}
	}
} // namespace RTC
