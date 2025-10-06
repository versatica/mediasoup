#ifndef MS_RTC_SCTP_DATA_CHUNK_HPP
#define MS_RTC_SCTP_DATA_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Payload Data Chunk (DATA) (0).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 0    |  Res  |I|U|B|E|            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                              TSN                              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |      Stream Identifier S      |   Stream Sequence Number n    |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                  Payload Protocol Identifier                  |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                 User Data (seq n of Stream S)                 /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 0.
		 * - Res (4 bits): All set to 0.
		 * - I bit (1 bit): The (I)mmediate bit MAY be set by the sender whenever
		 *   the sender of a DATA chunk can benefit from the corresponding SACK
		 *   chunk being sent back without delay.
		 * - U bit (1 bit): The (U)nordered bit, if set to 1, indicates that this
		 *   is an unordered DATA chunk, and there is no Stream Sequence Number
		 *   assigned to this DATA chunk.
		 * - B bit (1 bit): The (B)eginning fragment bit, if set, indicates the
		 *   first fragment of a user message.
		 * - E bit (1 bit): The (E)nding fragment bit, if set, indicates the last
		 *   fragment of a user message.
		 * - Length (16 bits): This field indicates the length of the DATA chunk in
		 *   bytes from the beginning of the type field to the end of the User Data
		 *   field excluding any padding. A DATA chunk with one byte of user data
		 *   will have the Length field set to 17 (indicating 17 bytes). A DATA
		 *   chunk with a User Data field of length L will have the Length field
		 *   set to (16 + L) (indicating 16 + L bytes) where L MUST be greater than
		 *   0.
		 * - TSN (32 bits): This value represents the TSN for this DATA chunk. The
		 *   valid range of TSN is from 0 to 4294967295 (232 - 1). TSN wraps back
		 *   to 0 after reaching 4294967295.
		 * - Stream Identifier S (16 bits): Identifies the stream to which the
		 *   following user data belongs.
		 * - Stream Sequence Number n (16 bits): This value represents the Stream
		 *   Sequence Number of the following user data within the stream S. Valid
		 *   range is 0 to 65535. When a user message is fragmented by SCTP for
		 *   transport, the same Stream Sequence Number MUST be carried in each of
		 *   the fragments of the message.
		 * - Payload Protocol Identifier (32 bits): This value represents an
		 *   application (or upper layer) specified protocol identifier.
		 * - User Data (variable length): This is the payload user data. The
		 *   implementation MUST pad the end of the data to a 4-byte boundary with
		 *   all zero bytes. Any padding MUST NOT be included in the Length field.
		 */

		// Forward declaration.
		class Packet;

		class DataChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t DataChunkHeaderLength{ 16 };

		public:
			/**
			 * Parse a DataChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static DataChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a DataChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static DataChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a DataChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static DataChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			DataChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~DataChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual DataChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			bool GetI() const
			{
				return GetBit3();
			}

			void SetI(bool flag);

			bool GetU() const
			{
				return GetBit2();
			}

			void SetU(bool flag);

			bool GetB() const
			{
				return GetBit1();
			}

			void SetB(bool flag);

			bool GetE() const
			{
				return GetBit0();
			}

			void SetE(bool flag);

			uint32_t GetTsn() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetTsn(uint32_t value);

			uint16_t GetStreamIdentifierS() const
			{
				return Utils::Byte::Get2Bytes(const_cast<uint8_t*>(GetBuffer()), 8);
			}

			void SetStreamIdentifierS(uint16_t value);

			uint16_t GetStreamSequenceNumberN() const
			{
				return Utils::Byte::Get2Bytes(const_cast<uint8_t*>(GetBuffer()), 10);
			}

			void SetStreamSequenceNumberN(uint16_t value);

			uint32_t GetPayloadProtocolIdentifier() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 12);
			}

			void SetPayloadProtocolIdentifier(uint32_t value);

			bool HasUserData() const
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUserData() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUserDataLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetUserData(const uint8_t* userData, uint16_t userDataLength);

		protected:
			virtual DataChunk* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We need to override this method since this Chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return DataChunk::DataChunkHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
