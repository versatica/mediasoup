#ifndef MS_RTC_RTP_PROBATION_GENERATOR_HPP
#define MS_RTC_RTP_PROBATION_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	// SSRC of the probation RTP stream.
	constexpr uint32_t RtpProbationSsrc{ 1234u };
	// Codec payload type of the probation RTP stream.
	constexpr uint8_t RtpProbationCodecPayloadType{ 127u };

	class RtpProbationGenerator
	{
	public:
		explicit RtpProbationGenerator();
		~RtpProbationGenerator();

	public:
		RTC::RTP::Packet* GetNextPacket(size_t len);

	private:
		// Allocated by this.
		uint8_t* probationPacketBuffer{ nullptr };
		RTC::RTP::Packet* probationPacket{ nullptr };
	}; // namespace RTC

} // namespace RTC

#endif
