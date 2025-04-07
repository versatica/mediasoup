#ifndef MS_RTC_SCTP_PACKET_HPP
#define MS_RTC_SCTP_PACKET_HPP

#include "common.hpp"
#include "RTC/SCTP/Chunk.hpp"
#include "RTC/Serializable.hpp"
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Packet.
		 *
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
		 *
		 * It's mandatory that the Packet total length is multiple of 4 bytes.
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
		 *
		 * - Source port (16 bits): Unsigned integer.
		 * - Destination port (16 bits): Unsigned integer.
		 * - Verification Tag (32 bits): Unsigned integer.
		 * - Checksum (32 bits): Unsigned integer.
		 */

		class Packet : public Serializable
		{
		public:
			using ChunksIterator = typename std::vector<Chunk*>::const_iterator;

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
			static const size_t CommonHeaderLength{ 12 };

			/**
			 * Whether given buffer could be a valid SCTP packet.
			 */
			static bool IsPacket(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse a SCTP packet.
			 *
			 * @remarks
			 * - `length` must be the exact length of the Packet.
			 */
			static Packet* Parse(const uint8_t* buffer, size_t bufferLength);

			static Packet* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Constructor is private because we only want to create Packet instances
			 * via Parse() and Factory().
			 */
			Packet(const uint8_t* buffer, size_t bufferLength);

		public:
			~Packet() override final;
			;

			void Dump() const override final;

			void Serialize(uint8_t* buffer, size_t bufferLength) override final;

			Packet* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			uint16_t GetSourcePort() const
			{
				return uint16_t{ ntohs(GetHeaderPointer()->sourcePort) };
			}

			void SetSourcePort(uint16_t sourcePort)
			{
				AssertNotFrozen();

				GetHeaderPointer()->sourcePort = uint16_t{ htons(sourcePort) };
			}

			uint16_t GetDestinationPort() const
			{
				return uint16_t{ ntohs(GetHeaderPointer()->destinationPort) };
			}

			void SetDestinationPort(uint16_t destinationPort)
			{
				AssertNotFrozen();

				GetHeaderPointer()->destinationPort = uint16_t{ htons(destinationPort) };
			}

			uint32_t GetVerificationTag() const
			{
				return uint32_t{ ntohl(GetHeaderPointer()->verificationTag) };
			}

			void SetVerificationTag(uint32_t verificationTag)
			{
				AssertNotFrozen();

				GetHeaderPointer()->verificationTag = uint32_t{ htonl(verificationTag) };
			}

			uint32_t GetChecksum() const
			{
				return uint32_t{ ntohl(GetHeaderPointer()->checksum) };
			}

			void SetChecksum(uint32_t checksum)
			{
				AssertNotFrozen();

				GetHeaderPointer()->checksum = uint32_t{ htonl(checksum) };
			}

			bool HasChunks() const
			{
				return GetLength() > Packet::CommonHeaderLength;
			}

			size_t GetChunksCount() const
			{
				return this->chunks.size();
			}

			ChunksIterator ChunksBegin() const
			{
				return this->chunks.begin();
			}

			ChunksIterator ChunksEnd() const
			{
				return this->chunks.end();
			}

			const Chunk* GetChunkAt(size_t idx) const
			{
				if (idx >= this->chunks.size())
				{
					return nullptr;
				}

				return this->chunks[idx];
			}

			/**
			 * Clone given Chunk into Packet's buffer.
			 *
			 * @remarks
			 * Once this method is called, the caller may want to free the original
			 * given Chunk.
			 */
			void AddChunk(const Chunk* chunk);

		private:
			void InitializeHeader();

			/**
			 * NOTE: Return CommonHeader* instead of const CommonHeader* since we may
			 * want to modify its fields.
			 */
			CommonHeader* GetHeaderPointer() const
			{
				return reinterpret_cast<CommonHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			uint8_t* GetChunksPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + Packet::CommonHeaderLength;
			}

			/**
			 * Must be used within Parse() static method (instead than AddChunk()).
			 * This method doesn't serializa the given Chunk into Packet's buffer
			 * since it's already serialized (obviously since we are parsing a
			 * buffer).
			 */
			void AddParsedChunk(Chunk* chunk);

		private:
			// Chunks.
			std::vector<Chunk*> chunks;
		};
	} // namespace SCTP
} // namespace RTC

#endif
