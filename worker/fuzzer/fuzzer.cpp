#define MS_CLASS "fuzzer"

#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "LogLevel.hpp"
#include "Logger.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "fuzzers.hpp"
#include <stdint.h>
#include <stddef.h>
#include <cstdlib> // std::getenv()
#include <iostream>

bool fuzzStun = false;
bool fuzzRtcp = false;
bool fuzzRtp = false;

int init()
{
	std::string loggerId = "fuzzer";

	Settings::configuration.logLevel = LogLevel::LOG_DEBUG;
	Logger::Init(loggerId);
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	Utils::Crypto::ClassInit();

	// Select what to fuzz.
	if (std::getenv("MS_FUZZ_STUN") && std::string(std::getenv("MS_FUZZ_STUN")) == "1")
	{
		std::cout << "[fuzzer] STUN checks enabled" << std::endl;

		fuzzStun = true;
	}
	if (std::getenv("MS_FUZZ_RTCP") && std::string(std::getenv("MS_FUZZ_RTCP")) == "1")
	{
		std::cout << "[fuzzer] RTCP checks enabled" << std::endl;

		fuzzRtcp = true;
	}
	if (std::getenv("MS_FUZZ_RTP") && std::string(std::getenv("MS_FUZZ_RTP")) == "1")
	{
		std::cout << "[fuzzer] RTP checks enabled" << std::endl;

		fuzzRtp = true;
	}
	if (!fuzzStun && !fuzzRtcp && !fuzzRtp)
	{
		std::cout << "[fuzzer] all checks enabled" << std::endl;

		fuzzStun = true;
		fuzzRtcp = true;
		fuzzRtp = true;
	}

	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	// Trick to initialize our stuff just once.
	static int unused = init();

	if (fuzzStun)
		fuzzers::fuzzStun(data, len);

	if (fuzzRtp)
		fuzzers::fuzzRtp(data, len);

	if (fuzzRtcp)
		fuzzers::fuzzRtcp(data, len);

	return 0;
}
