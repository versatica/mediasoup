#define MS_CLASS "fuzzer"

#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#include "DepLibWebRTC.hpp"
#include "DepOpenSSL.hpp"
#include "DepUsrSCTP.hpp"
#include "FuzzerUtils.hpp"
#include "LogLevel.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "RTC/FuzzerRtpPacket.hpp"
#include "RTC/FuzzerStunPacket.hpp"
#include "RTC/FuzzerTrendCalculator.hpp"
#include "RTC/RTCP/FuzzerPacket.hpp"
#include <cstdlib> // std::getenv()
#include <iostream>
#include <stddef.h>
#include <stdint.h>

bool fuzzStun  = false;
bool fuzzRtp   = false;
bool fuzzRtcp  = false;
bool fuzzUtils = false;

int Init();

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	// Trick to initialize our stuff just once.
	static int unused = Init();

	// Avoid [-Wunused-variable].
	unused++;

	if (fuzzStun)
		Fuzzer::RTC::StunPacket::Fuzz(data, len);

	if (fuzzRtp)
		Fuzzer::RTC::RtpPacket::Fuzz(data, len);

	if (fuzzRtcp)
		Fuzzer::RTC::RTCP::Packet::Fuzz(data, len);

	if (fuzzUtils)
	{
		Fuzzer::Utils::Fuzz(data, len);
		Fuzzer::RTC::TrendCalculator::Fuzz(data, len);
	}

	return 0;
}

int Init()
{
	LogLevel logLevel{ LogLevel::LOG_NONE };

	// Get logLevel from ENV variable.
	if (std::getenv("MS_FUZZ_LOG_LEVEL"))
	{
		if (std::string(std::getenv("MS_FUZZ_LOG_LEVEL")) == "debug")
			logLevel = LogLevel::LOG_DEBUG;
		else if (std::string(std::getenv("MS_FUZZ_LOG_LEVEL")) == "warn")
			logLevel = LogLevel::LOG_WARN;
		else if (std::string(std::getenv("MS_FUZZ_LOG_LEVEL")) == "error")
			logLevel = LogLevel::LOG_ERROR;
	}

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
	if (std::getenv("MS_FUZZ_UTILS") && std::string(std::getenv("MS_FUZZ_UTILS")) == "1")
	{
		std::cout << "[fuzzer] Utils fuzzers enabled" << std::endl;

		fuzzUtils = true;
	}
	if (!fuzzUtils && !fuzzStun && !fuzzRtcp && !fuzzRtp)
	{
		std::cout << "[fuzzer] all fuzzers enabled" << std::endl;

		fuzzStun  = true;
		fuzzRtp   = true;
		fuzzRtcp  = true;
		fuzzUtils = true;
	}

	Settings::configuration.logLevel = logLevel;

	// Initialize static stuff.
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	DepLibSRTP::ClassInit();
	DepUsrSCTP::ClassInit();
	DepLibWebRTC::ClassInit();
	Utils::Crypto::ClassInit();

	return 0;
}
