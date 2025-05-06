#ifndef MS_RTC_SCTP_COOKIE_RECEIVED_WHILE_SHUTTING_DOWN_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_COOKIE_RECEIVED_WHILE_SHUTTING_DOWN_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Cookie Received While Shutting Down Error Cause
		 * (COOKIE_RECEIVED_WHILE_SHUTTING_DOWN) (10)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 10        |       Cause Length = 4        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class CookieReceivedWhileShuttingDownErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a CookieReceivedWhileShuttingDownErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static CookieReceivedWhileShuttingDownErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a CookieReceivedWhileShuttingDownErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static CookieReceivedWhileShuttingDownErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a CookieReceivedWhileShuttingDownErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static CookieReceivedWhileShuttingDownErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			CookieReceivedWhileShuttingDownErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~CookieReceivedWhileShuttingDownErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual CookieReceivedWhileShuttingDownErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

		protected:
			virtual CookieReceivedWhileShuttingDownErrorCause* SoftClone(
			  const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
