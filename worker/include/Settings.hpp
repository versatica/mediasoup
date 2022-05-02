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
		bool xcode{ false };
		bool message{ false };
	};

public:
	// Struct holding the configuration.
	struct Configuration
	{
		LogLevel logLevel{ LogLevel::LOG_ERROR };
		LogDevLevel logDevLevel{LogDevLevel::LOG_DEV_NONE};
		bool logTraceEnabled{false};
		struct LogTags logTags;
		uint16_t rtcMinPort{ 10000u };
		uint16_t rtcMaxPort{ 59999u };
		std::string dtlsCertificateFile;
		std::string dtlsPrivateKeyFile;
	};

public:
	static void SetConfiguration(int argc, char* argv[]);
	static void PrintConfiguration();
	static void HandleRequest(Channel::ChannelRequest* request);

private:
	static void SetLogLevel(std::string& level);
	static void SetLogTags(const std::vector<std::string>& tags);
	static void SetLogDevLevel(std::string& devLevel);
	static void SetTrace(bool trace);
	static void SetDtlsCertificateAndPrivateKeyFiles();

public:
	thread_local static struct Configuration configuration;

private:
	static std::map<std::string, LogDevLevel> string2LogDevLevel;
	static std::map<LogDevLevel, std::string> logDevLevel2String;
	static absl::flat_hash_map<std::string, LogLevel> string2LogLevel;
	static absl::flat_hash_map<LogLevel, std::string> logLevel2String;
};

#endif
