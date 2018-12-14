#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "Utils.hpp"
#include "RTC/StunMessage.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RTCP/Packet.hpp"
#include <stdint.h>
#include <stddef.h>
#include <iostream>

using namespace RTC;

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
	//
	// NOTE: This code could be split in different files under a new src/ folder
	// with its corresponding new include/ folder.

	// Check if it's STUN.
	if (StunMessage::IsStun(data, len))
	{
		StunMessage* msg = StunMessage::Parse(data, len);

		if (msg)
			delete msg;
	}
	// Check if it's RTCP.
	else if (RTCP::Packet::IsRtcp(data, len))
	{
		RTCP::Packet* packet = RTCP::Packet::Parse(data, len);

		if (packet)
			delete packet;
	}
	// Check if it's RTP.
	else if (RtpPacket::IsRtp(data, len))
	{
		RtpPacket* packet = RtpPacket::Parse(data, len);

		if (packet)
			delete packet;
	}
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
