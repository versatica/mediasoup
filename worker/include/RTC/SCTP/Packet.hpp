#ifndef MS_RTC_SCTP_PACKET_HPP
#define MS_RTC_SCTP_PACKET_HPP

#include "common.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		class Packet
		{
		public:
			/* Struct for SCTP common header. */
			struct CommonHeader
			{
				uint16_t sourcePort;
				uint16_t destinationPort;
				uint32_t verificationTag;
				uint32_t checksum;
			};

			// Chunk type.
			enum class ChunkType : uint8_t
			{
				DATA              = 0x00,
				INIT              = 0x01,
				INIT_ACK          = 0x02,
				SACK              = 0x03,
				HEARTBEAT         = 0x04,
				HEARTBEAT_ACK     = 0x05,
				ABORT             = 0x06,
				SHUTDOWN          = 0x07,
				SHUTDOWN_ACK      = 0x08,
				ERROR             = 0x09,
				COOKIE_ECHO       = 0x0A,
				COOKIE_ACK        = 0x0B,
				ECNE              = 0x0C,
				CWR               = 0x0D,
				SHUTDOWN_COMPLETE = 0x0E
			};

		public:
			static const size_t CommonHeaderSize{ 12 };
			static bool IsSctp(const uint8_t* data, size_t len)
			{
				auto* header = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

				// clang-format off
				return (
					(len >= CommonHeaderSize) &&
					// Source and destination ports cannot be 0.
					(header->sourcePort != 0 && header->destinationPort != 0) &&
					// Must be padded to 4 bytes.
					(Utils::Byte::PadTo4Bytes(static_cast<uint32_t>(len)) == len)
				);
				// clang-format on
			}
			static Packet* Parse(const uint8_t* data, size_t len);

		public:
			Packet() = default;
			// ~Packet();
		};
	} // namespace SCTP
} // namespace RTC

#endif
