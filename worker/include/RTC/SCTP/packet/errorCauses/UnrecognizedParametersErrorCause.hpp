#ifndef MS_RTC_SCTP_UNRECOGNIZED_PARAMETERS_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_UNRECOGNIZED_PARAMETERS_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Unrecognized Parameters Error Cause (UNRECOGNIZED_PARAMETERS) (8)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 8         |         Cause Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                    Unrecognized Parameters                    /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UnrecognizedParametersErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UnrecognizedParametersErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static UnrecognizedParametersErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a UnrecognizedParametersErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static UnrecognizedParametersErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnrecognizedParametersErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static UnrecognizedParametersErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			UnrecognizedParametersErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnrecognizedParametersErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnrecognizedParametersErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnrecognizedParameters() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnrecognizedParameters() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnrecognizedParametersLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetUnrecognizedParameters(const uint8_t* parameters, uint16_t parametersLength);

		protected:
			virtual UnrecognizedParametersErrorCause* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
