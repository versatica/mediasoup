#ifndef MS_RTC_SCTP_FORWARD_TSN_SUPPORTED_PARAMETER_HPP
#define MS_RTC_SCTP_FORWARD_TSN_SUPPORTED_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Forward-TSN-Supported Parameter (FORWARD_TSN_SUPPORTED) (49152).
		 *
		 * @see RFC 3758.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |    Parameter Type = 49152     |  Parameter Length = 4         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class ForwardTsnSupportedParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a ForwardTsnSupportedParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static ForwardTsnSupportedParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ForwardTsnSupportedParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static ForwardTsnSupportedParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ForwardTsnSupportedParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static ForwardTsnSupportedParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ForwardTsnSupportedParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ForwardTsnSupportedParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ForwardTsnSupportedParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

		protected:
			virtual ForwardTsnSupportedParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
