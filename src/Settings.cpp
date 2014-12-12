#define MS_CLASS "Settings"

#include "Settings.h"
#include "Version.h"
#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>
#include <map>
#include <algorithm>  // std::transform()
#include <cstdio>  // std::fprintf()
#include <cstdlib>  // std::_Exit()
#include <cerrno>
#include <unistd.h>  // close()
#include <uv.h>
extern "C" {
	#include <getopt.h>
	#include <syslog.h>
}


/* Static variables. */

struct Settings::Arguments Settings::arguments;
struct Settings::Configuration Settings::configuration;
std::map<std::string, unsigned int> Settings::string2LogLevel = {
	{ "debug",  LOG_DEBUG   },
	{ "info",   LOG_INFO    },
	{ "notice", LOG_NOTICE  },
	{ "warn",   LOG_WARNING },
	{ "error",  LOG_ERR     }
};
std::map<unsigned int, std::string> Settings::logLevel2String = {
	{ LOG_DEBUG,   "debug"  },
	{ LOG_INFO,    "info"   },
	{ LOG_NOTICE,  "notice" },
	{ LOG_WARNING, "warn"   },
	{ LOG_ERR,     "error"  }
};
std::map<std::string, unsigned int> Settings::string2SyslogFacility = {
	{ "user",   LOG_USER   },
	{ "local0", LOG_LOCAL0 },
	{ "local1", LOG_LOCAL1 },
	{ "local2", LOG_LOCAL2 },
	{ "local3", LOG_LOCAL3 },
	{ "local4", LOG_LOCAL4 },
	{ "local5", LOG_LOCAL5 },
	{ "local6", LOG_LOCAL6 },
	{ "local7", LOG_LOCAL7 }
};
std::map<unsigned int, std::string> Settings::syslogFacility2String = {
	{ LOG_USER,   "user"   },
	{ LOG_LOCAL0, "local0" },
	{ LOG_LOCAL1, "local1" },
	{ LOG_LOCAL2, "local2" },
	{ LOG_LOCAL3, "local3" },
	{ LOG_LOCAL4, "local4" },
	{ LOG_LOCAL5, "local5" },
	{ LOG_LOCAL6, "local6" },
	{ LOG_LOCAL7, "local7" }
};


/* Static methods. */

void Settings::ReadArguments(int argc, char* argv[]) {
	MS_TRACE();

	/* Variables for getopt. */

	int c;
	int option_index = 0;
	extern char *optarg;
	extern int optind, opterr, optopt;

	// For getopt_long().
	// Begin with ":" so we get ':' error in case a valid option requires argument.
	std::string short_options = ":c:dp:u:g:vh";

	// For getopt_long().
	struct option long_options[] = {
		{ "configfile", required_argument, nullptr, 'c' },
		{ "daemonize",  no_argument,       nullptr, 'd' },
		{ "pidfile",    required_argument, nullptr, 'p' },
		{ "user",       required_argument, nullptr, 'u' },
		{ "group",      required_argument, nullptr, 'g' },
		{ "version",    no_argument,       nullptr, 'v' },
		{ "help",       no_argument,       nullptr, 'h' },
		{ 0, 0, 0, 0 }
	};

	// A map for associating short and long option names.
	std::map<char, std::string> long_option_names = {
		{ 'c', "configfile" },
		{ 'd', "daemonize"  },
		{ 'p', "pidfile"    },
		{ 'u', "user"       },
		{ 'g', "group"      },
		{ 'v', "version"    },
		{ 'h', "help"       }
	};

	/* Parse command line options. */

	opterr = 0;  // Don't allow getopt to print error messages.
	while ((c = getopt_long(argc, argv, short_options.c_str(), long_options, &option_index)) != -1) {
		switch(c) {
			case 'c':
				Settings::arguments.configFile = optarg;
				break;

			case 'd':
				Settings::arguments.daemonize = true;
				break;

			case 'p':
				Settings::arguments.pidFile = optarg;
				break;

			case 'u':
				Settings::arguments.user = optarg;
				break;

			case 'g':
				Settings::arguments.group = optarg;
				break;

			case 'v':
				Settings::PrintVersion();
				std::_Exit(EXIT_SUCCESS);
				break;

			case 'h':
				Settings::PrintVersion();
				Settings::PrintHelp(false);
				std::_Exit(EXIT_SUCCESS);
				break;

			// Invalid option.
			case '?':
				if (isprint(optopt)) {
					MS_ERROR("invalid option '-%c'", (char)optopt);
					Settings::PrintHelp(true);
					std::_Exit(EXIT_FAILURE);
				}
				else {
					MS_ERROR("unknown long option given as argument");
					Settings::PrintHelp(true);
					std::_Exit(EXIT_FAILURE);
				}

			// Valid option, but it requires and argument that is not given.
			case ':':
				MS_ERROR("option '-%c' or '--%s' requires an argument", (char)optopt, long_option_names[(char)optopt].c_str());
				Settings::PrintHelp(true);
				std::_Exit(EXIT_FAILURE);
				break;

			// This should never happen.
			default:
				MS_ABORT("'default' should never happen");
		}
	}

	// Ensure there are no more command line arguments after parsed options.
	if (optind != argc)
		MS_EXIT_FAILURE("there are remaining arguments after parsing command line options");

	// Ensure that PID file is not given when in foreground mode.
	if (! Settings::arguments.pidFile.empty() && ! Settings::arguments.daemonize)
		MS_EXIT_FAILURE("PID file option requires daemon mode");
}


