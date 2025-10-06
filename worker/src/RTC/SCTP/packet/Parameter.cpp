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
		std::unordered_map<Parameter::ParameterType, std::string> Parameter::parameterType2String =
		{
			{ Parameter::ParameterType::HEARTBEAT_INFO,               "HEARTBEAT_INFO"               },
			{ Parameter::ParameterType::IPV4_ADDRESS,                 "IPV4_ADDRESS"                 },
			{ Parameter::ParameterType::IPV6_ADDRESS,                 "IPV6_ADDRESS"                 },
			{ Parameter::ParameterType::STATE_COOKIE,                 "STATE_COOKIE"                 },
			{ Parameter::ParameterType::UNRECOGNIZED_PARAMETER,       "UNRECOGNIZED_PARAMETER"       },
			{ Parameter::ParameterType::COOKIE_PRESERVATIVE,          "COOKIE_PRESERVATIVE"          },
			{ Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES,      "SUPPORTED_ADDRESS_TYPES"      },
			{ Parameter::ParameterType::FORWARD_TSN_SUPPORTED,        "FORWARD_TSN_SUPPORTED"        },
			{ Parameter::ParameterType::SUPPORTED_EXTENSIONS,         "SUPPORTED_EXTENSIONS"         },
			{ Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST,   "OUTGOING_SSN_RESET_REQUEST"   },
			{ Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST,   "INCOMING_SSN_RESET_REQUEST"   },
			{ Parameter::ParameterType::SSN_TSN_RESET_REQUEST,        "SSN_TSN_RESET_REQUEST"        },
			{ Parameter::ParameterType::RECONFIGURATION_RESPONSE,     "RECONFIGURATION_RESPONSE"     },
			{ Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST, "ADD_OUTGOING_STREAMS_REQUEST" },
			{ Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST, "ADD_INCOMING_STREAMS_REQUEST" },
			{ Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,     "ZERO_CHECKSUM_ACCEPTABLE"     },
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

		const std::string& Parameter::ParameterType2String(ParameterType parameterType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = Parameter::parameterType2String.find(parameterType);

			if (it == Parameter::parameterType2String.end())
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
			  Parameter::ParameterType2String(GetType()).c_str(),
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
