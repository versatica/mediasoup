#define MS_CLASS "RTC::RtcpParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <limits> // std::numeric_limits()

namespace RTC
{
	/* Instance methods. */

	RtcpParameters::RtcpParameters(const FBS::RtpParameters::RtcpParameters* data)
	{
		MS_TRACE();

		// cname is mandatory. It is what identifies the streams that come from a same
		// sender, and hence share its clock, so media of a sender that comes with no CNAME
		// cannot be synchronized against the rest of the media of that same sender.
		if (!flatbuffers::IsFieldPresent(data, FBS::RtpParameters::RtcpParameters::VT_CNAME))
		{
			MS_THROW_TYPE_ERROR("missing cname");
		}

		this->cname = data->cname()->str();

		if (this->cname.empty())
		{
			MS_THROW_TYPE_ERROR("empty cname");
		}

		// The cname is sent in a SDES item, whose length field takes 8 bits.
		if (this->cname.size() > std::numeric_limits<uint8_t>::max())
		{
			MS_THROW_TYPE_ERROR(
			  "cname too long [len:%zu, max len:%zu]",
			  this->cname.size(),
			  static_cast<size_t>(std::numeric_limits<uint8_t>::max()));
		}

		// reducedSize is optional, default value is true.
		this->reducedSize = data->reducedSize();
	}

	flatbuffers::Offset<FBS::RtpParameters::RtcpParameters> RtcpParameters::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		return FBS::RtpParameters::CreateRtcpParametersDirect(
		  builder, this->cname.c_str(), this->reducedSize);
	}
} // namespace RTC