void Settings::PrintHelp(bool error) {
	MS_TRACE();

	std::string help;

	help.append("\nUsage: ");
	help.append(Version::command);
	help.append(" [options]\n");

	help.append("Options:\n");

	help.append("  -c, --configfile FILE     Path to the configuration file\n");
	help.append("  -d, --daemonize           Run in daemon mode\n");
	help.append("  -p, --pidfile FILE        Create a PID file (requires daemon mode)\n");
	help.append("  -u, --user USER           Run with the given system user\n");
	help.append("  -g, --group GROUP         Run with the given system group\n");
	help.append("  -v, --version             Show version\n");
	help.append("  -h, --help                Show this message\n");

	if (error)
		std::fprintf(stderr, "%s", help.c_str());
	else
		std::fprintf(stdout, "%s", help.c_str());
}


void Settings::PrintVersion() {
	MS_TRACE();

	std::fprintf(stdout, "%s\n", Version::GetNameAndVersion().c_str());
	std::fprintf(stdout, "%s\n", Version::copyright.c_str());
}


void Settings::SetDefaultConfiguration() {
	MS_TRACE();

	SetDefaultNumWorkers();
	SetDefaultRTClistenIP(AF_INET);
	SetDefaultRTClistenIP(AF_INET6);
}


void Settings::ReadConfigurationFile() {
	MS_TRACE();

	if (Settings::arguments.configFile.empty())
		return;

	libconfig::Config* config;

	try {
		config = ParseConfigFile();
	}
	catch (const MediaSoupError &error) {
		MS_EXIT_FAILURE("%s", error.what());
	}

	std::string str_value;
	std::string str_value2;
	int int_value;
	int int_value2;
	bool bool_value;
	std::string empty_string;

	try {
		/* First level settings. */

		if (config->lookupValue("logLevel", str_value))
			SetLogLevel(str_value);

		if (config->lookupValue("syslogFacility", str_value))
			SetSyslogFacility(str_value);

		if (config->lookupValue("numWorkers", int_value))
			SetNumWorkers(int_value);

		/* ControlProtocol section. */

		if (config->lookupValue("ControlProtocol.listenIP", str_value))
			SetControlProtocolListenIP(str_value);

		if (config->lookupValue("ControlProtocol.listenPort", int_value))
			SetControlProtocolListenPort(int_value);

		/* RTC section. */

		if (config->lookupValue("RTC.listenIPv4", str_value))
			SetRTClistenIPv4(str_value);
		else if ((config->lookupValue("RTC.listenIPv4", bool_value)) && bool_value == false)
			SetRTClistenIPv4(empty_string);

		if (config->lookupValue("RTC.listenIPv6", str_value))
			SetRTClistenIPv6(str_value);
		else if ((config->lookupValue("RTC.listenIPv6", bool_value)) && bool_value == false)
			SetRTClistenIPv6(empty_string);

		if (config->lookupValue("RTC.minPort", int_value) && config->lookupValue("RTC.maxPort", int_value2))
			SetRTCports(int_value, int_value2);

		if (config->lookupValue("RTC.dtlsCertificateFile", str_value) && config->lookupValue("RTC.dtlsPrivateKeyFile", str_value2))
			SetDtlsCertificateAndPrivateKeyFiles(str_value, str_value2);
	}
	catch (const MediaSoupError &error) {
		delete config;
		MS_EXIT_FAILURE("error in configuration file: %s", error.what());
	}

	delete config;
}


