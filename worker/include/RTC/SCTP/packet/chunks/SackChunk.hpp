#ifndef MS_RTC_SCTP_SACK_CHUNK_HPP
#define MS_RTC_SCTP_SACK_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Selective Acknowledgement Chunk (SACK) (3)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 3    |  Chunk Flags  |         Chunk Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      Cumulative TSN Ack                       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          Advertised Receiver Window Credit (a_rwnd)           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * | Number of Gap Ack Blocks = N  |  Number of Duplicate TSNs = M |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |    Gap Ack Block #1 Start     |     Gap Ack Block #1 End      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                                                               /
		 * \                              ...                              \
		 * /                                                               /
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |    Gap Ack Block #N Start     |     Gap Ack Block #N End      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                        Duplicate TSN 1                        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                                                               /
		 * \                              ...                              \
		 * /                                                               /
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                        Duplicate TSN M                        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 3.
		 * - Res (4 bits): All set to 0.
		 * - Length (16 bits).
		 * - Cumulative TSN Ack (32 bits): The largest TSN, such that all TSNs
		 *   smaller than or equal to it have been received and the next one has
		 *   not been received. In the case where no DATA chunk has been received,
		 *   this value is set to the peer's Initial TSN minus one.
		 * - Advertised Receiver Window Credit (a_rwnd) (32 bits): This field
		 *   indicates the updated receive buffer space in bytes of the sender of
		 *   this SACK chunk.
		 * - Number of Gap Ack Blocks (16 bits): Indicates the number of Gap Ack
		 *   Blocks included in this SACK chunk.
		 * - Number of Duplicate TSNs (16 bit): This field contains the number of
		 *   duplicate TSNs the endpoint has received. Each duplicate TSN is listed
		 *   following the Gap Ack Block list.
		 * - Gap Ack Blocks: These fields contain the Gap Ack Blocks. They are
		 *   repeated for each Gap Ack Block up to the number of Gap Ack Blocks
		 *   defined in the Number of Gap Ack Blocks field.
		 * - Gap Ack Block Start (16 bits): Indicates the Start offset TSN for this
		 *   Gap Ack Block.
		 * - Gap Ack Block End (16 bits): Indicates the End offset TSN for this Gap
		 *   Ack Block.
		 * - Duplicate TSN (32 bits): Indicates the number of times a TSN was
		 *   received in duplicate since the last SACK chunk was sent.
		 */

		// Forward declaration.
		class Packet;

		class SackChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t SackChunkHeaderLength{ 16 };

		public:
			/**
			 * Parse a SackChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static SackChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a SackChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static SackChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a SackChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static SackChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			SackChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~SackChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual SackChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetCumulativeTsnAck() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetCumulativeTsnAck(uint32_t value);

			uint32_t GetAdvertisedReceiverWindowCredit() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 8);
			}

			void SetAdvertisedReceiverWindowCredit(uint32_t value);

			uint16_t GetNumberOfGapAckBlocks() const
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 12);
			}

			uint16_t GetNumberOfDuplicateTsns() const
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 14);
			}

			uint16_t GetAckBlockStartAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetAckBlocksPointer(), (idx * 4));
			}

			uint16_t GetAckBlockEndAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetAckBlocksPointer(), (idx * 4) + 2);
			}

			uint32_t GetDuplicateTsnAt(uint16_t idx) const
			{
				return Utils::Byte::Get4Bytes(GetDuplicateTsnsPointer(), (idx * 4));
			}

			void AddAckBlock(uint16_t start, uint16_t end);

			void AddDuplicateTsn(uint32_t tsn);

		protected:
			virtual SackChunk* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We need to override this method since this Chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return SackChunk::SackChunkHeaderLength;
			}

		private:
			void SetNumberOfGapAckBlocks(uint16_t value);

			void SetNumberOfDuplicateTsns(uint16_t value);

			uint8_t* GetAckBlocksPointer() const
			{
				return GetVariableLengthValuePointer();
			}

			uint8_t* GetDuplicateTsnsPointer() const
			{
				return GetVariableLengthValuePointer() + (GetNumberOfGapAckBlocks() * 4);
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
