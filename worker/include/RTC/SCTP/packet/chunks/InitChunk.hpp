#ifndef MS_RTC_SCTP_INIT_CHUNK_HPP
#define MS_RTC_SCTP_INIT_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Initiation Chunk (INIT) (1).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 1    |  Chunk Flags  |      Chunk Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         Initiate Tag                          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          Advertised Receiver Window Credit (a_rwnd)           |
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
		 * - Chunk Type (8 bits): 1.
		 * - Flags (8 bits): All set to 0.
		 * - Initiate Tag (32 bits): The receiver of the INIT chunk (the responding
		 *   end) records the value of the Initiate Tag parameter. This value MUST
		 *   be placed into the Verification Tag field of every SCTP packet that
		 *   the receiver of the INIT chunk transmits within this association.
		 * - Advertised Receiver Window Credit (a_rwnd) (32 bits): This value
		 *   represents the dedicated buffer space, in number of bytes, the sender
		 *   of the INIT chunk has reserved in association with this window. The
		 *   Advertised Receiver Window Credit MUST NOT be smaller than 1500.
		 * - Number of Outbound Streams (OS) (16 bits): Defines the number of
		 *   outbound streams the sender of this INIT chunk wishes to create in
		 *   this association. The value of 0 MUST NOT be used.
		 * - Number of Inbound Streams (MIS) (16 bits): Defines the maximum number
		 *   of streams the sender of this INIT chunk allows the peer end to create
		 *   in this association. The value 0 MUST NOT be used.
		 * - Initial TSN (I-TSN) (32 bits): Defines the TSN that the sender of the
		 *   INIT chunk will use initially.
		 *
		 * Variable-Length Parameters:
		 * - IPv4 Address (5), optional.
		 * - IPv6 Address (6), optional.
		 * - Cookie Preservative (9), optional.
		 * - Reserved for ECN Capable (32768, 0x8000), optional.
		 * - Host Name Address (11), deprecated: The receiver of an INIT chunk or an
		 *   INIT ACK containing a Host Name Address parameter MUST send an ABORT
		 *   chunk and MAY include an "Unresolvable Address" error cause.
		 * - Supported Address Types (12), optional.
		 */

		// Forward declaration.
		class Packet;

		class InitChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			static const size_t InitChunkHeaderLength{ 20 };

		public:
			/**
			 * Parse a InitChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static InitChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a InitChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static InitChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a InitChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static InitChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			InitChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~InitChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual InitChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

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
			virtual InitChunk* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Chunk doesn't
			 * have variable-length value (despite the fixed header doesn't have
			 * default length). Optional/Variable-Length Parameters and/or Error
			 * Causes don't account here.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return InitChunk::InitChunkHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
