#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "Utils.hpp"
#include "RTC/RtpPacket.hpp"
#include <stdint.h>
#include <stddef.h>
#include <iostream>

int init()
{
	// Initialize static stuff.
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	Utils::Crypto::ClassInit();

	return 0;
}

void fuzz(const uint8_t* data, size_t len)
{
	// TODO: Test everything here.
	// If a RTP packet clone it, read properties, set stuff, etc.
	// If RTCP the same.
	// If ICE too.

	RTC::RtpPacket::IsRtp(data, len);

	RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

	if (packet)
		delete packet;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	static int unused = init();

	fuzz(data, len);

	return 0;
}

#ifdef MS_FUZZER_FAKE
int main(int argc, char* argv[])
{
	std::cout << "OK, it compiles. Now go to Linux and run `make fuzzer-run." << std::endl;

	return 0;
}
#endif
