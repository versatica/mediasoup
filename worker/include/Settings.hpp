#ifndef MS_SETTINGS_HPP
#define MS_SETTINGS_HPP

#include "common.hpp"
#include "LogLevel.hpp"
#include "Channel/Request.hpp"
#include <map>
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
		bool rbe{ false };
		bool tmp{ false }; // For debugging during development..
	};

public:
	// Struct holding the configuration.
	struct Configuration
	{
		LogLevel logLevel{ LogLevel::LOG_DEBUG };
		struct LogTags logTags;
		std::string rtcIPv4;
		std::string rtcIPv6;
		std::string rtcAnnouncedIPv4;
		std::string rtcAnnouncedIPv6;
		uint16_t rtcMinPort{ 10000 };
		uint16_t rtcMaxPort{ 59999 };
		std::string dtlsCertificateFile;
		std::string dtlsPrivateKeyFile;
		// Private fields.
		bool hasIPv4{ false };
		bool hasIPv6{ false };
		bool hasAnnouncedIPv4{ false };
		bool hasAnnouncedIPv6{ false };
	};

public:
	static void SetConfiguration(int argc, char* argv[]);
	static void PrintConfiguration();
	static void HandleRequest(Channel::Request* request);

private:
	static void SetDefaultRtcIP(int requestedFamily);
	static void SetLogLevel(std::string& level);
	static void SetRtcIPv4(const std::string& ip);
	static void SetRtcIPv6(const std::string& ip);
	static void SetRtcPorts();
	static void SetDtlsCertificateAndPrivateKeyFiles();
	static void SetLogTags(const std::vector<std::string>& tags);
	static void SetLogTags(Json::Value& json);

public:
	static struct Configuration configuration;

private:
	static std::map<std::string, LogLevel> string2LogLevel;
	static std::map<LogLevel, std::string> logLevel2String;
};

#endif
