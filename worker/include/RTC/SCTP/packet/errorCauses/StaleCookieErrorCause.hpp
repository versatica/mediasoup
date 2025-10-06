#ifndef MS_RTC_SCTP_STALE_COOKIE_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_STALE_COOKIE_ERROR_CAUSE_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Stale Cookie Error Cause (STALE_COOKIE) (3)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 3         |       Cause Length = 8        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                 Measure of Staleness (usec.)                  |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class StaleCookieErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t StaleCookieErrorCauseHeaderLength{ 8 };

		public:
			/**
			 * Parse a StaleCookieErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static StaleCookieErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a StaleCookieErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static StaleCookieErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a StaleCookieErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static StaleCookieErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			StaleCookieErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~StaleCookieErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual StaleCookieErrorCause* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetMeasureOfStaleness() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetMeasureOfStaleness(uint32_t value);

		protected:
			virtual StaleCookieErrorCause* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Error Cause
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return StaleCookieErrorCause::StaleCookieErrorCauseHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
