#ifndef MS_RTC_SCTP_STATE_COOKIE_PARAMETER_HPP
#define MS_RTC_SCTP_STATE_COOKIE_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP State Cookie Parameter (STATE_COOKIE) (7).
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

		class StateCookieParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a StateCookieParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static StateCookieParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a StateCookieParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static StateCookieParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a StateCookieParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static StateCookieParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			StateCookieParameter(uint8_t* buffer, size_t bufferLength);

		public:
			~StateCookieParameter() override;

			void Dump(int indentation = 0) const final;

			StateCookieParameter* Clone(uint8_t* buffer, size_t bufferLength) const final;

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

			/**
			 * Write a locally generated StateCookie in place within the Cookie
			 * field.
			 *
			 * This method is more performant than SetCookie() since it doesn't
			 * require neither the allocation of a StateCookie class instance nor a
			 * copy of its buffer to the StateCookieParameter.
			 */
			void WriteStateCookieInPlace(
			  uint32_t localVerificationTag,
			  uint32_t remoteVerificationTag,
			  uint32_t localInitialTsn,
			  uint32_t remoteInitialTsn,
			  uint32_t remoteAdvertisedReceiverWindowCredit,
			  uint64_t tieTag,
			  const NegotiatedCapabilities& negotiatedCapabilities);

		protected:
			StateCookieParameter* SoftClone(const uint8_t* buffer) const final;
		};
	} // namespace SCTP
} // namespace RTC

#endif
