#ifndef MS_RTC_SCTP_PACKET_HPP
#define MS_RTC_SCTP_PACKET_HPP

#include "common.hpp"
#include "RTC/SCTP/Chunk.hpp"
#include "RTC/Serializable.hpp"
#include <cstring> // std::memcpy()
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Packet.
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         Common Header                         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                           Chunk #1                            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                              ...                              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                           Chunk #n                            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

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
		class Packet : public Serializable
		{
		public:
			/**
			 * Struct of a SCTP Packet Common Header.
			 */
			struct CommonHeader
			{
				uint16_t sourcePort;
				uint16_t destinationPort;
				uint32_t verificationTag;
				uint32_t checksum;
			};

		public:
			static const size_t CommonHeaderSize{ 12 };

			/**
			 * Whether given `data` with length `len` could be a valid SCTP packet.
			 */
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

			/**
			 * Parses given `data` with length `len` and returns an allocated instance
			 * of Packet (or nullptr if it's not a valid SCTP packet).
			 *
			 * NOTE: Given `len` must include padding so `len` must be multiple of 4
			 * bytes.
			 */
			static Packet* Parse(const uint8_t* data, size_t len);

		public:
			Packet(const uint8_t* buffer, size_t size);

			~Packet();

			void Dump() const override;

			size_t GetSize() const override;

			void Serialize(uint8_t* buffer, size_t size) override;

			uint16_t GetSourcePort() const
			{
				return uint16_t{ ntohs(this->commonHeader->sourcePort) };
			}

			void SetSourcePort(uint16_t sourcePort)
			{
				this->commonHeader->sourcePort = uint16_t{ htons(sourcePort) };
			}

			uint16_t GetDestinationPort() const
			{
				return uint16_t{ ntohs(this->commonHeader->destinationPort) };
			}

			void SetDestinationPort(uint16_t destinationPort)
			{
				this->commonHeader->destinationPort = uint16_t{ htons(destinationPort) };
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

			void AddChunk(Chunk* chunk)
			{
				this->chunks.push_back(chunk);
			}

		private:
			// Pointer to the SCTP Common Header of the packet (same as this->buffer)
			// in Serializable parent class.
			CommonHeader* commonHeader{ nullptr };
			// Chunks.
			std::vector<Chunk*> chunks;
		};
	} // namespace SCTP
} // namespace RTC

#endif
