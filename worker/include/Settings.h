#ifndef MS_SETTINGS_H
#define	MS_SETTINGS_H

#include "common.h"
#include "LogLevel.h"
#include "Channel/Request.h"
#include <map>
#include <string>
#include <vector>

class Settings
{
public:
	struct LogTags
	{
		bool info { false };
		bool ice  { false };
		bool dtls { false };
		bool rtp  { false };
		bool srtp { false };
		bool rtcp { false };
		// TODO: Add more tags (here and in Settings.cpp).
	};

public:
	// Struct holding the configuration.
	struct Configuration
	{
		LogLevel       logLevel             { LogLevel::LOG_DEBUG };
		struct LogTags logTags;
		std::string    rtcListenIPv4;
		std::string    rtcListenIPv6;
		uint16_t       rtcMinPort           { 10000 };
		uint16_t       rtcMaxPort           { 59999 };
		std::string    dtlsCertificateFile;
		std::string    dtlsPrivateKeyFile;
		// Private fields.
		bool           hasIPv4              { false };
		bool           hasIPv6              { false };
	};

public:
	static void SetConfiguration(int argc, char* argv[]);
	static void PrintConfiguration();
	static void HandleRequest(Channel::Request* request);

private:
	static void SetDefaultRtcListenIP(int requested_family);
	static void SetLogLevel(std::string &level);
	static void SetRtcListenIPv4(const std::string &ip);
	static void SetRtcListenIPv6(const std::string &ip);
	static void SetRtcPorts();
	static void SetDtlsCertificateAndPrivateKeyFiles();
	static void SetLogTags(std::vector<std::string>& tags);
	static void SetLogTags(Json::Value& json);

public:
	static struct Configuration configuration;

private:
	static std::map<std::string, LogLevel> string2LogLevel;
	static std::map<LogLevel, std::string> logLevel2String;
};

#endif
