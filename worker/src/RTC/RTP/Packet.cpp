#define MS_CLASS "RTC::RTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Packet.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream

namespace RTC
{
	namespace RTP
	{
		/* Class methods. */

		bool Packet::IsRtp(const uint8_t* buffer, size_t bufferLength)
		{
			const auto* header = const_cast<FixedHeader*>(reinterpret_cast<const FixedHeader*>(buffer));

			// clang-format off
			return (
				(bufferLength >= FixedHeaderMinSize) &&
				// @see https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
				(buffer[0] > 127 && buffer[0] < 192) &&
				// RTP Version must be 2.
				(header->version == 2)
			);
			// clang-format on
		}

		Packet* Packet::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (!Packet::IsRtp(buffer, bufferLength))
			{
				MS_WARN_TAG(rtp, "not a RTP Packet");

				return nullptr;
			}

			auto* packet = new Packet(const_cast<uint8_t*>(buffer), bufferLength);

			// Packet length must be the length of the given buffer.
			packet->SetLength(bufferLength);

			if (!packet->Validate())
			{
				delete packet;
				return nullptr;
			}

			// Mark the Packet as frozen since we are parsing the given buffer.
			packet->Freeze();

			return packet;
		}

		/* Instance methods. */

		Packet::Packet(uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Packet::FixedHeaderMinSize);
		}

		Packet::~Packet()
		{
			MS_TRACE();

			// TODO
		}

		void Packet::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTP::Packet>");
			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(indentation, "  frozen: %s", IsFrozen() ? "yes" : "no");

			MS_DUMP_CLEAN(indentation, "  sequence number: %" PRIu16, GetSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  timestamp: %" PRIu32, GetTimestamp());
			MS_DUMP_CLEAN(indentation, "  marker: %s", HasMarker() ? "true" : "false");
			MS_DUMP_CLEAN(indentation, "  payload type: %" PRIu8, GetPayloadType());
			MS_DUMP_CLEAN(indentation, "  ssrc: %" PRIu32, GetSsrc());
			MS_DUMP_CLEAN(indentation, "  csrcs: %s", HasCsrcs() ? "true" : "false");

			if (HasHeaderExtension())
			{
				MS_DUMP_CLEAN(
				  indentation,
				  "  header extension: id:%" PRIu16 ", value length:%zu",
				  GetHeaderExtensionId(),
				  GetHeaderExtensionValueLength());
			}

			if (HasExtensions())
			{
				std::vector<std::string> extIds;
				std::ostringstream extIdsStream;

				if (HasOneByteExtensions())
				{
					for (const auto offset : this->oneByteExtensions)
					{
						if (offset == -1)
						{
							continue;
						}

						auto* extension = reinterpret_cast<OneByteExtension*>(GetHeaderExtensionValue() + offset);

						extIds.push_back(
						  "{id:" + std::to_string(extension->id) + ", len:" + std::to_string(extension->len) +
						  "}");
					}
				}
				else
				{
					extIds.reserve(this->twoBytesExtensions.size());

					for (const auto& kv : this->twoBytesExtensions)
					{
						const auto offset = kv.second;

						if (offset == -1)
						{
							continue;
						}

						auto* extension =
						  reinterpret_cast<TwoBytesExtension*>(GetHeaderExtensionValue() + offset);

						extIds.push_back(
						  "{id:" + std::to_string(extension->id) + ", len:" + std::to_string(extension->len) +
						  "}");
					}
				}

				if (!extIds.empty())
				{
					std::copy(
					  extIds.begin(), extIds.end() - 1, std::ostream_iterator<std::string>(extIdsStream, ", "));
					extIdsStream << extIds.back();

					MS_DUMP_CLEAN(
					  indentation,
					  "  RFC5285 extensions (%s): %s",
					  HasOneByteExtensions() ? "One-Byte" : "Two-Bytes",
					  extIdsStream.str().c_str());
				}
			}

			// TODO: Specific Extensions.

			MS_DUMP_CLEAN(indentation, "  payload length: %zu", GetPayloadLength());

			MS_DUMP_CLEAN(indentation, "  padding length: %" PRIu8, GetPaddingLength());

			// TODO: Spatial/temporal layers and DD.

			MS_DUMP_CLEAN(indentation, "</RTP::Packet>");
		}

		Packet* Packet::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new Packet(buffer, bufferLength);

			Serializable::CloneInto(clonedPacket);

			clonedPacket->oneByteExtensions  = this->oneByteExtensions;
			clonedPacket->twoBytesExtensions = this->twoBytesExtensions;

			return clonedPacket;
		}

		void Packet::SetPayloadType(uint8_t payloadType)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->payloadType = payloadType;
		}

		void Packet::SetMarker(bool marker)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->marker = marker;
		}

		void Packet::SetSequenceNumber(uint16_t seq)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->sequenceNumber = htons(seq);
		}

		void Packet::SetTimestamp(uint32_t timestamp)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->timestamp = htonl(timestamp);
		}

		void Packet::SetSsrc(uint32_t ssrc)
		{
			MS_TRACE();

			AssertNotFrozen();

			GetFixedHeaderPointer()->ssrc = htonl(ssrc);
		}

		bool Packet::Validate()
		{
			MS_TRACE();

			// Here we are at the beginning of the Packet.
			const auto* ptr = const_cast<uint8_t*>(GetBuffer());

			if (GetVersion() != 2)
			{
				MS_WARN_TAG(rtp, "invalid Packet, version must be 2");

				return false;
			}

			ptr += FixedHeaderMinSize;

			// Here we are at the beginning of the optional CCRS list.
			if (HasCsrcs())
			{
				auto csrcsLength = GetCsrcsLength();

				if (GetLength() < (ptr - GetBuffer()) + csrcsLength)
				{
					MS_WARN_TAG(rtp, "invalid Packet, not enough space for the announced CSRC list");

					return false;
				}

				ptr += csrcsLength;
			}

			// Here we are at the beginning of the optional Header Extension.
			if (HasHeaderExtension())
			{
				// The Header Extension is at least 4 bytes.
				if (GetLength() < (ptr - GetBuffer()) + 4)
				{
					MS_WARN_TAG(rtp, "invalid Packet, not enough space for the announced Header Extension");

					return false;
				}

				const auto headerExtensionTotalLength = GetHeaderExtensionTotalLength();

				if (GetLength() < (ptr - GetBuffer()) + headerExtensionTotalLength)
				{
					MS_WARN_TAG(
					  rtp, "invalid Packet, not enough space for the announced Header Extension value");

					return false;
				}

				if (!ParseExtensions())
				{
					MS_WARN_TAG(rtp, "invalid Packet, invalid Extensions");

					return false;
				}

				ptr += headerExtensionTotalLength;
			}

			// Here we are at the beginning of the optional payload.
			const auto payloadLength = GetPayloadLength();
			const auto paddingLength = GetPaddingLength();
			const auto availablePayloadAndPaddingLength = GetLength() - (GetPayloadPointer() - GetBuffer());

			if (payloadLength + paddingLength != availablePayloadAndPaddingLength)
			{
				MS_WARN_TAG(rtp, "invalid Packet, not enough space for announced padding");

				return false;
			}

			if (HasPadding() && paddingLength == 0)
			{
				MS_WARN_TAG(rtp, "invalid Packet, padding byte cannot be 0");

				return false;
			}

			ptr += availablePayloadAndPaddingLength;

			// Here we are at the end of the Packet.
			MS_ASSERT(
			  ptr - GetBuffer() == GetLength(),
			  "Packet computed length does not match its assigned length");

			return true;
		}

		bool Packet::ParseExtensions()
		{
			MS_TRACE();

			if (HasOneByteExtensions())
			{
				const uint8_t* extensionsStart = GetHeaderExtensionValue();
				const uint8_t* extensionsEnd   = extensionsStart + GetHeaderExtensionValueLength();
				uint8_t* ptr                   = const_cast<uint8_t*>(extensionsStart);

				// One-Byte Extensions cannot have length 0.
				while (ptr < extensionsEnd)
				{
					const auto* extension = reinterpret_cast<OneByteExtension*>(ptr);
					const uint8_t id      = extension->id;
					// NOTE: In Ont-Byte Extensions, announced value must be incremented
					// by 1.
					const size_t len = extension->len + 1;

					// id=0 means alignment.
					if (id == 0)
					{
						++ptr;
					}
					// id=15 in One-Byte extensions means "stop parsing here".
					else if (id == 15)
					{
						break;
					}
					// Valid Extension id.
					else
					{
						if (ptr + 1 + len > extensionsEnd)
						{
							MS_WARN_TAG(
							  rtp,
							  "not enough space for the announced value of the One-Byte Extension with id %" PRIu8,
							  id);

							return false;
						}

						// Store the One-Byte Extension offset in the array.
						// `-1` because we have 14 elements total 0..13 and `id` is in the
						// range 1..14.
						this->oneByteExtensions[id - 1] = ptr - extensionsStart;

						ptr += (1 + len);
					}

					// Counting padding bytes.
					while (ptr < extensionsEnd && *ptr == 0)
					{
						++ptr;
					}
				}

				return true;
			}
			else if (HasTwoBytesExtensions())
			{
				const uint8_t* extensionsStart = GetHeaderExtensionValue();
				const uint8_t* extensionsEnd   = extensionsStart + GetHeaderExtensionValueLength();
				// ptr points to the Extension id field (1 byte).
				// ptr+1 points to the length field (1 byte, can have value 0).
				uint8_t* ptr = const_cast<uint8_t*>(extensionsStart);

				// Two-Byte Extensions can have length 0.
				while (ptr + 1 < extensionsEnd)
				{
					const auto* extension = reinterpret_cast<TwoBytesExtension*>(ptr);
					const uint8_t id      = extension->id;
					const size_t len      = extension->len;

					// id=0 means alignment.
					if (id == 0)
					{
						++ptr;
					}
					// Valid Extension id.
					else
					{
						if (ptr + 2 + len > extensionsEnd)
						{
							MS_WARN_TAG(
							  rtp,
							  "not enough space for the announced value of the Two-Bytes Extension with id %" PRIu8,
							  id);

							return false;
						}

						// Store the Two-Bytes Extension offset in the map.
						this->twoBytesExtensions[id] = ptr - extensionsStart;

						ptr += (2 + len);
					}

					// Counting padding bytes.
					while (ptr < extensionsEnd && *ptr == 0)
					{
						++ptr;
					}
				}

				return true;
			}
			// If there is no Header Extension of if there is but it doesn't conform
			// to RFC 8285 Extensions, then this is ok.
			else
			{
				return true;
			}
		}
	} // namespace RTP
} // namespace RTC
