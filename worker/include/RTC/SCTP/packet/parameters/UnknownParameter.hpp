#ifndef MS_RTC_SCTP_UNKNOWN_PARAMETER_HPP
#define MS_RTC_SCTP_UNKNOWN_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Unknown Parameter (UNKNOWN).
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Parameter Type         |       Parameter Length        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                         Unknown Value                         /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UnknownParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UnknownParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static UnknownParameter* Parse(const uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnknownParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static UnknownParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			UnknownParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnknownParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnknownParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnknownType() const override
			{
				return true;
			}

			virtual bool HasUnknownValue() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnknownValue() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnknownValueLength() const
			{
				return GetVariableLengthValueLength();
			}

		protected:
			virtual UnknownParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
