#define MS_CLASS "RTC::RtpHeaderExtensionParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpHeaderExtensionParameters::RtpHeaderExtensionParameters(
	  const FBS::RtpParameters::RtpHeaderExtensionParameters* const data)
	  : type(RTC::RtpHeaderExtensionUri::TypeFromFbs(data->uri())),
	    id(data->id()),
	    encrypt(data->encrypt())
	{
		MS_TRACE();

		// Don't allow id 0.
		if (this->id == 0)
		{
			MS_THROW_TYPE_ERROR("invalid id 0");
		}

		// parameters is optional.
		if (flatbuffers::IsFieldPresent(data, FBS::RtpParameters::RtpHeaderExtensionParameters::VT_PARAMETERS))
		{
			this->parameters.Set(data->parameters());
		}
	}

	flatbuffers::Offset<FBS::RtpParameters::RtpHeaderExtensionParameters> RtpHeaderExtensionParameters::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		const auto parameters = this->parameters.FillBuffer(builder);

		return FBS::RtpParameters::CreateRtpHeaderExtensionParametersDirect(
		  builder,
		  RTC::RtpHeaderExtensionUri::TypeToFbs(this->type),
		  this->id,
		  this->encrypt,
		  std::addressof(parameters));
	}
} // namespace RTC
