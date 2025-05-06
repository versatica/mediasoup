#ifndef MS_RTC_SCTP_OUTGOING_SSN_RESET_REQUEST_PARAMETER_HPP
#define MS_RTC_SCTP_OUTGOING_SSN_RESET_REQUEST_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Outgoing SSN Reset Request Parameter (OUTGOING_SSN_RESET_REQUEST)
		 * (13).
		 *
		 * @see RFC 6525.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     Parameter Type = 13       | Parameter Length = 16 + 2 * N |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Re-configuration Request Sequence Number            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |           Re-configuration Response Sequence Number           |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                Sender's Last Assigned TSN                     |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |  Stream Number 1 (optional)   |    Stream Number 2 (optional) |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * /                            ......                             /
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |  Stream Number N-1 (optional) |    Stream Number N (optional) |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class OutgoingSsnResetRequestParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			static const size_t OutgoingSsnResetRequestParameterHeaderLength{ 16 };

		public:
			/**
			 * Parse a OutgoingSsnResetRequestParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static OutgoingSsnResetRequestParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a OutgoingSsnResetRequestParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static OutgoingSsnResetRequestParameter* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Parse a OutgoingSsnResetRequestParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static OutgoingSsnResetRequestParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			OutgoingSsnResetRequestParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~OutgoingSsnResetRequestParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual OutgoingSsnResetRequestParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetReconfigurationRequestSequenceNumber() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetReconfigurationRequestSequenceNumber(uint32_t value);

			uint32_t GetReconfigurationResponseSequenceNumber() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 8);
			}

			void SetReconfigurationResponseSequenceNumber(uint32_t value);

			uint32_t GetSenderLastAssignedTsn() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 12);
			}

			void SetSenderLastAssignedTsn(uint32_t value);

			uint16_t GetNumberOfStreams() const
			{
				return GetVariableLengthValueLength() / 2;
			}

			uint16_t GetStreamAt(uint16_t idx) const
			{
				return Utils::Byte::Get2Bytes(GetVariableLengthValuePointer(), (idx * 2));
			}

			void AddStream(uint16_t stream);

		protected:
			virtual OutgoingSsnResetRequestParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We need to override this method since this Chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return OutgoingSsnResetRequestParameter::OutgoingSsnResetRequestParameterHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