bool Settings::ReloadConfigurationFile() {
	MS_TRACE();

	if (Settings::arguments.configFile.empty()) {
		MS_ERROR("no configuration file was given in command line options");
		return false;
	}

	libconfig::Config* config;

	try {
		config = ParseConfigFile();
	}
	catch (const MediaSoupError &error) {
		MS_ERROR("%s", error.what());
		return false;
	}

	std::string str_value;

	// Just some configuration settings can be reloaded.

	try {
		/* First level settings. */

		if (config->lookupValue("logLevel", str_value))
			SetLogLevel(str_value);
		else
			Settings::configuration.logLevel = LOG_DEBUG;
	}
	catch (const MediaSoupError &error) {
		MS_ERROR("error in configuration file: %s", error.what());
		delete config;
		return false;
	}

	delete config;
	return true;
}


void Settings::PrintConfiguration() {
	MS_TRACE();

	MS_INFO("[configuration]");

	MS_INFO("- logLevel: \"%s\"", Settings::logLevel2String[Settings::configuration.logLevel].c_str());
	MS_INFO("- syslogFacility: \"%s\"", Settings::syslogFacility2String[Settings::configuration.syslogFacility].c_str());
	MS_INFO("- numWorkers: %d", Settings::configuration.numWorkers);

	MS_INFO("- ControlProtocol:");
	MS_INFO("  - listenIP: \"%s\"", Settings::configuration.ControlProtocol.listenIP.c_str());
	MS_INFO("  - listenPort: %d", Settings::configuration.ControlProtocol.listenPort);

	MS_INFO("- RTC:");
	if (Settings::configuration.RTC.hasIPv4)
		MS_INFO("  - listenIPv4: \"%s\"", Settings::configuration.RTC.listenIPv4.c_str());
	else
		MS_INFO("  - listenIPv4: (unavailable)");
	if (Settings::configuration.RTC.hasIPv6)
		MS_INFO("  - listenIPv6: \"%s\"", Settings::configuration.RTC.listenIPv6.c_str());
	else
		MS_INFO("  - listenIPv6: (unavailable)");
	MS_INFO("  - minPort: %d", Settings::configuration.RTC.minPort);
	MS_INFO("  - maxPort: %d", Settings::configuration.RTC.maxPort);
	if (! Settings::configuration.RTC.dtlsCertificateFile.empty()) {
		MS_INFO("  - dtlsCertificateFile: \"%s\"", Settings::configuration.RTC.dtlsCertificateFile.c_str());
		MS_INFO("  - dtlsPrivateKeyFile: \"%s\"", Settings::configuration.RTC.dtlsPrivateKeyFile.c_str());
	}

	MS_INFO("[/configuration]");
}


libconfig::Config* Settings::ParseConfigFile() {
	MS_TRACE();

	const char* config_file = Settings::arguments.configFile.c_str();

	try {
		Utils::File::CheckFile(config_file);
	}
	catch (const MediaSoupError &error) {
		MS_THROW_ERROR("error reading configuration file: %s", error.what());
	}

	libconfig::Config* config = new libconfig::Config();

	try {
		config->readFile(config_file);
	}
	catch (const libconfig::ParseException &error) {
		delete config;
		MS_THROW_ERROR("parsing error in configuration file '%s': %s in line %d", config_file, error.getError(), error.getLine());
	}
	catch (const libconfig::FileIOException &error) {
		delete config;
		MS_THROW_ERROR("cannot read configuration file '%s'", config_file);
	}
	catch (const libconfig::ConfigException &error) {
		delete config;
		MS_THROW_ERROR("error reading configuration file '%s': %s", config_file, error.what());
	}

	return config;
}


void Settings::SetLogLevel(std::string &level) {
	MS_TRACE();

	// Downcase given level.
	std::transform(level.begin(), level.end(), level.begin(), ::tolower);

	if (Settings::string2LogLevel.find(level) == Settings::string2LogLevel.end())
		MS_THROW_ERROR("invalid value '%s' for Logging.level", level.c_str());

	Settings::configuration.logLevel = Settings::string2LogLevel[level];
}


void Settings::SetSyslogFacility(std::string &facility) {
	MS_TRACE();

	// Downcase given facility.
	std::transform(facility.begin(), facility.end(), facility.begin(), ::tolower);

	if (Settings::string2SyslogFacility.find(facility) == Settings::string2SyslogFacility.end())
		MS_THROW_ERROR("invalid value '%s' for Logging.syslogFacility", facility.c_str());

	Settings::configuration.syslogFacility = Settings::string2SyslogFacility[facility];
}


void Settings::SetNumWorkers(int numWorkers) {
	MS_TRACE();

	if (numWorkers < 0)
		MS_THROW_ERROR("numWorkers must be greater or equal than 0");
	// If 0 let the already set default value.
	else if (numWorkers == 0)
		return;

	Settings::configuration.numWorkers = numWorkers;
}


