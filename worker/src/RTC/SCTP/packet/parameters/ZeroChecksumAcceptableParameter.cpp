#define MS_CLASS "RTC::SCTP::ZeroChecksumAcceptableParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		std::unordered_map<ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod, std::string> ZeroChecksumAcceptableParameter::alternateErrorDetectionMethod2String =
		{
			{ ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE,           "NONE"           },
			{ ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS, "SCTP_OVER_DTLS" },
		};
		// clang-format on

		/* Class methods. */

		ZeroChecksumAcceptableParameter* ZeroChecksumAcceptableParameter::Parse(
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

			if (parameterType != Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE)
			{
				MS_WARN_DEV("invalid Parameter type");

				return nullptr;
			}

			return ZeroChecksumAcceptableParameter::ParseStrict(
			  buffer, bufferLength, parameterLength, padding);
		}

		ZeroChecksumAcceptableParameter* ZeroChecksumAcceptableParameter::Factory(
		  uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < ZeroChecksumAcceptableParameter::ZeroChecksumAcceptableParameterHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* parameter = new ZeroChecksumAcceptableParameter(buffer, bufferLength);

			parameter->InitializeHeader(
			  Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE,
			  ZeroChecksumAcceptableParameter::ZeroChecksumAcceptableParameterHeaderLength);

			// Initialize Alternate Error Detection Method (EDMID) to zero (none).
			parameter->SetAlternateErrorDetectionMethod(AlternateErrorDetectionMethod::NONE);

			// No need to invoke SetLength() since parent constructor invoked it.

			return parameter;
		}

		ZeroChecksumAcceptableParameter* ZeroChecksumAcceptableParameter::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t parameterLength, uint8_t padding)
		{
			MS_TRACE();

			if (parameterLength != ZeroChecksumAcceptableParameter::ZeroChecksumAcceptableParameterHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "ZeroChecksumAcceptableParameter Length field must be %zu",
				  ZeroChecksumAcceptableParameter::ZeroChecksumAcceptableParameterHeaderLength);

				return nullptr;
			}

			auto* parameter =
			  new ZeroChecksumAcceptableParameter(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the Parameter as frozen since we are parsing.
			parameter->Freeze();

			return parameter;
		}

		const std::string& ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod2String(
		  AlternateErrorDetectionMethod alternateErrorDetectionMethod)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = ZeroChecksumAcceptableParameter::alternateErrorDetectionMethod2String.find(
			  alternateErrorDetectionMethod);

			if (it == ZeroChecksumAcceptableParameter::alternateErrorDetectionMethod2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		ZeroChecksumAcceptableParameter::ZeroChecksumAcceptableParameter(uint8_t* buffer, size_t bufferLength)
		  : Parameter(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(ZeroChecksumAcceptableParameter::ZeroChecksumAcceptableParameterHeaderLength);
		}

		ZeroChecksumAcceptableParameter::~ZeroChecksumAcceptableParameter()
		{
			MS_TRACE();
		}

		void ZeroChecksumAcceptableParameter::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::ZeroChecksumAcceptableParameter>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  alternate error detection method: %" PRIu32 " (%s)",
			  static_cast<uint32_t>(GetAlternateErrorDetectionMethod()),
			  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod2String(
			    GetAlternateErrorDetectionMethod())
			    .c_str());
			MS_DUMP_CLEAN(indentation, "</SCTP::ZeroChecksumAcceptableParameter>");
		}

		ZeroChecksumAcceptableParameter* ZeroChecksumAcceptableParameter::Clone(
		  uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedParameter = new ZeroChecksumAcceptableParameter(buffer, bufferLength);

			CloneInto(clonedParameter);

			return clonedParameter;
		}

		void ZeroChecksumAcceptableParameter::SetAlternateErrorDetectionMethod(
		  AlternateErrorDetectionMethod alternateErrorDetectionMethod)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(
			  const_cast<uint8_t*>(GetBuffer()), 4, static_cast<uint32_t>(alternateErrorDetectionMethod));
		}

		ZeroChecksumAcceptableParameter* ZeroChecksumAcceptableParameter::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedParameter =
			  new ZeroChecksumAcceptableParameter(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedParameter);

			return softClonedParameter;
		}
	} // namespace SCTP
} // namespace RTC
