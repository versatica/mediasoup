#include "RTC/SCTP/FuzzerPacket.hpp"
#include "RTC/SCTP/Packet.hpp"

static constexpr size_t SctpSerializeBufferSize{ 65536 };
thread_local static uint8_t SctpSerializeBuffer[SctpSerializeBufferSize];

void Fuzzer::RTC::SCTP::Packet::Fuzz(const uint8_t* data, size_t len)
{
	if (!::RTC::SCTP::Packet::IsPacket(data, len))
	{
		return;
	}

	::RTC::SCTP::Packet* packet = ::RTC::SCTP::Packet::Parse(data, len);

	if (!packet)
	{
		return;
	}

	packet->Dump();

	delete packet;
}
