#ifndef MS_RTC_SCTP_UNRESOLVABLE_ADDRESS_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_UNRESOLVABLE_ADDRESS_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Unresolvable Address Error Cause (UNRESOLVABLE_ADDRESS) (5)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 5         |         Cause Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                     Unresolvable Address                      /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UnresolvableAddressErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UnresolvableAddressErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static UnresolvableAddressErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a UnresolvableAddressErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static UnresolvableAddressErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UnresolvableAddressErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static UnresolvableAddressErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			UnresolvableAddressErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UnresolvableAddressErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UnresolvableAddressErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUnresolvableAddress() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUnresolvableAddress() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUnresolvableAddressLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetUnresolvableAddress(const uint8_t* address, uint16_t addressLength);

		protected:
			virtual UnresolvableAddressErrorCause* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
