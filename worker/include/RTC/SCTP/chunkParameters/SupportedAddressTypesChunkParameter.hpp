#ifndef MS_RTC_SCTP_SUPPORTED_ADDRESS_TYPES_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_SUPPORTED_ADDRESS_TYPES_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Supported Address Types Chunk Parameter
		 * (SUPPORTED_ADDRESS_TYPES) (12).
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

		class SupportedAddressTypesChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a SupportedAddressTypesChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static SupportedAddressTypesChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a SupportedAddressTypesChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static SupportedAddressTypesChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a SupportedAddressTypesChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static SupportedAddressTypesChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			SupportedAddressTypesChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~SupportedAddressTypesChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual SupportedAddressTypesChunkParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			const uint16_t GetNumberOfAddressTypes() const
			{
				return GetVariableLengthValueLength() / 2;
			}

			uint16_t GetAddressTypeAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 2));
			}

			void AddAddressType(uint16_t addressType);

		protected:
			virtual SupportedAddressTypesChunkParameter* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