void Settings::SetDefaultNumWorkers() {
	MS_TRACE();

	int err;
	uv_cpu_info_t* cpus;
	int num_cpus;

	err = uv_cpu_info(&cpus, &num_cpus);
	if (err)
		MS_ABORT("uv_cpu_info() failed: %s", uv_strerror(err));

	uv_free_cpu_info(cpus, num_cpus);

	MS_DEBUG("auto-detected value for numWorkers: %d", num_cpus);
	Settings::configuration.numWorkers = num_cpus;
}


void Settings::SetControlProtocolListenIP(const std::string &ip) {
	MS_TRACE();

	switch (Utils::IP::GetFamily(ip)) {
		case AF_INET:
		case AF_INET6:
			Settings::configuration.ControlProtocol.listenIP = ip;
			break;
		default:
			MS_THROW_ERROR("invalid value '%s' for ControlProtocol.listenIP", ip.c_str());
	}
}


void Settings::SetControlProtocolListenPort(MS_PORT port) {
	MS_TRACE();

	Settings::configuration.ControlProtocol.listenPort = port;
}


void Settings::SetRTClistenIPv4(const std::string &ip) {
	MS_TRACE();

	if (ip.empty()) {
		Settings::configuration.RTC.listenIPv4.clear();
		Settings::configuration.RTC.hasIPv4 = false;
		return;
	}

	switch (Utils::IP::GetFamily(ip)) {
		case AF_INET:
			if (ip == "0.0.0.0")
				MS_THROW_ERROR("RTC.listenIPv4 cannot be '0.0.0.0'");
			Settings::configuration.RTC.listenIPv4 = ip;
			Settings::configuration.RTC.hasIPv4 = true;
			break;
		case AF_INET6:
			MS_THROW_ERROR("invalid IPv6 '%s' for RTC.listenIPv4", ip.c_str());
		default:
			MS_THROW_ERROR("invalid value '%s' for RTC.listenIPv4", ip.c_str());
	}

	int _bind_err;
	if (! IsBindableIP(ip, AF_INET, &_bind_err))
		MS_THROW_ERROR("cannot bind on '%s' for RTC.listenIPv4: %s", ip.c_str(), std::strerror(_bind_err));
}


void Settings::SetRTClistenIPv6(const std::string &ip) {
	MS_TRACE();

	if (ip.empty()) {
		Settings::configuration.RTC.listenIPv6.clear();
		Settings::configuration.RTC.hasIPv6 = false;
		return;
	}

	switch (Utils::IP::GetFamily(ip)) {
		case AF_INET6:
			if (ip == "::")
				MS_THROW_ERROR("RTC.listenIPv6 cannot be '::'");
			Settings::configuration.RTC.listenIPv6 = ip;
			Settings::configuration.RTC.hasIPv6 = true;
			break;
		case AF_INET:
			MS_THROW_ERROR("invalid IPv4 '%s' for RTC.listenIPv6", ip.c_str());
		default:
			MS_THROW_ERROR("invalid value '%s' for RTC.listenIPv6", ip.c_str());
	}

	int _bind_err;
	if (! IsBindableIP(ip, AF_INET6, &_bind_err))
		MS_THROW_ERROR("cannot bind on '%s' for RTC.listenIPv6: %s", ip.c_str(), std::strerror(_bind_err));
}


