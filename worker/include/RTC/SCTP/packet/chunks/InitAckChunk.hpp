#ifndef MS_RTC_SCTP_INIT_ACK_CHUNK_HPP
#define MS_RTC_SCTP_INIT_ACK_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Initiation Acknowledgement Chunk (INIT_ACK) (2).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 2    |  Chunk Flags  |      Chunk Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         Initiate Tag                          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |               Advertised Receiver Window Credit               |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |  Number of Outbound Streams   |   Number of Inbound Streams   |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                          Initial TSN                          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /              Optional/Variable-Length Parameters              /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 2.
		 * - Flags (8 bits): All set to 0.
		 * - Initiate Tag (32 bits): The receiver of the INIT ACK chunk records the
		 *   value of the Initiate Tag parameter. This value MUST be placed into
		 *   the Verification Tag field of every SCTP packet that the receiver of
		 *   the INIT ACK chunk transmits within this association.
		 * - Advertised Receiver Window Credit (a_rwnd) (32 bits): This value
		 *   represents the dedicated buffer space, in number of bytes, the sender
		 *   of the INIT ACK chunk has reserved in association with this window.
		 *   The Advertised Receiver Window Credit MUST NOT be smaller than 1500.
		 * - Number of Outbound Streams (OS) (16 bits): Defines the number of
		 *   outbound streams the sender of this INIT ACK chunk wishes to create in
		 *   this association. The value of 0 MUST NOT be used.
		 * - Number of Inbound Streams (MIS) (16 bits): Defines the maximum number
		 *   of streams the sender of this INIT ACK chunk allows the peer end to
		 *   create in this association. The value 0 MUST NOT be used.
		 * - Initial TSN (I-TSN) (32 bits): Defines the TSN that the sender of the
		 *   INIT ACK chunk will use initially.
		 *
		 * Variable-Length Parameters:
		 * - State Cookie (7), mandatory.
		 * - IPv4 Address (5), optional.
		 * - IPv6 Address (6), optional.
		 * - Unrecognized Parameter	(8), optional.
		 * - Reserved for ECN Capable (32768, 0x8000), optional.
		 * - Host Name Address (11), deprecated: The receiver of an INIT chunk or an
		 *   INIT ACK containing a Host Name Address parameter MUST send an ABORT
		 *   chunk and MAY include an "Unresolvable Address" error cause.
		 */

		// Forward declaration.
		class Packet;

		class InitAckChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t InitAckChunkHeaderLength{ 20 };

		public:
			/**
			 * Parse a InitAckChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static InitAckChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a InitAckChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static InitAckChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a InitAckChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static InitAckChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			InitAckChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~InitAckChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual InitAckChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool CanHaveParameters() const final
			{
				return true;
			}

			uint32_t GetInitiateTag() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetInitiateTag(uint32_t value);

			uint32_t GetAdvertisedReceiverWindowCredit() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 8);
			}

			void SetAdvertisedReceiverWindowCredit(uint32_t value);

			uint16_t GetNumberOfOutboundStreams() const
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 12);
			}

			void SetNumberOfOutboundStreams(uint16_t value);

			uint16_t GetNumberOfInboundStreams() const
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 14);
			}

			void SetNumberOfInboundStreams(uint16_t value);

			uint32_t GetInitialTsn() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 16);
			}

			void SetInitialTsn(uint32_t value);

		protected:
			virtual InitAckChunk* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Chunk doesn't
			 * have variable-length value (despite the fixed header doesn't have
			 * default length). Optional/Variable-Length Parameters and/or Error
			 * Causes don't account here.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return InitAckChunk::InitAckChunkHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
