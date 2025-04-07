#ifndef MS_RTC_SCTP_PACKET_HPP
#define MS_RTC_SCTP_PACKET_HPP

#include "common.hpp"
// #include "RTC/SCTP/Chunk.hpp"
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
			static bool IsSctp(const uint8_t* buffer, size_t bufferLength);

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
				return uint16_t{ ntohs(GetCommonHeaderPointer()->sourcePort) };
			}

			void SetSourcePort(uint16_t sourcePort)
			{
				AssertNotFrozen();

				GetCommonHeaderPointer()->sourcePort = uint16_t{ htons(sourcePort) };
			}

			uint16_t GetDestinationPort() const
			{
				return uint16_t{ ntohs(GetCommonHeaderPointer()->destinationPort) };
			}

			void SetDestinationPort(uint16_t destinationPort)
			{
				AssertNotFrozen();

				GetCommonHeaderPointer()->destinationPort = uint16_t{ htons(destinationPort) };
			}

			uint32_t GetVerificationTag() const
			{
				return uint32_t{ ntohl(GetCommonHeaderPointer()->verificationTag) };
			}

			void SetVerificationTag(uint32_t verificationTag)
			{
				AssertNotFrozen();

				GetCommonHeaderPointer()->verificationTag = uint32_t{ htonl(verificationTag) };
			}

			uint32_t GetChecksum() const
			{
				return uint32_t{ ntohl(GetCommonHeaderPointer()->checksum) };
			}

			void SetChecksum(uint32_t checksum)
			{
				AssertNotFrozen();

				GetCommonHeaderPointer()->checksum = uint32_t{ htonl(checksum) };
			}

			// void AddChunk(Chunk* chunk)
			// {
			// AssertNotFrozen();
			//
			// 	this->chunks.push_back(chunk);
			// }

		private:
			/**
			 * NOTE: Return CommonHeader* instead of const CommonHeader* since we may
			 * want to modify its fields.
			 */
			CommonHeader* GetCommonHeaderPointer() const
			{
				return reinterpret_cast<CommonHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			const uint8_t* GetChunksPointer() const
			{
				return GetBuffer() + Packet::CommonHeaderLength;
			}

			virtual const uint8_t* GetEndPointer() const final
			{
				return GetBuffer() + GetBufferLength();
			}

			/**
			 * Must be used within Parse() static method (instead than AddChunk()).
			 * This method doesn't serializa the given Chunk into Packet's buffer
			 * since it's already serialized (obviously since we are parsing a
			 * buffer).
			 */
			// void AddParsedChunk(Chunk* chunk);

		private:
			// Chunks.
			// std::vector<Chunk*> chunks;
		};
	} // namespace SCTP
} // namespace RTC

#endif
