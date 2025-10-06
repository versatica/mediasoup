#ifndef MS_RTC_SCTP_OUT_OF_INVALID_MANDATORY_PARAMETER_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_OUT_OF_INVALID_MANDATORY_PARAMETER_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Invalid Mandatory Parameter Error Cause
		 * (INVALID_MANDATORY_PARAMETER) (7)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 7         |       Cause Length = 4        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class InvalidMandatoryParameterErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a InvalidMandatoryParameterErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static InvalidMandatoryParameterErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a InvalidMandatoryParameterErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static InvalidMandatoryParameterErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a InvalidMandatoryParameterErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static InvalidMandatoryParameterErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			InvalidMandatoryParameterErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~InvalidMandatoryParameterErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual InvalidMandatoryParameterErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

		protected:
			virtual InvalidMandatoryParameterErrorCause* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
