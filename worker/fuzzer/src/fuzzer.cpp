#define MS_CLASS "fuzzer"

#include "common.hpp"
#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "RTC/DtlsTransport.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "fuzzer/include/FuzzerUtils.hpp"
#include "fuzzer/include/RTC/FuzzerDtlsTransport.hpp"
#include "fuzzer/include/RTC/FuzzerRateCalculator.hpp"
#include "fuzzer/include/RTC/FuzzerSeqManager.hpp"
#include "fuzzer/include/RTC/ICE/FuzzerStunPacket.hpp"
#include "fuzzer/include/RTC/RTCP/packet/FuzzerCompoundPacket.hpp"
#include "fuzzer/include/RTC/RTP/Codecs/FuzzerDependencyDescriptor.hpp"
#include "fuzzer/include/RTC/RTP/Codecs/FuzzerH264.hpp"
#include "fuzzer/include/RTC/RTP/Codecs/FuzzerOpus.hpp"
#include "fuzzer/include/RTC/RTP/Codecs/FuzzerVP8.hpp"
#include "fuzzer/include/RTC/RTP/Codecs/FuzzerVP9.hpp"
#include "fuzzer/include/RTC/RTP/FuzzerPacket.hpp"
#include "fuzzer/include/RTC/RTP/FuzzerRetransmissionBuffer.hpp"
#include "fuzzer/include/RTC/RTP/FuzzerRtpStreamSend.hpp"
#include "fuzzer/include/RTC/SCTP/FuzzerStateCookie.hpp"
#include "fuzzer/include/RTC/SCTP/packet/FuzzerPacket.hpp"
#include <cstdlib> // std::getenv()
#include <iostream>
#include <sstream> // std::istringstream
#include <string>
#include <vector>

namespace
{
	struct EnabledFuzzers
	{
		bool stun{ false };
		bool dtls{ false };
		bool sctp{ false };
		bool rtp{ false };
		bool rtcp{ false };
		bool codecs{ false };
		bool utils{ false };
	};

	EnabledFuzzers init()
	{
		const char* const logLevelPtr = std::getenv("MS_FUZZ_LOG_LEVEL");
		const char* const logTagsPtr  = std::getenv("MS_FUZZ_LOG_TAGS");

		std::string logLevel{ "none" };
		std::vector<std::string> logTags{ "info" };
		EnabledFuzzers enabledFuzzers;

		// Get logLevel from ENV variable.
		if (logLevelPtr)
		{
			logLevel = logLevelPtr;
		}

		// Get logTags from ENV variable.
		if (logTagsPtr)
		{
			std::istringstream iss{ logTagsPtr };
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

			enabledFuzzers.stun = true;
		}

		if (std::getenv("MS_FUZZ_DTLS"))
		{
			std::cout << "[fuzzer] DTLS fuzzer enabled" << std::endl;

			enabledFuzzers.dtls = true;
		}

		if (std::getenv("MS_FUZZ_SCTP"))
		{
			std::cout << "[fuzzer] SCTP fuzzer enabled" << std::endl;

			enabledFuzzers.sctp = true;
		}

		if (std::getenv("MS_FUZZ_RTP"))
		{
			std::cout << "[fuzzer] RTP fuzzer enabled" << std::endl;

			enabledFuzzers.rtp = true;
		}

		if (std::getenv("MS_FUZZ_RTCP"))
		{
			std::cout << "[fuzzer] RTCP fuzzer enabled" << std::endl;

			enabledFuzzers.rtcp = true;
		}

		if (std::getenv("MS_FUZZ_CODECS"))
		{
			std::cout << "[fuzzer] codecs fuzzer enabled" << std::endl;

			enabledFuzzers.codecs = true;
		}

		if (std::getenv("MS_FUZZ_UTILS"))
		{
			std::cout << "[fuzzer] Utils fuzzer enabled" << std::endl;

			enabledFuzzers.utils = true;
		}

		if (
		  !enabledFuzzers.stun && !enabledFuzzers.dtls && !enabledFuzzers.sctp && !enabledFuzzers.rtp &&
		  !enabledFuzzers.rtcp && !enabledFuzzers.codecs && !enabledFuzzers.utils)
		{
			std::cout << "[fuzzer] all fuzzers enabled" << std::endl;

			enabledFuzzers = {
				.stun   = true,
				.dtls   = true,
				.sctp   = true,
				.rtp    = true,
				.rtcp   = true,
				.codecs = true,
				.utils  = true,
			};
		}

		// Initialize static stuff.
		DepLibUV::ClassInit();
		DepOpenSSL::ClassInit();
		DepLibSRTP::ClassInit();
		Utils::Crypto::ClassInit();
		RTC::DtlsTransport::ClassInit();

		return enabledFuzzers;
	}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t len)
{
	// Initialize our stuff just once.
	static thread_local const EnabledFuzzers Enabled = init();

	if (Enabled.stun)
	{
		FuzzerRtcIceStunPacket::Fuzz(data, len);
	}

	if (Enabled.dtls)
	{
		FuzzerRtcDtlsTransport::Fuzz(data, len);
	}

	if (Enabled.sctp)
	{
		FuzzerRtcSctpPacket::Fuzz(data, len);
		FuzzerRtcSctpStateCookie::Fuzz(data, len);
	}

	if (Enabled.rtp)
	{
		FuzzerRtcRtpPacket::Fuzz(data, len);
		FuzzerRtcRtpStreamSend::Fuzz(data, len);
		FuzzerRtcRtpRetransmissionBuffer::Fuzz(data, len);
		FuzzerRtcSeqManager::Fuzz(data, len);
		FuzzerRtcRateCalculator::Fuzz(data, len);
	}

	if (Enabled.rtcp)
	{
		FuzzerRtcRtcpCompoundPacket::Fuzz(data, len);
	}

	if (Enabled.codecs)
	{
		FuzzerRtcRtpCodecsOpus::Fuzz(data, len);
		FuzzerRtcRtpCodecsVP8::Fuzz(data, len);
		FuzzerRtcRtpCodecsVP9::Fuzz(data, len);
		FuzzerRtcRtpCodecsH264::Fuzz(data, len);

		FuzzerRtcRtpCodecsDependencyDescriptor::Fuzz(data, len);
	}

	if (Enabled.utils)
	{
		FuzzerUtils::Fuzz(data, len);
	}

	return 0;
}
