#ifndef MS_RTC_SCTP_COOKIE_ECHO_CHUNK_HPP
#define MS_RTC_SCTP_COOKIE_ECHO_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Cookie Echo Chunk (COOKIE_ECHO) (10).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |   Type = 10   |  Chunk Flags  |            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                            Cookie                             /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Chunk Type (8 bits): 10.
		 * - Flags (8 bits): All set to 0.
		 * - Length (16 bits).
		 * - Cookie (variable length).
		 */

		// Forward declaration.
		class Packet;

		class CookieEchoChunk : public Chunk
		{
			// We need that Packet calls protected and private methods in this class.
			friend class Packet;

		public:
			/**
			 * Parse a CookieEchoChunk.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk.
			 */
			static CookieEchoChunk* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a CookieEchoChunk.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Chunk real length.
			 */
			static CookieEchoChunk* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a CookieEchoChunk.
			 *
			 * @remarks
			 * To be used only by `Packet::Parse()`.
			 */
			static CookieEchoChunk* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			CookieEchoChunk(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~CookieEchoChunk() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual CookieEchoChunk* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			bool HasCookie() const
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetCookie() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetCookieLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetCookie(const uint8_t* cookie, uint16_t cookieLength);

		protected:
			virtual CookieEchoChunk* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
