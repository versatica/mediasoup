#ifndef MS_RTC_SCTP_SSN_TSN_RESET_REQUEST_PARAMETER_HPP
#define MS_RTC_SCTP_SSN_TSN_RESET_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Zero Checksum Acceptable Parameter (SSN_TSN_RESET_REQUEST)
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
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t SsnTsnResetRequestParameterHeaderLength{ 8 };

		public:
			/**
			 * Parse a SsnTsnResetRequestParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static SsnTsnResetRequestParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a SsnTsnResetRequestParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
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
			virtual ~SsnTsnResetRequestParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual SsnTsnResetRequestParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetReconfigurationRequestSequenceNumber() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetReconfigurationRequestSequenceNumber(uint32_t value);

		protected:
			virtual SsnTsnResetRequestParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return SsnTsnResetRequestParameter::SsnTsnResetRequestParameterHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
