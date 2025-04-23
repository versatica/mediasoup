#ifndef MS_RTC_SCTP_IPV6_ADDRESS_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_IPV6_ADDRESS_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP IPv6 Adress Chunk Parameter (IPV6 ADDRESS) (6).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 6            |          Length = 20          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                                                               |
		 * |                         IPv6 Address                          |
		 * |                                                               |
		 * |                                                               |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class IPv6AddressChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t IPv6AddressChunkParameterHeaderLength{ 20 };

		public:
			/**
			 * Parse a IPv6AddressChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Chunk Parameter.
			 */
			static IPv6AddressChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a IPv6AddressChunkParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static IPv6AddressChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a IPv6AddressChunkParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static IPv6AddressChunkParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			IPv6AddressChunkParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~IPv6AddressChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual IPv6AddressChunkParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			/**
			 * @return A pointer to a 16 bytes unsigned integer in network order
			 * representing the binary encoded IPv6 value.
			 */
			const uint8_t* GetIPv6Address() const
			{
				return GetBuffer() + 4;
			}

			/**
			 * @param ip - A pointer to a 16 bytes unsigned integer in network order
			 * representing the binary encoded IPv6 value.
			 */
			void SetIPv6Address(const uint8_t* ip);

		protected:
			virtual IPv6AddressChunkParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Chunk Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return IPv6AddressChunkParameter::IPv6AddressChunkParameterHeaderLength;
			}

		private:
			void ResetIPv6Address();
		};
	} // namespace SCTP
} // namespace RTC

#endif
