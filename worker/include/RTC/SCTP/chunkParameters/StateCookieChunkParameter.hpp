#ifndef MS_RTC_SCTP_STATE_COOKIE_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_STATE_COOKIE_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP State Cookie Chunk Parameter (STATE_COOKIE) (7).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 7            |            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                            Cookie                             /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class StateCookieChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a StateCookieChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static StateCookieChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a StateCookieChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static StateCookieChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a StateCookieChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static StateCookieChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			StateCookieChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~StateCookieChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual StateCookieChunkParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasCookie() const final
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
			virtual StateCookieChunkParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
