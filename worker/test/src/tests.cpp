#include "common.hpp"
#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#include "DepLibWebRTC.hpp"
#include "DepOpenSSL.hpp"
#ifndef MS_SCTP_STACK
#include "DepUsrSCTP.hpp"
#endif
#include "Settings.hpp"
#include "Utils.hpp"
#include <catch2/catch_session.hpp>
#include <cstdlib> // std::getenv()
#include <sstream> // std::istringstream()
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
	std::string logLevel{ "none" };
	std::vector<std::string> logTags = { "info" };

	const auto* logLevelPtr = std::getenv("MS_TEST_LOG_LEVEL");
	const auto* logTagsPtr  = std::getenv("MS_TEST_LOG_TAGS");

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

	// Initialize static stuff.
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	DepLibSRTP::ClassInit();
#ifndef MS_SCTP_STACK
	DepUsrSCTP::ClassInit();
#endif
	DepLibWebRTC::ClassInit();
	Utils::Crypto::ClassInit();

	Catch::Session session;

	const int status = session.run(argc, argv);

	// Free static stuff.
	DepLibSRTP::ClassDestroy();
	Utils::Crypto::ClassDestroy();
	DepLibWebRTC::ClassDestroy();
#ifndef MS_SCTP_STACK
	DepUsrSCTP::ClassDestroy();
#endif
	DepLibUV::ClassDestroy();

	return status;
}
