#ifndef MS_RTC_RTP_PROBATION_GENERATOR_HPP
#define MS_RTC_RTP_PROBATION_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	class RtpProbationGenerator
	{
	public:
		// SSRC of the probation RTP stream.
		static const uint32_t Ssrc{ 1234 };
		// Codec payload type of the probation RTP stream.
		static const uint8_t PayloadType{ 127u };

	public:
		explicit RtpProbationGenerator();
		~RtpProbationGenerator();

	public:
		RTC::RTP::Packet* GetNextPacket(size_t len);

	private:
		// Allocated by this.
		RTC::RTP::Packet* probationPacket{ nullptr };
		// Others.
		// The length of the probation RTP Packet without payload or padding.
		size_t probationPacketMinLength{ 0 };
	}; // namespace RTC

} // namespace RTC

#endif
