#define MS_CLASS "RTC::SctpStreamParameters"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SctpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	SctpStreamParameters::SctpStreamParameters(const FBS::SctpParameters::SctpStreamParameters* data)
	{
		MS_TRACE();

		this->streamId = data->streamId();

		if (this->streamId > 65534)
		{
			MS_THROW_TYPE_ERROR("streamId must not be greater than 65534");
		}

		// ordered is optional.
		bool orderedGiven = false;

		if (auto ordered = data->ordered(); ordered.has_value())
		{
			orderedGiven  = true;
			this->ordered = ordered.value();
		}

		// maxPacketLifeTime is optional.
		if (auto maxPacketLifeTime = data->maxPacketLifeTime(); maxPacketLifeTime.has_value())
		{
			this->maxPacketLifeTime = maxPacketLifeTime.value();
		}

		// maxRetransmits is optional.
		if (auto maxRetransmits = data->maxRetransmits(); maxRetransmits.has_value())
		{
			this->maxRetransmits = maxRetransmits.value();
		}

		if (this->maxPacketLifeTime && this->maxRetransmits)
		{
			MS_THROW_TYPE_ERROR("cannot provide both maxPacketLifeTime and maxRetransmits");
		}

		// clang-format off
		if (
			orderedGiven &&
			this->ordered &&
			(this->maxPacketLifeTime || this->maxRetransmits)
		)
		// clang-format on
		{
			MS_THROW_TYPE_ERROR("cannot be ordered with maxPacketLifeTime or maxRetransmits");
		}
		else if (!orderedGiven && (this->maxPacketLifeTime || this->maxRetransmits))
		{
			this->ordered = false;
		}
	}

	flatbuffers::Offset<FBS::SctpParameters::SctpStreamParameters> SctpStreamParameters::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		return FBS::SctpParameters::CreateSctpStreamParameters(
		  builder,
		  this->streamId,
		  this->ordered,
		  this->maxPacketLifeTime ? flatbuffers::Optional<uint16_t>(this->maxPacketLifeTime)
		                          : flatbuffers::nullopt,
		  this->maxRetransmits ? flatbuffers::Optional<uint16_t>(this->maxRetransmits)
		                       : flatbuffers::nullopt);
	}
} // namespace RTC
