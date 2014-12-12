#ifndef MS_SETTINGS_H
#define	MS_SETTINGS_H


#include "common.h"
#include <map>
#include <string>
#include <vector>
#include <libconfig.h++>
extern "C" {
	#include <syslog.h>
}


class Settings {
public:
	// Struct holding command line arguments.
	struct Arguments {
		std::string         configFile;
		bool                daemonize       { false };
		std::string         pidFile;
		std::string         user;
		std::string         group;
	};

	// Struct holding the configuration (can be overriden by a config file).
	struct Configuration {
		unsigned int        logLevel        { LOG_DEBUG };
		unsigned int        syslogFacility  { LOG_USER };
		int                 numWorkers      { 0 };

		struct {
			std::string     listenIP        { "127.0.0.1" };
			MS_PORT         listenPort      { 4080 };
		} ControlProtocol;

		struct {
			std::string     listenIPv4;
			std::string     listenIPv6;
			MS_PORT         minPort         { 10000 };
			MS_PORT         maxPort         { 59999 };
			std::string     dtlsCertificateFile;
			std::string     dtlsPrivateKeyFile;
			// Private fields.
			bool            hasIPv4         { false };
			bool            hasIPv6         { false };
		} RTC;
	};

public:
	static void ReadArguments(int argc, char* argv[]);
	static void PrintHelp(bool error);
	static void PrintVersion();
	static void SetDefaultConfiguration();
	static void ReadConfigurationFile();
	static void ConfigurationPostCheck();
	static bool ReloadConfigurationFile();
	static void PrintConfiguration();

private:
	static libconfig::Config* ParseConfigFile();
	static void SetLogLevel(std::string &level);
	static void SetSyslogFacility(std::string &facility);
	static void SetNumWorkers(int numWorkers);
	static void SetDefaultNumWorkers();
	static void SetControlProtocolListenIP(const std::string &ip);
	static void SetControlProtocolListenPort(MS_PORT port);
	static void SetRTClistenIPv4(const std::string &ip);
	static void SetRTClistenIPv6(const std::string &ip);
	static void SetDefaultRTClistenIP(int requested_family);
	static bool IsBindableIP(const std::string &ip, int family, int* _bind_err);
	static void SetRTCports(MS_PORT minPort, MS_PORT maxPort);
	static void SetDtlsCertificateAndPrivateKeyFiles(std::string& dtlsCertificateFile, std::string& dtlsPrivateKeyFile);

public:
	static struct Arguments arguments;
	static struct Configuration configuration;

private:
	static std::map<std::string, unsigned int> string2LogLevel;
	static std::map<unsigned int, std::string> logLevel2String;
	static std::map<std::string, unsigned int> string2SyslogFacility;
	static std::map<unsigned int, std::string> syslogFacility2String;
};


#endif
