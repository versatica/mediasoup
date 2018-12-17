#include "fuzzers.hpp"
#include "RTC/RTCP/Packet.hpp"

using namespace RTC;

namespace fuzzers
{
	void fuzzRtp(const uint8_t* data, size_t len)
	{
		if (!RTCP::Packet::IsRtcp(data, len))
			return;

		RTCP::Packet* packet = RTCP::Packet::Parse(data, len);

		if (!packet)
			return;

		// TODO: Add more checks here.

		while (packet != nullptr)
		{
			auto* previousPacket = packet;

			packet = packet->GetNext();
			delete previousPacket;
		}
	}
}
