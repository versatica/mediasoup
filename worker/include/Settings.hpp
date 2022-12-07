#ifndef MS_SETTINGS_HPP
#define MS_SETTINGS_HPP

#include "common.hpp"
#include "LogLevel.hpp"
#include "Channel/ChannelRequest.hpp"
#include <absl/container/flat_hash_map.h>
#include <string>
#include <vector>

class Settings
{
public:
	struct LogTags
	{
		bool info{ false };
		bool ice{ false };
		bool dtls{ false };
		bool rtp{ false };
		bool srtp{ false };
		bool rtcp{ false };
		bool rtx{ false };
		bool bwe{ false };
		bool score{ false };
		bool simulcast{ false };
		bool svc{ false };
		bool sctp{ false };
		bool message{ false };
	};

public:
	// Struct holding the configuration.
	struct Configuration
	{
		LogLevel logLevel{ LogLevel::LOG_ERROR };
		struct LogTags logTags;
		uint16_t rtcMinPort{ 10000u };
		uint16_t rtcMaxPort{ 59999u };
		std::string dtlsCertificateFile;
		std::string dtlsPrivateKeyFile;
		std::string libwebrtcFieldTrials{
			"WebRTC-Bwe-AlrLimitedBackoff/Enabled/WebRTC-Bwe-LossBasedBweV2/Enabled:true,CandidateFactors:1.02|1.0|0.95,DelayBasedCandidate:true,HigherBwBiasFactor:0.0002,HigherLogBwBiasFactor:0.02,ObservationDurationLowerBound:250ms,InstantUpperBoundBwBalance:75kbps,BwRampupUpperBoundFactor:1000000.0,InstantUpperBoundTemporalWeightFactor:0.9,TemporalWeightFactor:0.9,MaxIncreaseFactor:1.3,NewtonStepSize:0.75,InherentLossUpperBoundBwBalance:75kbps,LossThresholdOfHighBandwidthPreference:0.15,NotIncreaseIfInherentLossLessThanAverageLoss:true,TrendlineIntegrationEnabled:true,TrendlineObservationsWindowSize:2,SendingRateSmoothingFactor:0.6,InstantUpperBoundLossOffset:0.1/"
		};
	};

public:
	static void SetConfiguration(int argc, char* argv[]);
	static void PrintConfiguration();
	static void HandleRequest(Channel::ChannelRequest* request);

private:
	static void SetLogLevel(std::string& level);
	static void SetLogTags(const std::vector<std::string>& tags);
	static void SetDtlsCertificateAndPrivateKeyFiles();

public:
	thread_local static struct Configuration configuration;

private:
	static absl::flat_hash_map<std::string, LogLevel> string2LogLevel;
	static absl::flat_hash_map<LogLevel, std::string> logLevel2String;
};

#endif
