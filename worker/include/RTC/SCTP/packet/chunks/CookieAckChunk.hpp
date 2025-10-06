#ifndef MS_RTC_SCTP_COOKIE_ACK_CHUNK_HPP
#define MS_RTC_SCTP_COOKIE_ACK_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Cookie Acknowledgement Chunk (COOKIE_ACK) (11).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 11   |  Chunk Flags  |          Length = 4           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 11.
		 * - Flags (8 bits): All set to 0.
		 * - Length (16 bits): 4.
		 */

		// Forward declaration.
		class Packet;

		class CookieAckChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a CookieAckChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static CookieAckChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a CookieAckChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static CookieAckChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a CookieAckChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static CookieAckChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			CookieAckChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~CookieAckChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual CookieAckChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

		protected:
			virtual CookieAckChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
