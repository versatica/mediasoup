#include "RTC/RtpPacket.hpp"
#include <stdint.h>
#include <stddef.h>
#include <iostream>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	RTC::RtpPacket::IsRtp(data, len);

	RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

	if (packet)
	{
		std::cout << "It is a RTP packet" << std::endl;

		delete packet;
	}
	else
	{
		// std::cout << "It is NOT a RTP packet !!!" << std::endl;
		// std::cout << ".";
	}

	return 0;
}
