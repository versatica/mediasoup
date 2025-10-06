#ifndef MS_RTC_SCTP_USER_INITIATED_ABORT_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_USER_INITIATED_ABORT_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP User-Initiated Abort Error Cause (USER_INITIATED_ABORT) (12)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 12        |         Cause Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                   Upper Layer Abort Reason                    /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class UserInitiatedAbortErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a UserInitiatedAbortErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static UserInitiatedAbortErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a UserInitiatedAbortErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static UserInitiatedAbortErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a UserInitiatedAbortErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static UserInitiatedAbortErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			UserInitiatedAbortErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~UserInitiatedAbortErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual UserInitiatedAbortErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasUpperLayerAbortReason() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetUpperLayerAbortReason() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetUpperLayerAbortReasonLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetUpperLayerAbortReason(const uint8_t* reason, uint16_t reasonLength);

		protected:
			virtual UserInitiatedAbortErrorCause* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
