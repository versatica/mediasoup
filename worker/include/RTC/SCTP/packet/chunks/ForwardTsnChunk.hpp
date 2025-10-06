#ifndef MS_RTC_SCTP_FORWARD_TSN_CHUNK_HPP
#define MS_RTC_SCTP_FORWARD_TSN_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Forward Cumulative TSN Chunk (FORWARD_TSN) (192)
		 *
		 * @see RFC 3758.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 192  |  Flags = 0x00 |        Length = Variable      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      New Cumulative TSN                       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |         Stream-1              |       Stream Sequence-1       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               /
		 * /                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |         Stream-N              |       Stream Sequence-N       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 192.
		 * - Flags: All set to 0.
		 * - Length (16 bits).
		 * - New Cumulative TSN (32 bits): This indicates the new cumulative TSN to
		 *   the data receiver.
		 * - Stream-N (16 bits): Stream number that was skipped by this FWD-TSN.
		 * - Stream Sequence-N (16 bit): Sequence number associated with the stream
		 *   that was skipped.
		 */

		// Forward declaration.
		class Packet;

		class ForwardTsnChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t ForwardTsnChunkHeaderLength{ 8 };

		public:
			/**
			 * Parse a ForwardTsnChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static ForwardTsnChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ForwardTsnChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static ForwardTsnChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ForwardTsnChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static ForwardTsnChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ForwardTsnChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ForwardTsnChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ForwardTsnChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetNewCumulativeTsn() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetNewCumulativeTsn(uint32_t value);

			uint16_t GetNumberOfStreams() const
			{
				return GetVariableLengthValueLength() / 4;
			}

			uint16_t GetStreamAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 4));
			}

			uint16_t GetStreamSequenceAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 4) + 2);
			}

			void AddStream(uint16_t stream, uint16_t streamSequence);

		protected:
			virtual ForwardTsnChunk* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We need to override this method since this Chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return ForwardTsnChunk::ForwardTsnChunkHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
