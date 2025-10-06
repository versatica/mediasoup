#ifndef MS_RTC_SCTP_IPV4_ADDRESS_PARAMETER_HPP
#define MS_RTC_SCTP_IPV4_ADDRESS_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP IPv4 Adress Parameter (IPV4 ADDRESS) (5).
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Type = 5            |          Length = 8           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         IPv4 Address                          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class IPv4AddressParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t IPv4AddressParameterHeaderLength{ 8 };

		public:
			/**
			 * Parse a IPv4AddressParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static IPv4AddressParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a IPv4AddressParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static IPv4AddressParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a IPv4AddressParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static IPv4AddressParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			IPv4AddressParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~IPv4AddressParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual IPv4AddressParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			/**
			 * @return A pointer to a 4 bytes unsigned integer in network order
			 * representing the binary encoded IPv4 value.
			 */
			const uint8_t* GetIPv4Address() const
			{
				return GetBuffer() + 4;
			}

			/**
			 * @param ip - A pointer to a 4 bytes unsigned integer in network order
			 * representing the binary encoded IPv4 value.
			 */
			void SetIPv4Address(const uint8_t* ip);

		protected:
			virtual IPv4AddressParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We don't really need to override this method since this Parameter
			 * doesn't have variable-length value (despite the fixed header doesn't
			 * have default length).
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return IPv4AddressParameter::IPv4AddressParameterHeaderLength;
			}

		private:
			void ResetIPv4Address();
		};
	} // namespace SCTP
} // namespace RTC

#endif
