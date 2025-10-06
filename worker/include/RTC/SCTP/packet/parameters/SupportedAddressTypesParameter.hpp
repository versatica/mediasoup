#ifndef MS_RTC_SCTP_SUPPORTED_ADDRESS_TYPES_PARAMETER_HPP
#define MS_RTC_SCTP_SUPPORTED_ADDRESS_TYPES_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Supported Address Types Parameter (SUPPORTED_ADDRESS_TYPES) (12).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 12           |            Length             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Address Type #1        |        Address Type #2        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                            ......                             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+-+-+-+-+-+-++-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class SupportedAddressTypesParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a SupportedAddressTypesParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static SupportedAddressTypesParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a SupportedAddressTypesParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static SupportedAddressTypesParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a SupportedAddressTypesParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static SupportedAddressTypesParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			SupportedAddressTypesParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~SupportedAddressTypesParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual SupportedAddressTypesParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint16_t GetNumberOfAddressTypes() const
			{
				return GetVariableLengthValueLength() / 2;
			}

			uint16_t GetAddressTypeAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 2));
			}

			void AddAddressType(uint16_t addressType);

		protected:
			virtual SupportedAddressTypesParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
