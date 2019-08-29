#ifndef MS_RTC_RTP_PROBATION_GENERATOR_HPP
#define MS_RTC_RTP_PROBATION_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	class RtpProbationGenerator
	{
	public:
		explicit RtpProbationGenerator(size_t probationPacketLen);
		virtual ~RtpProbationGenerator();

	public:
		RTC::RtpPacket* GetNextPacket();

	private:
		// Given as argument.
		size_t probationPacketLen{ 0 };
		// Allocated by this.
		uint8_t* probationPacketBuffer{ nullptr };
		RTC::RtpPacket* probationPacket{ nullptr };
	}; // namespace RTC

} // namespace RTC

#endif
