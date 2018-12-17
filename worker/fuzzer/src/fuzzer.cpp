#define MS_CLASS "fuzzer"

#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "LogLevel.hpp"
#include "Logger.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "RTC/FuzzerStunMessage.hpp"
#include "RTC/FuzzerRtpPacket.hpp"
#include "RTC/RTCP/FuzzerPacket.hpp"
#include <stdint.h>
#include <stddef.h>
#include <cstdlib> // std::getenv()
#include <iostream>

bool fuzzStun = false;
bool fuzzRtp = false;
bool fuzzRtcp = false;

int init()
{
	std::string loggerId = "fuzzer";

	Settings::configuration.logLevel = LogLevel::LOG_NONE;
	Logger::Init(loggerId);
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	Utils::Crypto::ClassInit();

	// Select what to fuzz.
	if (std::getenv("MS_FUZZ_STUN") && std::string(std::getenv("MS_FUZZ_STUN")) == "1")
	{
		std::cout << "[fuzzer] STUN fuzzers enabled" << std::endl;

		fuzzStun = true;
	}
	if (std::getenv("MS_FUZZ_RTP") && std::string(std::getenv("MS_FUZZ_RTP")) == "1")
	{
		std::cout << "[fuzzer] RTP fuzzers enabled" << std::endl;

		fuzzRtp = true;
	}
	if (std::getenv("MS_FUZZ_RTCP") && std::string(std::getenv("MS_FUZZ_RTCP")) == "1")
	{
		std::cout << "[fuzzer] RTCP fuzzers enabled" << std::endl;

		fuzzRtcp = true;
	}
	if (!fuzzStun && !fuzzRtcp && !fuzzRtp)
	{
		std::cout << "[fuzzer] all fuzzers enabled" << std::endl;

		fuzzStun = true;
		fuzzRtp = true;
		fuzzRtcp = true;
	}

	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	// Trick to initialize our stuff just once.
	static int unused = init();

	if (fuzzStun)
		Fuzzer::RTC::StunMessage::Fuzz(data, len);

	if (fuzzRtp)
		Fuzzer::RTC::RtpPacket::Fuzz(data, len);

	if (fuzzRtcp)
		Fuzzer::RTC::RTCP::Packet::Fuzz(data, len);

	return 0;
}
