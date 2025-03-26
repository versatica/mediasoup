#ifndef MS_RTC_SCTP_PACKET_HPP
#define MS_RTC_SCTP_PACKET_HPP

#include "common.hpp"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace RTC
{
	namespace SCTP
	{
		class Packet
		{
			/**
			 * SCTP Common Header.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |      Source Port Number       |    Destination Port Number    |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                       Verification Tag                        |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |                           Checksum                            |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */

			/**
			 * SCTP Chunk.
			 *
			 *  0                   1                   2                   3
			 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * |  Chunk Type   |  Chunk Flags  |         Chunk Length          |
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 * \                                                               \
			 * /                          Chunk Value                          /
			 * \                                                               \
			 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			 */

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
					(header->sourcePort != 0 && header->destinationPort != 0)
				);
				// clang-format on
			}

			static Packet* Parse(const uint8_t* data, size_t len);
			static const std::string& ChunkType2String(ChunkType chunkType);

		private:
			static absl::flat_hash_map<ChunkType, std::string> chunkType2String;

		public:
			Packet(CommonHeader* commonHeader, size_t size);

			~Packet();

			const uint8_t* GetData() const
			{
				return reinterpret_cast<const uint8_t*>(this->commonHeader);
			}

			size_t GetSize() const
			{
				return this->size;
			}

			void Dump() const;

			uint16_t GetSourcePort() const
			{
				return uint16_t{ ntohs(this->commonHeader->sourcePort) };
			}

			void SetSourcePort(uint16_t port)
			{
				this->commonHeader->sourcePort = uint16_t{ htons(port) };
			}

			uint16_t GetDestinationPort() const
			{
				return uint16_t{ ntohs(this->commonHeader->destinationPort) };
			}

			void SetDestinationPort(uint16_t port)
			{
				this->commonHeader->destinationPort = uint16_t{ htons(port) };
			}

			uint32_t GetVerificationTag() const
			{
				return uint32_t{ ntohl(this->commonHeader->verificationTag) };
			}

			void SetVerificationTag(uint32_t verificationTag)
			{
				this->commonHeader->verificationTag = uint32_t{ htonl(verificationTag) };
			}

			uint32_t GetChecksum() const
			{
				return uint32_t{ ntohl(this->commonHeader->checksum) };
			}

			void SetChecksum(uint32_t checksum)
			{
				this->commonHeader->checksum = uint32_t{ htonl(checksum) };
			}

		private:
			CommonHeader* commonHeader{ nullptr };
			// Full size of the packet in bytes.
			size_t size{ 0u };
		};
	} // namespace SCTP
} // namespace RTC

#endif
