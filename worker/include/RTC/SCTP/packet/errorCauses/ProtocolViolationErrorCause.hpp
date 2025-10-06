#ifndef MS_RTC_SCTP_PROTOCOL_VIOLATION_ERROR_CAUSE_HPP
#define MS_RTC_SCTP_PROTOCOL_VIOLATION_ERROR_CAUSE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Protocol Violation Error Cause (PROTOCOL_VIOLATION) (13)
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |        Cause Code = 13        |         Cause Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                    Additional Information                     /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class ProtocolViolationErrorCause : public ErrorCause
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			/**
			 * Parse a ProtocolViolationErrorCause.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Error Cause.
			 */
			static ProtocolViolationErrorCause* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ProtocolViolationErrorCause.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Error Cause real length.
			 */
			static ProtocolViolationErrorCause* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a ProtocolViolationErrorCause.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseErrorCauses()`.
			 */
			static ProtocolViolationErrorCause* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t causeLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse() and ParseStrict() static methods.
			 */
			ProtocolViolationErrorCause(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ProtocolViolationErrorCause() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ProtocolViolationErrorCause* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			virtual bool HasAdditionalInformation() const final
			{
				return HasVariableLengthValue();
			}

			const uint8_t* GetAdditionalInformation() const
			{
				return GetVariableLengthValue();
			}

			uint16_t GetAdditionalInformationLength() const
			{
				return GetVariableLengthValueLength();
			}

			void SetAdditionalInformation(const uint8_t* info, uint16_t infoLength);

		protected:
			virtual ProtocolViolationErrorCause* SoftClone(const uint8_t* buffer) const final override;
		};
	} // namespace SCTP
} // namespace RTC

#endif
