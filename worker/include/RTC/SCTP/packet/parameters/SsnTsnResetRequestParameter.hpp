#ifndef MS_RTC_SCTP_SSN_TSN_RESET_REQUEST_PARAMETER_HPP
#define MS_RTC_SCTP_SSN_TSN_RESET_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Zero Checksum Acceptable Parameter (SSN-TSN-RESET-REQUEST)
		 * (15).
		 *
		 * @see RFC 6525.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |          Type = 0x000F        |          Length = 8           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |         Re-configuration Request Sequence Number              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class SsnTsnResetRequestParameter : public Parameter
		{
			// We need that chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static constexpr size_t SsnTsnResetRequestParameterHeaderLength{ 8 };

		public:
			/**
			 * Parse a SsnTsnResetRequestParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the parameter.
			 */
			static SsnTsnResetRequestParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a SsnTsnResetRequestParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the parameter real length.
			 */
			static SsnTsnResetRequestParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a SsnTsnResetRequestParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static SsnTsnResetRequestParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			SsnTsnResetRequestParameter(uint8_t* buffer, size_t bufferLength);

		public:
			~SsnTsnResetRequestParameter() override;

			void Dump(int indentation = 0) const final;

			SsnTsnResetRequestParameter* Clone(uint8_t* buffer, size_t bufferLength) const final;

			uint32_t GetReconfigurationRequestSequenceNumber() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetReconfigurationRequestSequenceNumber(uint32_t value);

		protected:
			SsnTsnResetRequestParameter* SoftClone(const uint8_t* buffer) const final;

			/**
			 * We don't really need to override this method since this parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			size_t GetHeaderLength() const final
			{
				return SsnTsnResetRequestParameter::SsnTsnResetRequestParameterHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
