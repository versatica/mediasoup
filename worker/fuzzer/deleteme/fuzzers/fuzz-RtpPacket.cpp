// clang++ -std=c++11 -D MS_LITTLE_ENDIAN -g -fsanitize=address,fuzzer -I include fuzzers/fuzz-RtpPacket.cpp src/RTC/RtpPacket.cpp && ./a.out

#include "RTC/RtpPacket.hpp"
#include <stdint.h>
#include <stddef.h>
#include <iostream>



extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	// RTC::RtpPacket::IsRtp(data, len);
	RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

	if (packet)
	{
		// std::cout << "It is a RTP packet" << std::endl;
		// std::cout << "o";

		// RTC::RtpPacket* packet2 = RTC::RtpPacket::CreateProbationPacket(data, len);
		// if (packet2)
		// {
		// 	std::cout << "packet2 probation created!" << std::endl;
		// 	delete packet2;
		// }

		delete packet;
	}
	else
	{
		// std::cout << "It is NOT a RTP packet !!!" << std::endl;
		// std::cout << ".";
	}

	return 0;
}




// int main(int argc, char* argv[])
// {
// 	uint8_t data[] =
// 	{
// 		0b10010000, 0b00000001, 0, 8,
// 		0, 0, 0, 4,
// 		0, 0, 0, 5,
// 		0xBE, 0xDE, 0, 3, // Extension header
// 		0b00010000, 0xFF, 0b00100001, 0xFF,
// 		0xFF, 0, 0, 0b00110011,
// 		0xFF, 0xFF, 0xFF, 0xFF
// 	};

// 	bool ret = RTC::RtpPacket::IsRtp(data, sizeof(data));

// 	if (ret)
// 	{
// 		std::cout << "It is a RTP packet" << std::endl;
// 		return 0;
// 	}
// 	else
// 	{
// 		std::cout << "It is NOT a RTP packet !!!" << std::endl;
// 		return 123;
// 	}
// }
