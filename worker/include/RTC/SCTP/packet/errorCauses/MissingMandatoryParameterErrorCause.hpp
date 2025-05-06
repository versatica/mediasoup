#ifndef MS_RTC_SCTP_MISSING_MANDATORY_PARAMETER_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_MISSING_MANDATORY_PARAMETER_ERROR_CAUSE_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Missing Mandatory Parameter Error Cause
		 * (MISSING_MANDATORY_PARAMETER) (2)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 2         |   Cause Length = 8 + N * 2    |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                 Number of missing params = N                  |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     Missing Param Type #1     |     Missing Param Type #2     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |    Missing Param Type #N-1    |     Missing Param Type #N     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class MissingMandatoryParameterErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t MissingMandatoryParameterErrorCauseHeaderLength{ 8 };

		public:
			/**
			 * Parse a MissingMandatoryParameterErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static MissingMandatoryParameterErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a MissingMandatoryParameterErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static MissingMandatoryParameterErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a MissingMandatoryParameterErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static MissingMandatoryParameterErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			MissingMandatoryParameterErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~MissingMandatoryParameterErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual MissingMandatoryParameterErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetNumberOfMissingParameters() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			Parameter::ParameterType GetMissingParameterTypeAt(uint32_t idx) const
			{
				return static_cast<Parameter::ParameterType>(
				  Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 2)));
			}

			void AddMissingParameterType(Parameter::ParameterType parameterType);

		protected:
			virtual MissingMandatoryParameterErrorCause* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Error Cause
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return MissingMandatoryParameterErrorCause::MissingMandatoryParameterErrorCauseHeaderLength;
			}

		private:
			void SetNumberOfMissingParameters(uint32_t value);
		};
	} // namespace SCTP
} // namespace RTC

#endif
