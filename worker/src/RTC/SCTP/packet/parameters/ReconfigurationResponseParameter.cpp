#define MS_CLASS "RTC::SCTP::ReconfigurationResponseParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		std::unordered_map<ReconfigurationResponseParameter::Result, std::string> ReconfigurationResponseParameter::result2String =
		{
			{ ReconfigurationResponseParameter::Result::SUCCESS_NOTHING_TO_DO,             "SUCCESS_NOTHING_TO_DO"             },
			{ ReconfigurationResponseParameter::Result::SUCCESS_PERFORMED,                 "SUCCESS_PERFORMED"                 },
			{ ReconfigurationResponseParameter::Result::DENIED,                            "DENIED"                            },
			{ ReconfigurationResponseParameter::Result::ERROR_WRONG_SSN,                   "ERROR_WRONG_SSN"                   },
			{ ReconfigurationResponseParameter::Result::ERROR_REQUEST_ALREADY_IN_PROGRESS, "ERROR_REQUEST_ALREADY_IN_PROGRESS" },
			{ ReconfigurationResponseParameter::Result::ERROR_BAD_SEQUENCE_NUMBER,         "ERROR_BAD_SEQUENCE_NUMBER"         },
			{ ReconfigurationResponseParameter::Result::IN_PROGRESS,                       "IN_PROGRESS"                       },
		};
		// clang-format on

		/* Class methods. */

		ReconfigurationResponseParameter* ReconfigurationResponseParameter::Parse(
		  const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Parameter::ParameterType parameterType;
			uint16_t parameterLength;
			uint8_t padding;

			if (!Parameter::IsParameter(buffer, bufferLength, parameterType, parameterLength, padding))
			{
				return nullptr;
			}

			if (parameterType != Parameter::ParameterType::RECONFIGURATION_RESPONSE)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return ReconfigurationResponseParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		ReconfigurationResponseParameter* ReconfigurationResponseParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new ReconfigurationResponseParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::RECONFIGURATION_RESPONSE,
			  ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLength);

			// Must also initialize extra fields in the header.
			parameter->SetReconfigurationResponseSequenceNumber(0);
			parameter->SetResult(static_cast<Result>(0));

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		const std::string& ReconfigurationResponseParameter::Result2String(
		  ReconfigurationResponseParameter::Result result)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = ReconfigurationResponseParameter::result2String.find(result);

			if (it == ReconfigurationResponseParameter::result2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		ReconfigurationResponseParameter* ReconfigurationResponseParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (
			  parameterLength !=
			    ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLength &&
			  parameterLength !=
			    ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLengthWithOptionalFields)
			{
				MS_WARN_TAG(
				  sctp,
				  "ReconfigurationResponseParameter Length field must be %zu or %zu",
				  ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLength,
				  ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLengthWithOptionalFields);

				return nullptr;
			}

			auto* parameter =
			  new ReconfigurationResponseParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			parameter->SetLength(parameterLength + padding);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		/* Instance methods. */

		ReconfigurationResponseParameter::ReconfigurationResponseParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLength);
		}

		ReconfigurationResponseParameter::~ReconfigurationResponseParameter()
		{
			MS_TRACE();
		}

		void ReconfigurationResponseParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ReconfigurationResponseParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  re-configuration response sequence number: %" PRIu32,
			  GetReconfigurationResponseSequenceNumber());
			MS_DUMP_CLEAN(
			  indentation,
			  "  result: %" PRIu32 " (%s)",
			  static_cast<uint32_t>(GetResult()),
			  ReconfigurationResponseParameter::Result2String(GetResult()).c_str());
			MS_DUMP_CLEAN(indentation, "  has next tsns: %s", HasNextTsns() ? "yes" : "no");
			if (HasNextTsns())
			{
				MS_DUMP_CLEAN(indentation, "  sender next tsn: %" PRIu32, GetSenderNextTsn());
				MS_DUMP_CLEAN(indentation, "  receiver next tsn: %" PRIu32, GetReceiverNextTsn());
			}
			MS_DUMP_CLEAN(indentation, "</SCTP::ReconfigurationResponseParameter>");
		}

		ReconfigurationResponseParameter* ReconfigurationResponseParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new ReconfigurationResponseParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void ReconfigurationResponseParameter::SetReconfigurationResponseSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void ReconfigurationResponseParameter::SetResult(Result result)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 8, static_cast<uint32_t>(result));
		}

		void ReconfigurationResponseParameter::SetNextTsns(uint32_t senderNextTsn, uint32_t receiverNextTsn)
		{
			MS_TRACE();

			AssertNotFrozen();

			if (!HasNextTsns())
			{
				// This may throw.
				SetVariableLengthValueLength(
				  ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLengthWithOptionalFields -
				  ReconfigurationResponseParameter::ReconfigurationResponseParameterHeaderLength);
			}

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 12, senderNextTsn);
			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 16, receiverNextTsn);
		}

		ReconfigurationResponseParameter* ReconfigurationResponseParameter::SoftClone(
		  const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new ReconfigurationResponseParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
