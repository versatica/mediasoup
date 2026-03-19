#define MS_CLASS "fuzzer"

#include "common.hpp"
#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#include "DepLibWebRTC.hpp"
#include "DepOpenSSL.hpp"
#ifndef MS_SCTP_STACK
#include "DepUsrSCTP.hpp"
#endif
#include "FuzzerUtils.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "RTC/DtlsTransport.hpp"
#include "RTC/FuzzerDtlsTransport.hpp"
#include "RTC/FuzzerRateCalculator.hpp"
#include "RTC/FuzzerSeqManager.hpp"
#include "RTC/FuzzerTrendCalculator.hpp"
#include "RTC/ICE/FuzzerStunPacket.hpp"
#include "RTC/RTCP/FuzzerPacket.hpp"
#include "RTC/RTP/Codecs/FuzzerAV1.hpp"
#include "RTC/RTP/Codecs/FuzzerDependencyDescriptor.hpp"
#include "RTC/RTP/Codecs/FuzzerH264.hpp"
#include "RTC/RTP/Codecs/FuzzerOpus.hpp"
#include "RTC/RTP/Codecs/FuzzerVP8.hpp"
#include "RTC/RTP/Codecs/FuzzerVP9.hpp"
#include "RTC/RTP/FuzzerPacket.hpp"
#include "RTC/RTP/FuzzerProbationGenerator.hpp"
#include "RTC/RTP/FuzzerRetransmissionBuffer.hpp"
#include "RTC/RTP/FuzzerRtpStreamSend.hpp"
#include "RTC/SCTP/FuzzerStateCookie.hpp"
#include "RTC/SCTP/packet/FuzzerPacket.hpp"
#include <cstdlib> // std::getenv()
#include <iostream>
#include <sstream> // std::istringstream()
#include <string>
#include <vector>

namespace
{
	// NOLINTBEGIN(readability-identifier-naming)
	bool fuzzStun   = false;
	bool fuzzDtls   = false;
	bool fuzzSctp   = false;
	bool fuzzRtp    = false;
	bool fuzzRtcp   = false;
	bool fuzzCodecs = false;
	bool fuzzUtils  = false;
	// NOLINTEND(readability-identifier-naming)

	int init()
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

		if (std::getenv("MS_FUZZ_STUN"))
		{
			std::cout << "[fuzzer] STUN fuzzer enabled" << std::endl;

			fuzzStun = true;
		}

		if (std::getenv("MS_FUZZ_DTLS"))
		{
			std::cout << "[fuzzer] DTLS fuzzer enabled" << std::endl;

			fuzzDtls = true;
		}

		if (std::getenv("MS_FUZZ_SCTP"))
		{
			std::cout << "[fuzzer] SCTP fuzzer enabled" << std::endl;

			fuzzSctp = true;
		}

		if (std::getenv("MS_FUZZ_RTP"))
		{
			std::cout << "[fuzzer] RTP fuzzer enabled" << std::endl;

			fuzzRtp = true;
		}

		if (std::getenv("MS_FUZZ_RTCP"))
		{
			std::cout << "[fuzzer] RTCP fuzzer enabled" << std::endl;

			fuzzRtcp = true;
		}

		if (std::getenv("MS_FUZZ_CODECS"))
		{
			std::cout << "[fuzzer] codecs fuzzer enabled" << std::endl;

			fuzzCodecs = true;
		}

		if (std::getenv("MS_FUZZ_UTILS"))
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
#ifndef MS_SCTP_STACK
		DepUsrSCTP::ClassInit();
#endif
		DepLibWebRTC::ClassInit();
		Utils::Crypto::ClassInit();
		RTC::DtlsTransport::ClassInit();

		return 0;
	}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	// Trick to initialize our stuff just once.
	// NOLINTNEXTLINE(readability-identifier-naming)
	thread_local const int unused = init();

	// Avoid [-Wunused-variable].
	(void)unused;

	if (fuzzStun)
	{
		FuzzerRtcIceStunPacket::Fuzz(data, len);
	}

	if (fuzzDtls)
	{
		FuzzerRtcDtlsTransport::Fuzz(data, len);
	}

	if (fuzzSctp)
	{
		FuzzerRtcSctpPacket::Fuzz(data, len);
		FuzzerRtcSctpStateCookie::Fuzz(data, len);
	}

	if (fuzzRtp)
	{
		FuzzerRtcRtcPacket::Fuzz(data, len);
		FuzzerRtcRtpStreamSend::Fuzz(data, len);
		FuzzerRtcRtpRetransmissionBuffer::Fuzz(data, len);
		FuzzerRtcRtpProbationGenerator::Fuzz(data, len);
		FuzzerRtcSeqManager::Fuzz(data, len);
		FuzzerRtcRateCalculator::Fuzz(data, len);
	}

	if (fuzzRtcp)
	{
		FuzzerRtcRtcpPacket::Fuzz(data, len);
	}

	if (fuzzCodecs)
	{
		FuzzerRtcRtpCodecsOpus::Fuzz(data, len);
		FuzzerRtcRtpCodecsVP8::Fuzz(data, len);
		FuzzerRtcRtpCodecsVP9::Fuzz(data, len);
		FuzzerRtcRtpCodecsH264::Fuzz(data, len);
		FuzzerRtcRtpCodecsAV1::Fuzz(data, len);
		FuzzerRtcRtpCodecsDependencyDescriptor::Fuzz(data, len);
	}

	if (fuzzUtils)
	{
		FuzzerUtils::Fuzz(data, len);
		FuzzerRtcTrendCalculator::Fuzz(data, len);
	}

	return 0;
}
