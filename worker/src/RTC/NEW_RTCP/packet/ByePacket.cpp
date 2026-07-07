#define MS_CLASS "RTC::NEW_RTCP::ByePacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/ByePacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove()
#include <limits>  // std::numeric_limits

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		ByePacket* ByePacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::BYE)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			return ByePacket::ParseStrict(buffer, bufferLength, packetLength);
		}

		ByePacket* ByePacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < Packet::CommonHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* packet = new ByePacket(buffer, bufferLength);

			packet->InitializeHeader(Packet::PacketType::BYE, Packet::CommonHeaderLength);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum Packet length.

			return packet;
		}

		ByePacket* ByePacket::ParseStrict(const uint8_t* buffer, size_t bufferLength, size_t packetLength)
		{
			MS_TRACE();

			auto* packet = new ByePacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Validate that Reason length value fits into the packet.
			if (packet->HasReason())
			{
				MS_WARN_TAG(
				  rtcp, "length indicated in the Reason length field exceeds the length of the packet");

				delete packet;
				return nullptr;
			}

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			return packet;
		}

		/* Instance methods. */

		ByePacket::ByePacket(uint8_t* buffer, size_t bufferLength) : Packet(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Packet::CommonHeaderLength);
		}

		ByePacket::~ByePacket()
		{
			MS_TRACE();
		}

		void ByePacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::ByePacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  ssrcs:");
			for (const uint32_t ssrc : GetSsrcs())
			{
				MS_DUMP_CLEAN(indentation, "  - ssrc: %" PRIu32, ssrc);
			}
			if (HasReason())
			{
				const auto reason = GetReason();

				MS_DUMP_CLEAN(indentation, "  has reason: yes");
				MS_DUMP_CLEAN(
				  indentation, "  reason: \"%.*s\"", static_cast<int>(reason.size()), reason.data());
			}
			else
			{
				MS_DUMP_CLEAN(indentation, "  has reason: no");
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::ByePacket>");
		}

		ByePacket* ByePacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new ByePacket(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		std::vector<uint32_t> ByePacket::GetSsrcs() const
		{
			MS_TRACE();

			const uint8_t numberOfSsrcs = GetCount();
			std::vector<uint32_t> ssrcs;

			ssrcs.reserve(numberOfSsrcs);

			for (uint8_t idx{ 0 }; idx < numberOfSsrcs; ++idx)
			{
				ssrcs.emplace_back(GetSsrcAt(idx));
			}

			return ssrcs;
		}

		void ByePacket::AddSsrc(uint32_t ssrc)
		{
			MS_TRACE();

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 4);

			// Must move Reason fields down.
			if (HasReason())
			{
				std::memmove(
				  GetReasonLengthPointer() + 4,
				  GetReasonLengthPointer(),
				  Utils::Byte::PadTo4Bytes(1u + GetReasonLength()));
			}

			// Add the new ssrc.
			Utils::Byte::Set4Bytes(GetSsrcsPointer(), GetCount() * 4, ssrc);

			// Update the counter field.
			SetCount(GetCount() + 1);
		}

		void ByePacket::SetReason(const std::string_view& reason)
		{
			MS_TRACE();

			if (reason.size() > std::numeric_limits<uint8_t>::max())
			{
				MS_THROW_TYPE_ERROR("reason length (%zu bytes) cannot be greater than 255", reason.size());
			}

			const size_t previousLength      = GetLength();
			const size_t previousLengthField = GetLengthFieldComputed();
			// The previous total length of Reason length and Reason fields padded
			// to 4 bytes.
			const uint8_t previousReasonFieldsPaddedLength =
			  HasReason() ? Utils::Byte::PadTo4Bytes(1u + GetReasonLength()) : 0;
			// The new total length of Reason length and Reason fields padded to
			// 4 bytes.
			const uint8_t newReasonFieldsPaddedLength =
			  reason.size() > 0 ? Utils::Byte::PadTo4Bytes(1 + reason.size()) : 0;
			const size_t newLength = size_t{ previousLength } - size_t{ previousReasonFieldsPaddedLength } +
			                         newReasonFieldsPaddedLength;

			try
			{
				// Let's call SetLength() on parent with the new computed length.
				// NOTE: If there is no space in the buffer for it, it will throw.
				SetLength(newLength);

				// Update length field.
				// NOTE: This will throw if computed value is too big.
				SetLengthField(newLength);

				if (reason.size() > 0)
				{
					// Write Reason length field.
					Utils::Byte::Set1Byte(GetReasonLengthPointer(), 0, reason.size());

					// Write Reason field.
					std::memmove(GetReasonPointer(), reason.data(), reason.size());

					// Fill padding bytes with zero.
					FillPadding(newReasonFieldsPaddedLength - 1 - reason.size());
				}
			}
			catch (const MediaSoupError& error)
			{
				// Rollback.
				SetLength(previousLength);
				SetLengthField(previousLengthField);

				throw;
			}
		}

		ByePacket* ByePacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedPacket = new ByePacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace NEW_RTCP
} // namespace RTC
