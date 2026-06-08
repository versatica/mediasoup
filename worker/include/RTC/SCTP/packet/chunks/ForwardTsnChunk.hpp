#ifndef MS_RTC_SCTP_FORWARD_TSN_CHUNK_HPP
#define MS_RTC_SCTP_FORWARD_TSN_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Forward Cumulative TSN Chunk (FORWARD-TSN) (192)
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
		 * - Chunk type (8 bits): 192.
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

		class ForwardTsnChunk : public AnyForwardTsnChunk
		{
			// We need that packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t ForwardTsnChunkHeaderLength{ 8 };

		public:
			/**
			 * Parse a ForwardTsnChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the chunk.
			 */
			static ForwardTsnChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ForwardTsnChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the chunk real length.
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
			~ForwardTsnChunk() override;

			void Dump(int indentation = 0) const final;

			ForwardTsnChunk* Clone(uint8_t* buffer, size_t bufferLength) const final;

			bool CanHaveParameters() const final
			{
				return false;
			}

			bool CanHaveErrorCauses() const final
			{
				return false;
			}

			uint32_t GetNewCumulativeTsn() const final
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetNewCumulativeTsn(uint32_t value);

			uint16_t GetNumberOfSkippedStreams() const final
			{
				return GetVariableLengthValueLength() / 4;
			}

			std::vector<AnyForwardTsnChunk::SkippedStream> GetSkippedStreams() const final;

			void AddSkippedStream(const AnyForwardTsnChunk::SkippedStream& skippedStream);

		protected:
			ForwardTsnChunk* SoftClone(const uint8_t* buffer) const final;

			/**
			 * We need to override this method since this chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			size_t GetHeaderLength() const final
			{
				return ForwardTsnChunk::ForwardTsnChunkHeaderLength;
			}

		private:
			uint16_t GetSkippedStreamIdAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 4));
			}

			uint16_t GetStreamSequenceAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 4) + 2);
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
