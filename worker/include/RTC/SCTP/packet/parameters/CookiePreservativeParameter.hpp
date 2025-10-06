#ifndef MS_RTC_SCTP_COOKIE_PRESERVATIVE_PARAMETER_HPP
#define MS_RTC_SCTP_COOKIE_PRESERVATIVE_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Cookie Preservative Parameter (COOKIE_PRESERVATIVE) (9).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 9            |          Length = 8           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |         Suggested Cookie Life-Span Increment (msec.)          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class CookiePreservativeParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t CookiePreservativeParameterHeaderLength{ 8 };

		public:
			/**
			 * Parse a CookiePreservativeParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static CookiePreservativeParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a CookiePreservativeParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static CookiePreservativeParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a CookiePreservativeParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static CookiePreservativeParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			CookiePreservativeParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~CookiePreservativeParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual CookiePreservativeParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetLifeSpanIncrement() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetLifeSpanIncrement(uint32_t increment);

		protected:
			virtual CookiePreservativeParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return CookiePreservativeParameter::CookiePreservativeParameterHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
