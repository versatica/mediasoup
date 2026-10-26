#ifndef MS_RTC_SCTP_DICTIONARIES_HPP
#define MS_RTC_SCTP_DICTIONARIES_HPP

#include "common.hpp"
#include <FBS/sctpParameters.h>

namespace RTC
{
	class SctpStreamParameters
	{
	public:
		SctpStreamParameters() = default;
		explicit SctpStreamParameters(const FBS::SctpParameters::SctpStreamParameters* data);

		flatbuffers::Offset<FBS::SctpParameters::SctpStreamParameters> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const;

	public:
		uint16_t streamId{ 0 };
		bool ordered{ true };
		std::optional<uint16_t> maxPacketLifeTime;
		std::optional<uint16_t> maxRetransmits;
	};
} // namespace RTC

#endif
