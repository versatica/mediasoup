#ifndef MS_RTC_SCTP_NO_USER_DATA_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_NO_USER_DATA_ERROR_CAUSE_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP No User Data Error Cause (NO_USER_DATA) (9)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 9         |       Cause Length = 8        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                              TSN                              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class NoUserDataErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t NoUserDataErrorCauseHeaderLength{ 8 };

		public:
			/**
			 * Parse a NoUserDataErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static NoUserDataErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a NoUserDataErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static NoUserDataErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a NoUserDataErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static NoUserDataErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			NoUserDataErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~NoUserDataErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual NoUserDataErrorCause* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetTsn() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetTsn(uint32_t value);

		protected:
			virtual NoUserDataErrorCause* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Error Cause
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return NoUserDataErrorCause::NoUserDataErrorCauseHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
