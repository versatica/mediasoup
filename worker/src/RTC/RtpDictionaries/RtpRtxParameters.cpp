#define MS_CLASS "RTC::RtpRtxParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpRtxParameters::RtpRtxParameters(const FBS::RtpParameters::Rtx* data)
	{
		MS_TRACE();

		// ssrc is optional.
		if (data->ssrc().has_value())
			this->ssrc = data->ssrc().value();
	}

	flatbuffers::Offset<FBS::RtpParameters::Rtx> RtpRtxParameters::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		return FBS::RtpParameters::CreateRtx(
		  builder, this->ssrc != 0u ? flatbuffers::Optional<uint32_t>(this->ssrc) : flatbuffers::nullopt);
	}
} // namespace RTC
