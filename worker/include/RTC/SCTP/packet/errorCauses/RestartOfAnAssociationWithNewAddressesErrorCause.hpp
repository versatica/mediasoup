#ifndef MS_RTC_SCTP_RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Restart of an Association with New Addresses Error Cause
		 * (RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES) (11)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 11        |         Cause Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                       New Address TLVs                        /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class RestartOfAnAssociationWithNewAddressesErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a RestartOfAnAssociationWithNewAddressesErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static RestartOfAnAssociationWithNewAddressesErrorCause* Parse(
			  const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a RestartOfAnAssociationWithNewAddressesErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static RestartOfAnAssociationWithNewAddressesErrorCause* Factory(
			  uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a RestartOfAnAssociationWithNewAddressesErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static RestartOfAnAssociationWithNewAddressesErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			RestartOfAnAssociationWithNewAddressesErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~RestartOfAnAssociationWithNewAddressesErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual RestartOfAnAssociationWithNewAddressesErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasNewAddressTlvs() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetNewAddressTlvs() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetNewAddressTlvsLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetNewAddressTlvs(const uint8_t* tlvs, uint16_t tlvsLength);

		protected:
			virtual RestartOfAnAssociationWithNewAddressesErrorCause* SoftClone(
			  const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
