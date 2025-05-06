#ifndef MS_RTC_SCTP_UNRECOGNIZED_PARAMETER_PARAMETER_HPP
#define MS_RTC_SCTP_UNRECOGNIZED_PARAMETER_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Unrecognized Parameter Parameter (UNRECOGNIZED_PARAMETER) (7).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 8            |            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                    Unrecognized Parameter                     /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UnrecognizedParameterParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UnrecognizedParameterParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static UnrecognizedParameterParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a UnrecognizedParameterParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static UnrecognizedParameterParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnrecognizedParameterParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static UnrecognizedParameterParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			UnrecognizedParameterParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnrecognizedParameterParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnrecognizedParameterParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnrecognizedParameter() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnrecognizedParameter() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnrecognizedParameterLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetUnrecognizedParameter(const uint8_t* parameter, uint16_t parameterLength);

		protected:
			virtual UnrecognizedParameterParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
