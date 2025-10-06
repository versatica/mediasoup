#ifndef MS_RTC_SCTP_IPV6_ADDRESS_PARAMETER_HPP
#define MS_RTC_SCTP_IPV6_ADDRESS_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP IPv6 Adress Parameter (IPV6_ADDRESS) (6).
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

		class IPv6AddressParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t IPv6AddressParameterHeaderLength{ 20 };

		public:
			/**
			 * Parse a IPv6AddressParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static IPv6AddressParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a IPv6AddressParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static IPv6AddressParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a IPv6AddressParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static IPv6AddressParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			IPv6AddressParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~IPv6AddressParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual IPv6AddressParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

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
			virtual IPv6AddressParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return IPv6AddressParameter::IPv6AddressParameterHeaderLength;
			}

		private:
			void ResetIPv6Address();
		};
	} // namespace SCTP
} // namespace RTC

#endif
