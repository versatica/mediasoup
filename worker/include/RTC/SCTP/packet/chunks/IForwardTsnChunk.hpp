#ifndef MS_RTC_SCTP_I_FORWARD_TSN_CHUNK_HPP
#define MS_RTC_SCTP_I_FORWARD_TSN_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP I-Forward Cumulative TSN Chunk (I_FORWARD_TSN) (194)
		 *
		 * @see RFC 8260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 194  | Flags = 0x00  |      Length = Variable        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                       New Cumulative TSN                      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |       Stream Identifier       |         (Reserved)          |U|
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                       Message Identifier                      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                                                               /
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |       Stream Identifier       |         (Reserved)          |U|
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                       Message Identifier                      |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 194.
		 * - Flags: All set to 0.
		 * - Length (16 bits).
		 * - New Cumulative TSN (32 bits): This indicates the new cumulative TSN to
		 *   the data receiver.
		 * - Stream Identifier (SID) (16 bits): This field holds the stream number
		 *   this entry refers to.
		 * - Reserved (15 bits).
		 * - U bit (1 bit): The U bit specifies if the Message Identifier of this
		 *   entry refers to unordered messages (U bit is set) or ordered messages
		 *   (U bit is not set).
		 * - Message Identifier (MID) (32 bits): This field holds the largest
		 *   Message Identifier for ordered or unordered messages indicated by the
		 *   U bit that was skipped for the stream specified by the Stream
		 *   Identifier. For ordered messages, this is similar to the FORWARD-TSN
		 *   chunk, just replacing the 16-bit SSN by the 32-bit MID.
		 */

		// Forward declaration.
		class Packet;

		class IForwardTsnChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t IForwardTsnChunkHeaderLength{ 8 };

		public:
			/**
			 * Parse a IForwardTsnChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static IForwardTsnChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a IForwardTsnChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static IForwardTsnChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a IForwardTsnChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static IForwardTsnChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			IForwardTsnChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~IForwardTsnChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual IForwardTsnChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetNewCumulativeTsn() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetNewCumulativeTsn(uint32_t value);

			uint16_t GetNumberOfStreams() const
			{
				return GetVariableLengthValueLength() / 8;
			}

			uint16_t GetStreamAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 8));
			}

			bool GetUFlagAt(uint16_t idx) const
			{
				return (Utils::Byte::Get1Byte(GetVariableLengthValuePointer(), (idx * 8) + 3) & 0x01) != 0;
			}

			uint32_t GetMessageIdentifierAt(uint16_t idx) const
			{
				return Utils::Byte::Get4Bytes(GetVariableLengthValuePointer(), (idx * 8) + 4);
			}

			void AddStream(uint16_t stream, bool uFlag, uint32_t messageIdentifier);

		protected:
			virtual IForwardTsnChunk* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We need to override this method since this Chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return IForwardTsnChunk::IForwardTsnChunkHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
