#ifndef MS_SETTINGS_H
#define	MS_SETTINGS_H

#include "common.h"
#include <map>
#include <string>
extern "C"
{
	#include <syslog.h>
}

class Settings
{
public:
	// Struct holding the configuration.
	struct Configuration
	{
		unsigned int logLevel             { LOG_DEBUG };
		bool         useSyslog            { false };
		unsigned int syslogFacility       { LOG_USER };

		struct
		{
			std::string listenIPv4;
			std::string listenIPv6;
			MS_PORT     minPort             { 10000 };
			MS_PORT     maxPort             { 59999 };
			std::string dtlsCertificateFile;
			std::string dtlsPrivateKeyFile;

			// Private fields.
			bool        hasIPv4             { false };
			bool        hasIPv6             { false };
		} RTC;
	};

public:
	static void SetConfiguration(int argc, char* argv[]);
	static void PrintConfiguration();

private:
	static void SetDefaultRtcListenIP(int requested_family);
	static void SetLogLevel(std::string &level);
	static void SetSyslogFacility(std::string &facility);
	static void SetRtcListenIPv4(const std::string &ip);
	static void SetRtcListenIPv6(const std::string &ip);
	static void SetRtcPorts();
	static void SetDtlsCertificateAndPrivateKeyFiles();

public:
	static struct Configuration configuration;

private:
	static std::map<std::string, unsigned int> string2LogLevel;
	static std::map<unsigned int, std::string> logLevel2String;
	static std::map<std::string, unsigned int> string2SyslogFacility;
	static std::map<unsigned int, std::string> syslogFacility2String;
};

#endif
