#define MS_CLASS "RTC::SCTP::Parameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/Parameter.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		const std::unordered_map<Parameter::ParameterType, std::string> Parameter::ParameterType2String =
		{
			{ Parameter::ParameterType::HEARTBEAT_INFO,               "HEARTBEAT-INFO"               },
			{ Parameter::ParameterType::IPV4_ADDRESS,                 "IPV4-ADDRESS"                 },
			{ Parameter::ParameterType::IPV6_ADDRESS,                 "IPV6-ADDRESS"                 },
			{ Parameter::ParameterType::STATE_COOKIE,                 "STATE-COOKIE"                 },
			{ Parameter::ParameterType::UNRECOGNIZED_PARAMETER,       "UNRECOGNIZED-PARAMETER"       },
			{ Parameter::ParameterType::COOKIE_PRESERVATIVE,          "COOKIE-PRESERVATIVE"          },
			{ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,      "SUPPORTED-ADDRESS-TYPES"      },
			{ Parameter::ParameterType::FORWARD_TSN_SUPPORTED,        "FORWARD-TSN-SUPPORTED"        },
			{ Parameter::ParameterType::SUPPORTED_EXTENSIONS,         "SUPPORTED-EXTENSIONS"         },
			{ Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,   "OUTGOING-SSN-RESET-REQUEST"   },
			{ Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,   "INCOMING-SSN-RESET-REQUEST"   },
			{ Parameter::ParameterType::SSN_TSN_RESET_REQUEST,        "SSN-TSN-RESET-REQUEST"        },
			{ Parameter::ParameterType::RECONFIGURATION_RESPONSE,     "RECONFIGURATION-RESPONSE"     },
			{ Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST, "ADD-OUTGOING-STREAMS-REQUEST" },
			{ Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST, "ADD-INCOMING-STREAMS-REQUEST" },
			{ Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,     "ZERO-CHECKSUM-ACCEPTABLE"     },
		};
		// clang-format on

		/* Class methods. */

		bool Parameter::IsParameter(
		  const uint8_t* buffer,
		  size_t bufferLength,
		  Parameter::ParameterType& parameterType,
		  uint16_t& parameterLength,
		  uint8_t& padding)
		{
			MS_TRACE();

			if (!TLV::IsTLV(buffer, bufferLength, parameterLength, padding))
			{
				return false;
			}

			parameterType = static_cast<Parameter::ParameterType>(Utils::Byte::Get2Bytes(buffer, 0));

			return true;
		}

		const std::string& Parameter::ParameterTypeToString(ParameterType parameterType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = Parameter::ParameterType2String.find(parameterType);

			if (it == Parameter::ParameterType2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		Parameter::Parameter(uint8_t* buffer, size_t bufferLength) : TLV(buffer, bufferLength)
		{
			MS_TRACE();
		}

		Parameter::~Parameter()
		{
			MS_TRACE();
		}

		void Parameter::SoftCloneInto(Parameter* parameter) const
		{
			MS_TRACE();

			// Need to manually set Serializable length.
			parameter->SetLength(GetLength());
		}

		void Parameter::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation,
			  "  type: %" PRIu16 " (%s) (unknown: %s)",
			  static_cast<uint16_t>(GetType()),
			  Parameter::ParameterTypeToString(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			TLV::DumpCommon(indentation);
		}

		void Parameter::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			SetBuffer(const_cast<uint8_t*>(buffer));
		}

		void Parameter::InitializeHeader(ParameterType parameterType, uint16_t lengthFieldValue)
		{
			MS_TRACE();

			SetType(parameterType);
			InitializeTLVHeader(lengthFieldValue);
		}
	} // namespace SCTP
} // namespace RTC
