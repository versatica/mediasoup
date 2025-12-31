#define MS_CLASS "RTC::RTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTP/Packet.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

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
			// TODO
			// MS_DUMP_CLEAN(indentation, "  source port: %" PRIu16, GetSourcePort());
			// MS_DUMP_CLEAN(indentation, "  destination port: %" PRIu16, GetDestinationPort());
			// MS_DUMP_CLEAN(indentation, "  verification tag: %" PRIu32, GetVerificationTag());
			// MS_DUMP_CLEAN(indentation, "  checksum: %" PRIu32, GetChecksum());
			// MS_DUMP_CLEAN(indentation, "  chunks count: %zu", GetChunksCount());
			// for (const auto* chunk : this->chunks)
			// {
			// 	chunk->Dump(indentation + 1);
			// }
			MS_DUMP_CLEAN(indentation, "</RTP::Packet>");
		}

		void Packet::Serialize(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			// const auto* previousBuffer = GetBuffer();

			// Invoke the parent method to copy the whole buffer.
			Serializable::Serialize(buffer, bufferLength);

			// TODO: Extensions pointers and DD and so on.
		}

		Packet* Packet::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new Packet(buffer, bufferLength);

			Serializable::CloneInto(clonedPacket);

			// TODO
			// Soft clone Packet Chunks into the given cloned Packet.
			// for (auto* chunk : this->chunks)
			// {
			// 	const size_t offset = chunk->GetBuffer() - GetBuffer();

			// 	auto* softClonedChunk = chunk->SoftClone(buffer + offset);

			// 	// Chunk constructors don't freeze the Chunk so we must do it manually.
			// 	softClonedChunk->Freeze();

			// 	clonedPacket->chunks.push_back(softClonedChunk);
			// }

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

		bool Packet::Validate() const
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
	} // namespace RTP
} // namespace RTC
