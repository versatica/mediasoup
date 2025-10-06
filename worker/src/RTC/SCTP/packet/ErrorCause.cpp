#define MS_CLASS "RTC::SCTP::ErrorCause"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		std::unordered_map<ErrorCause::ErrorCauseCode, std::string> ErrorCause::errorCauseCode2String =
		{
			{ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,                    "INVALID_STREAM_IDENTIFIER"                    },
			{ ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER,                  "MISSING_MANDATORY_PARAMETER"                  },
			{ ErrorCause::ErrorCauseCode::STALE_COOKIE,                                 "STALE_COOKIE"                                 },
			{ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,                              "OUT_OF_RESOURCE"                              },
			{ ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS,                         "UNRESOLVABLE_ADDRESS"                         },
			{ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,                      "UNRECOGNIZED_CHUNK_TYPE"                      },
			{ ErrorCause::ErrorCauseCode::INVALID_MANDATORY_PARAMETER,                  "INVALID_MANDATORY_PARAMETER"                  },
			{ ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,                      "UNRECOGNIZED_PARAMETERS"                      },
			{ ErrorCause::ErrorCauseCode::NO_USER_DATA,                                 "NO_USER_DATA"                                 },
			{ ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,          "COOKIE_RECEIVED_WHILE_SHUTTING_DOWN"          },
			{ ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES, "RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES" },
			{ ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,                         "USER_INITIATED_ABORT"                         },
			{ ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,                           "PROTOCOL_VIOLATION"                           },
		};
		// clang-format on

		/* Class methods. */

		bool ErrorCause::IsErrorCause(
		  const uint8_t* buffer,
		  size_t bufferLength,
		  ErrorCause::ErrorCauseCode& causeCode,
		  uint16_t& causeLength,
		  uint8_t& padding)
		{
			MS_TRACE();

			if (!TLV::IsTLV(buffer, bufferLength, causeLength, padding))
			{
				return false;
			}

			causeCode = static_cast<ErrorCause::ErrorCauseCode>(Utils::Byte::Get2Bytes(buffer, 0));

			return true;
		}

		const std::string& ErrorCause::ErrorCauseCode2String(ErrorCauseCode causeCode)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = ErrorCause::errorCauseCode2String.find(causeCode);

			if (it == ErrorCause::errorCauseCode2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		ErrorCause::ErrorCause(uint8_t* buffer, size_t bufferLength) : TLV(buffer, bufferLength)
		{
			MS_TRACE();
		}

		ErrorCause::~ErrorCause()
		{
			MS_TRACE();
		}

		void ErrorCause::SoftCloneInto(ErrorCause* errorCause) const
		{
			MS_TRACE();

			// Need to manually set Serializable length.
			errorCause->SetLength(GetLength());
		}

		void ErrorCause::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation,
			  "  code: %" PRIu16 " (%s) (unknown: %s)",
			  static_cast<uint16_t>(GetCode()),
			  ErrorCause::ErrorCauseCode2String(GetCode()).c_str(),
			  HasUnknownCode() ? "yes" : "no");
			TLV::DumpCommon(indentation);
		}

		void ErrorCause::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			SetBuffer(const_cast<uint8_t*>(buffer));
		}

		void ErrorCause::InitializeHeader(ErrorCauseCode causeCode, uint16_t lengthFieldValue)
		{
			MS_TRACE();

			SetCode(causeCode);
			InitializeTLVHeader(lengthFieldValue);
		}
	} // namespace SCTP
} // namespace RTC
