#ifndef MS_RTC_SCTP_I_DATA_CHUNK_HPP
#define MS_RTC_SCTP_I_DATA_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP I-Data Chunk (I_DATA) (64).
		 *
		 * @see RFC 8260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 64   |  Res  |I|U|B|E|       Length = Variable       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                              TSN                              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Stream Identifier      |          (Reserved)           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                      Message Identifier                       |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |    Payload Protocol Identifier / Fragment Sequence Number     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                           User Data                           /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 64.
		 * - Res (4 bits): All set to 0.
		 * - I bit (1 bit): The (I)mmediate Bit, if set, indicates that the
		 *   receiver SHOULD NOT delay the sending of the corresponding SACK chunk.
		 * - U bit (1 bit): The (U)nordered bit, if set, indicates the user message
		 *   is unordered.
		 * - B bit (1 bit): The (B)eginning fragment bit, if set, indicates the
		 *   first fragment of a user message.
		 * - E bit (1 bit): The (E)nding fragment bit, if set, indicates the last
		 *   fragment of a user message.
		 * - Length (16 bits): This field indicates the length of the I-DATA chunk in
		 *   bytes from the beginning of the type field to the end of the User Data
		 *   field excluding any padding.
		 * - TSN (32 bits): This value represents the TSN for this I-DATA chunk.
		 * - Stream Identifier (16 bits): Identifies the stream to which the user
		 *   data belongs.
		 * - Reserved (16 bits): All set to zero.
		 * - Message Identifier (MID) (32 bits): The MID is the same for all
		 *   fragments of a user message. It is used to determine which fragments
		 *   (enumerated by the FSN) belong to the same user message. For ordered
		 *   user messages, the MID is also used by the SCTP receiver to deliver
		 *   the user messages in the correct order to the upper layer.
		 * - Payload Protocol Identifier (PPID) / Fragment Sequence Number (FSN)
		 *   (32 bits): If the B bit is set, this field contains the PPID of the
		 *   user message. If the B bit is not set, this field contains the FSN.
		 *   The FSN is used to enumerate all fragments of a single user message,
		 *   starting from 0 and incremented by 1. The last fragment of a message
		 *   MUST have the E bit set.
		 * - User Data (variable length): This is the payload user data.
		 */

		// Forward declaration.
		class Packet;

		class IDataChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t IDataChunkHeaderLength{ 20 };

		public:
			/**
			 * Parse a IDataChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static IDataChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a IDataChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static IDataChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a IDataChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static IDataChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			IDataChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~IDataChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual IDataChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

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

			uint16_t GetStreamIdentifier() const
			{
				return Utils::Byte::Get2Bytes(const_cast<uint8_t*>(GetBuffer()), 8);
			}

			void SetStreamIdentifier(uint16_t value);

			uint32_t GetMessageIdentifier() const
			{
				return Utils::Byte::Get4Bytes(const_cast<uint8_t*>(GetBuffer()), 12);
			}

			void SetMessageIdentifier(uint32_t value);

			uint32_t GetPayloadProtocolIdentifierOrFragmentSequenceNumber() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 16);
			}

			void SetPayloadProtocolIdentifierOrFragmentSequenceNumber(uint32_t value);

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
			virtual IDataChunk* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We need to override this method since this Chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return IDataChunk::IDataChunkHeaderLength;
			}

		private:
			void SetReserved();
		};
	} // namespace SCTP
} // namespace RTC

#endif
