#include "RTC/RTCP/FuzzerPacket.hpp"
#include "RTC/RTCP/Packet.hpp"

void Fuzzer::RTC::RTCP::Packet::Fuzz(const uint8_t* data, size_t len)
{
	if (!::RTC::RTCP::Packet::IsRtcp(data, len))
		return;

	::RTC::RTCP::Packet* packet = ::RTC::RTCP::Packet::Parse(data, len);

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
