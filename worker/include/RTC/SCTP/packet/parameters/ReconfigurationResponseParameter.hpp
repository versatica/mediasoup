#ifndef MS_RTC_SCTP_RECONFIGURATION_RESPONSE_PARAMETER_HPP
#define MS_RTC_SCTP_RECONFIGURATION_RESPONSE_PARAMETER_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include <string>
#include <unordered_map>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Re-configuration Response Parameter (RECONFIGURATION_RESPONSE)
		 * (16).
		 *
		 * @see RFC 6525.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |     Parameter Type = 16       |      Parameter Length         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |         Re-configuration Response Sequence Number             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                            Result                             |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                   Sender's Next TSN (optional)                |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                  Receiver's Next TSN (optional)               |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Forward declaration.
		class Chunk;

		class ReconfigurationResponseParameter : public Parameter
		{
			// We need that Chunk calls protected and private methods in this class.
			friend class Chunk;

		public:
			enum class Result : uint32_t
			{
				SUCCESS_NOTHING_TO_DO             = 0x00000000,
				SUCCESS_PERFORMED                 = 0x00000001,
				DENIED                            = 0x00000002,
				ERROR_WRONG_SSN                   = 0x00000003,
				ERROR_REQUEST_ALREADY_IN_PROGRESS = 0x00000004,
				ERROR_BAD_SEQUENCE_NUMBER         = 0x00000005,
				IN_PROGRESS                       = 0x00000006,
			};

		public:
			static const size_t ReconfigurationResponseParameterHeaderLength{ 12 };
			static const size_t ReconfigurationResponseParameterHeaderLengthWithOptionalFields{ 20 };

		public:
			/**
			 * Parse a ReconfigurationResponseParameter.
			 *
			 * @remarks
			 * `bufferLength` may exceed the exact length of the Parameter.
			 */
			static ReconfigurationResponseParameter* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create a ReconfigurationResponseParameter.
			 *
			 * @remarks
			 * `bufferLength` could be greater than the Parameter real length.
			 */
			static ReconfigurationResponseParameter* Factory(uint8_t* buffer, size_t bufferLength);

			static const std::string& Result2String(Result result);

		private:
			/**
			 * Parse a ReconfigurationResponseParameter.
			 *
			 * @remarks
			 * To be used only by `Chunk::ParseParameters()`.
			 */
			static ReconfigurationResponseParameter* ParseStrict(
			  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding);

		private:
			static std::unordered_map<Result, std::string> result2String;

		private:
			/**
			 * Only used by Parse(), ParseStrict() and Factory() static methods.
			 */
			ReconfigurationResponseParameter(uint8_t* buffer, size_t bufferLength);

		public:
			virtual ~ReconfigurationResponseParameter() override;

			virtual void Dump(int indentation = 0) const override final;

			virtual ReconfigurationResponseParameter* Clone(
			  uint8_t* buffer, size_t bufferLength) const override final;

			uint32_t GetReconfigurationResponseSequenceNumber() const
			{
				return Utils::Byte::Get4Bytes(GetBuffer(), 4);
			}

			void SetReconfigurationResponseSequenceNumber(uint32_t value);

			Result GetResult() const
			{
				return static_cast<Result>(Utils::Byte::Get4Bytes(GetBuffer(), 8));
			}

			void SetResult(Result result);

			bool HasNextTsns() const
			{
				return GetVariableLengthValueLength() ==
				       ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLengthWithOptionalFields -
				         ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLength;
			}

			uint32_t GetSenderNextTsn() const
			{
				if (!HasNextTsns())
				{
					return 0;
				}

				return Utils::Byte::Get4Bytes(GetBuffer(), 12);
			}

			uint32_t GetReceiverNextTsn() const
			{
				if (!HasNextTsns())
				{
					return 0;
				}

				return Utils::Byte::Get4Bytes(GetBuffer(), 16);
			}

			void SetNextTsns(uint32_t senderNextTsn, uint32_t receiverNextTsn);

		protected:
			virtual ReconfigurationResponseParameter* SoftClone(const uint8_t* buffer) const final override;

			/**
			 * We need to override this method since this Chunk has a variable-length
			 * value and the fixed header doesn't have default length.
			 */
			virtual size_t GetHeaderLength() const override final
			{
				return ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLength;
			}
		};
	} // namespace SCTP
} // namespace RTC

#endif
