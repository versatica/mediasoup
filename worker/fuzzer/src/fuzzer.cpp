#define MS_CLASS "fuzzer"

#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#include "DepLibWebRTC.hpp"
#include "DepOpenSSL.hpp"
#include "DepUsrSCTP.hpp"
#include "FuzzerUtils.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "RTC/Codecs/FuzzerAV1.hpp"
#include "RTC/Codecs/FuzzerDependencyDescriptor.hpp"
#include "RTC/Codecs/FuzzerH264.hpp"
#include "RTC/Codecs/FuzzerOpus.hpp"
#include "RTC/Codecs/FuzzerVP8.hpp"
#include "RTC/Codecs/FuzzerVP9.hpp"
#include "RTC/DtlsTransport.hpp"
#include "RTC/FuzzerDtlsTransport.hpp"
#include "RTC/FuzzerRateCalculator.hpp"
#include "RTC/FuzzerRtpPacket.hpp"
#include "RTC/FuzzerRtpRetransmissionBuffer.hpp"
#include "RTC/FuzzerRtpStreamSend.hpp"
#include "RTC/FuzzerSeqManager.hpp"
#include "RTC/FuzzerStunPacket.hpp"
#include "RTC/FuzzerTrendCalculator.hpp"
#include "RTC/RTCP/FuzzerPacket.hpp"
#include "RTC/SCTP/association/FuzzerStateCookie.hpp"
#include "RTC/SCTP/packet/FuzzerPacket.hpp"
#include <cstdlib> // std::getenv()
#include <iostream>
#include <sstream> // std::istringstream()
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

bool fuzzStun   = false;
bool fuzzDtls   = false;
bool fuzzSctp   = false;
bool fuzzRtp    = false;
bool fuzzRtcp   = false;
bool fuzzCodecs = false;
bool fuzzUtils  = false;

// This Init() function must be declared static, otherwise linking will fail if
// another source file defines same non static Init() function.
static int Init();

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	// Trick to initialize our stuff just once.
	static int unused = Init();

	// Avoid [-Wunused-variable].
	(void)unused;

	if (fuzzStun)
	{
		Fuzzer::RTC::StunPacket::Fuzz(data, len);
	}

	if (fuzzDtls)
	{
		Fuzzer::RTC::DtlsTransport::Fuzz(data, len);
	}

	if (fuzzSctp)
	{
		Fuzzer::RTC::SCTP::Packet::Fuzz(data, len);
		Fuzzer::RTC::SCTP::StateCookie::Fuzz(data, len);
	}

	if (fuzzRtp)
	{
		Fuzzer::RTC::RtpPacket::Fuzz(data, len);
		Fuzzer::RTC::RtpStreamSend::Fuzz(data, len);
		Fuzzer::RTC::RtpRetransmissionBuffer::Fuzz(data, len);
		Fuzzer::RTC::SeqManager::Fuzz(data, len);
		Fuzzer::RTC::RateCalculator::Fuzz(data, len);
	}

	if (fuzzRtcp)
	{
		Fuzzer::RTC::RTCP::Packet::Fuzz(data, len);
	}

	if (fuzzCodecs)
	{
		Fuzzer::RTC::Codecs::Opus::Fuzz(data, len);
		Fuzzer::RTC::Codecs::VP8::Fuzz(data, len);
		Fuzzer::RTC::Codecs::VP9::Fuzz(data, len);
		Fuzzer::RTC::Codecs::H264::Fuzz(data, len);
		Fuzzer::RTC::Codecs::AV1::Fuzz(data, len);
		Fuzzer::RTC::Codecs::DependencyDescriptor::Fuzz(data, len);
	}

	if (fuzzUtils)
	{
		Fuzzer::Utils::Fuzz(data, len);
		Fuzzer::RTC::TrendCalculator::Fuzz(data, len);
	}

	return 0;
}

int Init()
{
	std::string logLevel{ "none" };
	std::vector<std::string> logTags = { "info" };

	const auto* logLevelPtr = std::getenv("MS_FUZZ_LOG_LEVEL");
	const auto* logTagsPtr  = std::getenv("MS_FUZZ_LOG_TAGS");

	// Get logLevel from ENV variable.
	if (logLevelPtr)
	{
		logLevel = std::string(logLevelPtr);
	}

	// Get logTags from ENV variable.
	if (logTagsPtr)
	{
		auto logTagsStr = std::string(logTagsPtr);
		std::istringstream iss(logTagsStr);
		std::string logTag;

		while (iss >> logTag)
		{
			logTags.push_back(logTag);
		}
	}

	Settings::SetLogLevel(logLevel);
	Settings::SetLogTags(logTags);
	Settings::PrintConfiguration();

	// Select what to fuzz.

	if (std::getenv("MS_FUZZ_STUN") && std::string(std::getenv("MS_FUZZ_STUN")) == "1")
	{
		std::cout << "[fuzzer] STUN fuzzer enabled" << std::endl;

		fuzzStun = true;
	}

	if (std::getenv("MS_FUZZ_DTLS") && std::string(std::getenv("MS_FUZZ_DTLS")) == "1")
	{
		std::cout << "[fuzzer] DTLS fuzzer enabled" << std::endl;

		fuzzDtls = true;
	}

	if (std::getenv("MS_FUZZ_SCTP") && std::string(std::getenv("MS_FUZZ_SCTP")) == "1")
	{
		std::cout << "[fuzzer] SCTP fuzzer enabled" << std::endl;

		fuzzSctp = true;
	}

	if (std::getenv("MS_FUZZ_RTP") && std::string(std::getenv("MS_FUZZ_RTP")) == "1")
	{
		std::cout << "[fuzzer] RTP fuzzer enabled" << std::endl;

		fuzzRtp = true;
	}

	if (std::getenv("MS_FUZZ_RTCP") && std::string(std::getenv("MS_FUZZ_RTCP")) == "1")
	{
		std::cout << "[fuzzer] RTCP fuzzer enabled" << std::endl;

		fuzzRtcp = true;
	}

	if (std::getenv("MS_FUZZ_CODECS") && std::string(std::getenv("MS_FUZZ_CODECS")) == "1")
	{
		std::cout << "[fuzzer] codecs fuzzer enabled" << std::endl;

		fuzzCodecs = true;
	}

	if (std::getenv("MS_FUZZ_UTILS") && std::string(std::getenv("MS_FUZZ_UTILS")) == "1")
	{
		std::cout << "[fuzzer] Utils fuzzer enabled" << std::endl;

		fuzzUtils = true;
	}

	if (!fuzzStun && !fuzzDtls && !fuzzSctp && !fuzzRtp && !fuzzRtcp && !fuzzCodecs && !fuzzUtils)
	{
		std::cout << "[fuzzer] all fuzzers enabled" << std::endl;

		fuzzStun   = true;
		fuzzDtls   = true;
		fuzzSctp   = true;
		fuzzRtp    = true;
		fuzzRtcp   = true;
		fuzzCodecs = true;
		fuzzUtils  = true;
	}

	// Initialize static stuff.
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	DepLibSRTP::ClassInit();
	DepUsrSCTP::ClassInit();
	DepLibWebRTC::ClassInit();
	Utils::Crypto::ClassInit();
	::RTC::DtlsTransport::ClassInit();

	return 0;
}
