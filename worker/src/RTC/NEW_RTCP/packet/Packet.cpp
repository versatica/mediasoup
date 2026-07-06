#define MS_CLASS "RTC::RTCP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove()

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class variables. */

		// clang-format off
		const ankerl::unordered_dense::map<Packet::PacketType, std::string> Packet::PacketType2String =
		{
			{ Packet::PacketType::IJ,    "IF"    },
			{ Packet::PacketType::SR,    "SR"    },
			{ Packet::PacketType::RR,    "RR"    },
			{ Packet::PacketType::SDES,  "SDES"  },
			{ Packet::PacketType::BYE,   "BYE"   },
			{ Packet::PacketType::APP,   "APP"   },
			{ Packet::PacketType::RTPFB, "RTPFB" },
			{ Packet::PacketType::PSFB,  "PSFB"  },
			{ Packet::PacketType::XR,    "XR"    },
		};
		// clang-format on

		/* Class methods. */

		bool Packet::IsRtcp(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			return (
			  bufferLength >= Packet::CommonHeaderLength &&
			  // @see RFC 7983.
			  (buffer[0] > 127 && buffer[0] < 192) &&
			  // RTP Version must be 2.
			  (buffer[0] >> 6) == 2 &&
			  // RTCP packet types defined by IANA:
			  // http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4
			  // RFC 5761 (RTCP-mux) states this range for secure RTCP/RTP detection.
			  (buffer[1] >= 192 && buffer[1] <= 223) &&
			  // RTCP packets must have a length that is a multiple of 4 bytes.
			  Utils::Byte::IsPaddedTo4Bytes(bufferLength));
		}

		bool Packet::IsPacket(
		  const uint8_t* buffer, size_t bufferLength, PacketType& packetType, size_t& packetLength)
		{
			MS_TRACE();

			if (!Packet::IsRtcp(buffer, bufferLength))
			{
				return false;
			}

			// The padding mechanism is not implemented since it's only used when
			// encrypting the compound packet by following the section 9.1 of RFC
			// 3550 which absolutely nobody does (SRTCP is used instead). So packets
			// with Padding bit set to 1 are considered invalid and rejected.
			if ((buffer[0] >> 5) & 0x01)
			{
				MS_WARN_TAG(rtcp, "RTCP packet with Padding bit set to 1 not supported");

				return false;
			}

			packetType   = static_cast<Packet::PacketType>(buffer[1]);
			packetLength = (static_cast<size_t>(Utils::Byte::Get2Bytes(buffer, 2)) + 1) * 4;

			if (bufferLength < packetLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "no space for announced packet length [packetType:%s, packetLength:%zu, bufferLength:%zu]",
				  Packet::PacketTypeToString(packetType).c_str(),
				  packetLength,
				  bufferLength);

				return false;
			}

			return true;
		}

		const std::string& Packet::PacketTypeToString(PacketType packetType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = Packet::PacketType2String.find(packetType);

			if (it == Packet::PacketType2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		Packet::Packet(uint8_t* buffer, size_t bufferLength) : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		Packet::~Packet()
		{
			MS_TRACE();
		}

		void Packet::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation,
			  "  type: %" PRIu8 " (%s) (unknown: %s)",
			  static_cast<uint8_t>(GetType()),
			  Packet::PacketTypeToString(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
		}

		void Packet::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			SetBuffer(const_cast<uint8_t*>(buffer));
		}

		void Packet::SoftCloneInto(Packet* packet) const
		{
			MS_TRACE();

			// Need to manually set Serializable length.
			packet->SetLength(GetLength());
		}

		void Packet::SetVariableLengthValue(const uint8_t* value, size_t valueLength)
		{
			MS_TRACE();

			if (value == nullptr && valueLength > 0)
			{
				MS_THROW_TYPE_ERROR("value cannot be nullptr if valueLength is > 0");
			}

			// NOTE: This can throw.
			SetVariableLengthValueLength(valueLength);

			if (value)
			{
				std::memmove(GetVariableLengthValuePointer(), value, valueLength);
			}
		}

		void Packet::SetVariableLengthValueLength(size_t valueLength)
		{
			MS_TRACE();

			// TODO: See TLV::SetVariableLengthValueLength() but here we need to update Length
			// field properly (32-bit words - 1, etc) and take into account padding, and the
			// padding bit, and write padding lenght in the last byte of the value, etc. So
			// also check RTP::Packet.
		}
	} // namespace NEW_RTCP
} // namespace RTC
