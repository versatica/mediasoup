#define MS_CLASS "RTC::NEW_RTCP::RembPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/RembPacket.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <limits> // std::numeric_limits

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class methods. */

		RembPacket* RembPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			if (packetType != Packet::PacketType::PSFB)
			{
				MS_WARN_DEV("invalid packet type");

				return nullptr;
			}

			const auto messageType =
			  static_cast<AnyPayloadSpecificFeedbackPacket::MessageType>(buffer[0] & 0x1F);

			if (messageType != AnyPayloadSpecificFeedbackPacket::MessageType::AFB)
			{
				MS_WARN_DEV("invalid message type");

				return nullptr;
			}

			return RembPacket::ParseStrict(buffer, bufferLength, packetLength, padding);
		}

		RembPacket* RembPacket::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < RembPacket::RembHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* const packet = new RembPacket(buffer, bufferLength);

			packet->InitializeHeader(
			  Packet::PacketType::PSFB,
			  static_cast<uint8_t>(AnyPayloadSpecificFeedbackPacket::MessageType::AFB),
			  RembPacket::RembHeaderLength);

			// Must also initialize extra fields in the header.
			packet->SetSenderSsrc(0);
			packet->SetMediaSsrc(0);
			Utils::Byte::Set4Bytes(buffer, 12, RembPacket::UniqueIdentifier);
			Utils::Byte::Set1Byte(buffer, 16, 0);
			packet->SetBitrate(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum RembPacket length.

			return packet;
		}

		RembPacket* RembPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength, uint8_t padding)
		{
			MS_TRACE();

			// Length of the packet excluding padding.
			const size_t contentLength = packetLength - padding;

			if (contentLength < RembPacket::RembHeaderLength)
			{
				MS_WARN_TAG(
				  rtcp, "RembPacket length must be equal or greater than %zu", RembPacket::RembHeaderLength);

				return nullptr;
			}

			if (Utils::Byte::Get4Bytes(buffer, 12) != RembPacket::UniqueIdentifier)
			{
				MS_WARN_TAG(rtcp, "invalid unique identifier");

				return nullptr;
			}

			const uint8_t numberOfSsrcs = Utils::Byte::Get1Byte(buffer, 16);

			// The packet (excluding padding) must end right after the announced
			// SSRCs.
			if (contentLength != RembPacket::RembHeaderLength + (numberOfSsrcs * 4))
			{
				MS_WARN_TAG(
				  rtcp,
				  "packet length (%zu bytes, excluding padding) does not match the number of SSRCs (%" PRIu8
				  ")",
				  contentLength,
				  numberOfSsrcs);

				return nullptr;
			}

			const uint8_t exponent = Utils::Byte::Get1Byte(buffer, 17) >> 2;
			const uint64_t mantissa =
			  (static_cast<uint64_t>(Utils::Byte::Get1Byte(buffer, 17) & 0x03) << 16) |
			  Utils::Byte::Get2Bytes(buffer, 18);
			const uint64_t bitrate = mantissa << exponent;

			// The bitrate must not overflow the shift and must fit in a signed 64 bits
			// integer.
			if ((bitrate >> exponent) != mantissa || bitrate > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
			{
				MS_WARN_TAG(
				  rtcp, "invalid bitrate [mantissa:%" PRIu64 ", exponent:%" PRIu8 "]", mantissa, exponent);

				return nullptr;
			}

			auto* const packet = new RembPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			return packet;
		}

		/* Instance methods. */

		RembPacket::RembPacket(uint8_t* buffer, size_t bufferLength)
		  : AnyPayloadSpecificFeedbackPacket(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(RembPacket::RembHeaderLength);
		}

		RembPacket::~RembPacket()
		{
			MS_TRACE();
		}

		void RembPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::RembPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  message type: %" PRIu8 " (%s)",
			  static_cast<uint8_t>(GetMessageType()),
			  AnyPayloadSpecificFeedbackPacket::MessageTypeToString(GetMessageType()).c_str());
			DumpFeedbackCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  bitrate: %" PRIi64 " bps", GetBitrate());
			MS_DUMP_CLEAN(indentation, "  ssrcs: %" PRIu8, GetNumberOfSsrcs());
			for (const auto ssrc : GetSsrcs())
			{
				MS_DUMP_CLEAN(indentation, "  - ssrc: %" PRIu32, ssrc);
			}
			MS_DUMP_CLEAN(indentation, "</RTCP::RembPacket>");
		}

		RembPacket* RembPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new RembPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		int64_t RembPacket::GetBitrate() const
		{
			MS_TRACE();

			const uint8_t exponent = Utils::Byte::Get1Byte(GetBuffer(), 17) >> 2;
			const uint64_t mantissa =
			  (static_cast<uint64_t>(Utils::Byte::Get1Byte(GetBuffer(), 17) & 0x03) << 16) |
			  Utils::Byte::Get2Bytes(GetBuffer(), 18);

			return static_cast<int64_t>(mantissa << exponent);
		}

		void RembPacket::SetBitrate(int64_t bitrate)
		{
			MS_TRACE();

			if (bitrate < 0)
			{
				MS_THROW_TYPE_ERROR("bitrate (%" PRIi64 ") cannot be negative", bitrate);
			}

			auto mantissa = static_cast<uint64_t>(bitrate);
			uint8_t exponent{ 0 };

			// The mantissa has 18 bits.
			while (mantissa > 0x3FFFF)
			{
				mantissa >>= 1;
				++exponent;
			}

			auto* const buffer = const_cast<uint8_t*>(GetBuffer());

			Utils::Byte::Set1Byte(buffer, 17, static_cast<uint8_t>((exponent << 2) | (mantissa >> 16)));
			Utils::Byte::Set2Bytes(buffer, 18, static_cast<uint16_t>(mantissa & 0xFFFF));
		}

		std::vector<uint32_t> RembPacket::GetSsrcs() const
		{
			MS_TRACE();

			const auto* const ssrcsPointer = GetSsrcsPointer();
			const uint8_t numberOfSsrcs    = GetNumberOfSsrcs();

			std::vector<uint32_t> ssrcs;

			ssrcs.reserve(numberOfSsrcs);

			for (uint8_t idx{ 0 }; idx < numberOfSsrcs; ++idx)
			{
				ssrcs.push_back(Utils::Byte::Get4Bytes(ssrcsPointer, idx * 4));
			}

			return ssrcs;
		}

		void RembPacket::AddSsrc(uint32_t ssrc)
		{
			MS_TRACE();

			const uint8_t numberOfSsrcs = GetNumberOfSsrcs();

			if (numberOfSsrcs == RembPacket::MaxSsrcs)
			{
				MS_THROW_TYPE_ERROR("cannot add more than %zu SSRCs", RembPacket::MaxSsrcs);
			}

			// The new SSRC goes right after the last one.
			auto* const ptr = GetSsrcsPointer() + (numberOfSsrcs * 4);

			// NOTE: This may throw.
			SetVariableLengthValueLength(GetVariableLengthValueLength() + 4);

			// Write the SSRC.
			Utils::Byte::Set4Bytes(ptr, 0, ssrc);

			// Update the Num SSRC field.
			Utils::Byte::Set1Byte(const_cast<uint8_t*>(GetBuffer()), 16, numberOfSsrcs + 1);
		}

		RembPacket* RembPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket = new RembPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace NEW_RTCP
} // namespace RTC
