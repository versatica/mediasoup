#ifndef MS_RTC_SCTP_STATE_COOKIE_PARAMETER_HPP
#define MS_RTC_SCTP_STATE_COOKIE_PARAMETER_HPP

#include "common.hpp"
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
			virtual ~StateCookieParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual StateCookieParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

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
			virtual StateCookieParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
