#ifndef MS_RTC_SCTP_INVALID_STREAM_IDENTIFIER_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_INVALID_STREAM_IDENTIFIER_ERROR_CAUSE_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Invalid Stream Identifier Error Cause (INVALID_STREAM_IDENTIFIER)
		 * (1)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 1         |       Cause Length = 8        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |       Stream Identifier       |          (Reserved)           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class InvalidStreamIdentifierErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t InvalidStreamIdentifierErrorCauseHeaderLength{ 8 };

		public:
			/**
			 * Parse a InvalidStreamIdentifierErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static InvalidStreamIdentifierErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a InvalidStreamIdentifierErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static InvalidStreamIdentifierErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a InvalidStreamIdentifierErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static InvalidStreamIdentifierErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			InvalidStreamIdentifierErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~InvalidStreamIdentifierErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual InvalidStreamIdentifierErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint16_t GetStreamIdentifier() const
			{
				return Utils::Byte::Get2Bytes(GetBuffer(), 4);
			}

			void SetStreamIdentifier(uint16_t value);

		protected:
			virtual InvalidStreamIdentifierErrorCause* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Error Cause
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return InvalidStreamIdentifierErrorCause::InvalidStreamIdentifierErrorCauseHeaderLength;
			}

		private:
			void SetReserved();
		};
	} // namespace SCTP
} // namespace RTC

#endif
