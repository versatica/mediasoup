#ifndef MS_RTC_SCTP_IPV4_ADDRESS_CHUNK_PARAMETER_HPP
#define MS_RTC_SCTP_IPV4_ADDRESS_CHUNK_PARAMETER_HPP

#include "common.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * IPv4 Adress Chunk Parameter (IPV4 ADDRESS) (5).
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

		class IPv4AddressChunkParameter : public ChunkParameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t IPv4AddressChunkParameterLength{ 8 };

		public:
			/**
			 * Parse a IPv4AddressChunkParameter.
			 *
			 * @remarks
			 * - `bufferLength` may exceed the exact length of the Chunk.
			 */
			static IPv4AddressChunkParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a IPv4AddressChunkParameter.
			 *
			 * @remarks
			 * - `bufferLength` could be greater than the Parameter real length.
			 */
			static IPv4AddressChunkParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Private constructor used by Parse() and Factory() static methods.
			 */
			IPv4AddressChunkParameter(const uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~IPv4AddressChunkParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual IPv4AddressChunkParameter* Clone(uint8_t* buffer, size_t bufferLength) const override final;

			/**
			 * @return A pointer to a 4 bytes unsigned integer in network order
			 * representing the binary encoded IPv4 value.
			 */
			const uint8_t* GetIPv4Address() const
			{
				return GetValuePointer();
			}

			/**
			 * @param ip - A pointer to a 4 bytes unsigned integer in network order
			 * representing the binary encoded IPv4 value.
			 */
			void SetIPv4Address(const uint8_t* ip);
		};
	} // namespace SCTP
} // namespace RTC

#endif