void Settings::SetDefaultRTClistenIP(int requested_family) {
	MS_TRACE();

	int err;
	uv_interface_address_t* addresses;
	int num_addresses;
	std::string ipv4;
	std::string ipv6;
	int _bind_err;

	err = uv_interface_addresses(&addresses, &num_addresses);
	if (err)
		MS_ABORT("uv_interface_addresses() failed: %s", uv_strerror(err));

	for (int i=0; i<num_addresses; i++) {
		uv_interface_address_t address = addresses[i];

		// Ignore internal addresses.
		if (address.is_internal)
			continue;

		int family;
		MS_PORT port;
		std::string ip;
		Utils::IP::GetAddressInfo((struct sockaddr*)(&address.address.address4), &family, ip, &port);

		if (family != requested_family)
			continue;

		switch(family) {
			case AF_INET:
				// Ignore if already got an IPv4.
				if (! ipv4.empty())
					continue;

				// Check if it is bindable.
				if (! IsBindableIP(ip, AF_INET, &_bind_err)) {
					MS_DEBUG("ignoring '%s' for RTC.listenIPv4: %s", ip.c_str(), std::strerror(errno));
					continue;
				}

				MS_DEBUG("auto-discovered '%s' for RTC.listenIPv4", ip.c_str());
				ipv4 = ip;
				break;

			case AF_INET6:
				// Ignore if already got an IPv6.
				if (! ipv6.empty())
					continue;

				// Check if it is bindable.
				if (! IsBindableIP(ip, AF_INET6, &_bind_err)) {
					MS_DEBUG("ignoring '%s' for RTC.listenIPv6: %s", ip.c_str(), std::strerror(errno));
					continue;
				}

				MS_DEBUG("auto-discovered '%s' for RTC.listenIPv6", ip.c_str());
				ipv6 = ip;
				break;
		}
	}

	if (! ipv4.empty()) {
		Settings::configuration.RTC.listenIPv4 = ipv4;
		Settings::configuration.RTC.hasIPv4 = true;
	}

	if (! ipv6.empty()) {
		Settings::configuration.RTC.listenIPv6 = ipv6;
		Settings::configuration.RTC.hasIPv6 = true;
	}

	uv_free_interface_addresses(addresses, num_addresses);
}


bool Settings::IsBindableIP(const std::string &ip, int family, int* _bind_err) {
	MS_TRACE();

	struct sockaddr_storage bind_addr;
	int bind_socket;
	int err = 0;
	bool bind_ok;

	switch (family) {
		case AF_INET:
			err = uv_ip4_addr(ip.c_str(), 0, (struct sockaddr_in*)&bind_addr);
			if (err)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));

			bind_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (bind_socket == -1)
				MS_ABORT("socket() failed: %s", std::strerror(errno));

			err = bind(bind_socket, (const struct sockaddr*)&bind_addr, sizeof(struct sockaddr_in));
			break;

		case AF_INET6:
			uv_ip6_addr(ip.c_str(), 0, (struct sockaddr_in6*)&bind_addr);
			if (err)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));
			bind_socket = socket(AF_INET6, SOCK_DGRAM, 0);
			if (bind_socket == -1)
				MS_ABORT("socket() failed: %s", std::strerror(errno));

			err = bind(bind_socket, (const struct sockaddr*)&bind_addr, sizeof(struct sockaddr_in6));
			break;

		default:
			MS_ABORT("unknown family");
	}

	if (err == 0) {
		bind_ok = true;
	}
	else {
		bind_ok = false;
		*_bind_err = errno;
	}

	err = close(bind_socket);
	if (err)
		MS_ABORT("close() failed: %s", std::strerror(errno));

	return bind_ok;
}


void Settings::SetRTCports(MS_PORT minPort, MS_PORT maxPort) {
	MS_TRACE();

	if (minPort < 1024)
		MS_THROW_ERROR("RTC.minPort must be greater or equal than 1024");

	if (maxPort == 0)
		MS_THROW_ERROR("RTC.maxPort can not be 0");

	// Make minPort even.
	minPort &= ~1;

	// Make maxPort odd.
	if (maxPort % 2 == 0)
		maxPort--;

	if ((maxPort - minPort) < 99)
		MS_THROW_ERROR("RTC.maxPort must be at least 99 ports higher than RTC.minPort");

	Settings::configuration.RTC.minPort = minPort;
	Settings::configuration.RTC.maxPort = maxPort;
}


void Settings::SetDtlsCertificateAndPrivateKeyFiles(std::string& dtlsCertificateFile, std::string& dtlsPrivateKeyFile) {
	MS_TRACE();

	try {
		Utils::File::CheckFile(dtlsCertificateFile.c_str());
	}
	catch (const MediaSoupError &error) {
		MS_THROW_ERROR("RTC.dtlsCertificateFile: %s", error.what());
	}

	try {
		Utils::File::CheckFile(dtlsPrivateKeyFile.c_str());
	}
	catch (const MediaSoupError &error) {
		MS_THROW_ERROR("RTC.dtlsPrivateKeyFile: %s", error.what());
	}

	Settings::configuration.RTC.dtlsCertificateFile = dtlsCertificateFile;
	Settings::configuration.RTC.dtlsPrivateKeyFile = dtlsPrivateKeyFile;
}


void Settings::ConfigurationPostCheck() {
	MS_TRACE();

	// RTC must have at least 'listenIPv4' or 'listenIPv6'.
	if (! Settings::configuration.RTC.hasIPv4 && ! Settings::configuration.RTC.hasIPv6)
		MS_EXIT_FAILURE("at least RTC.listenIPv4 or RTC.listenIPv6 must be enabled");
}
