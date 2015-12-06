#define MS_CLASS "Settings"

#include "Settings.h"
#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <string>
#include <map>
#include <algorithm>  // std::transform()
#include <cctype>  // isprint()
#include <cerrno>
#include <unistd.h>  // close()
#include <uv.h>
extern "C"
{
	#include <getopt.h>
	#include <syslog.h>
}

/* Helpers declaration. */

static
bool IsBindableIP(const std::string &ip, int family, int* _bind_err);

/* Static variables. */

struct Settings::Configuration Settings::configuration;
std::map<std::string, unsigned int> Settings::string2LogLevel =
{
	{ "debug",  LOG_DEBUG   },
	{ "info",   LOG_INFO    },
	{ "notice", LOG_NOTICE  },
	{ "warn",   LOG_WARNING },
	{ "error",  LOG_ERR     }
};
std::map<unsigned int, std::string> Settings::logLevel2String =
{
	{ LOG_DEBUG,   "debug"  },
	{ LOG_INFO,    "info"   },
	{ LOG_NOTICE,  "notice" },
	{ LOG_WARNING, "warn"   },
	{ LOG_ERR,     "error"  }
};
std::map<std::string, unsigned int> Settings::string2SyslogFacility =
{
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
std::map<unsigned int, std::string> Settings::syslogFacility2String =
{
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

void Settings::SetConfiguration(int argc, char* argv[])
{
	MS_TRACE();

	/* Set default configuration. */

	SetDefaultRtcListenIP(AF_INET);
	SetDefaultRtcListenIP(AF_INET6);

	/* Variables for getopt. */

	extern char *optarg;
	extern int optind, opterr, optopt;
	int c;
	int option_index = 0;
	std::string value_string;

	struct option options[] =
	{
		{ "logLevel",            optional_argument, nullptr, 'l' },
		{ "syslogFacility",      optional_argument, nullptr, 'f' },
		{ "rtcListenIPv4",       optional_argument, nullptr, '4' },
		{ "rtcListenIPv6",       optional_argument, nullptr, '6' },
		{ "rtcMinPort",          optional_argument, nullptr, 'm' },
		{ "rtcMaxPort",          optional_argument, nullptr, 'M' },
		{ "dtlsCertificateFile", optional_argument, nullptr, 'c' },
		{ "dtlsPrivateKeyFile",  optional_argument, nullptr, 'p' },
		{ 0, 0, 0, 0 }
	};

	/* Parse command line options. */

	opterr = 0;  // Don't allow getopt to print error messages.
	while ((c = getopt_long_only(argc, argv, "", options, &option_index)) != -1)
	{
		if (!optarg)
			MS_THROW_ERROR("parameters without value not allowed");

		switch (c)
		{
			case 'l':
				value_string = std::string(optarg);
				SetLogLevel(value_string);
				break;

			case 'f':
				value_string = std::string(optarg);
				SetSyslogFacility(value_string);
				break;

			case '4':
				value_string = std::string(optarg);
				SetRtcListenIPv4(value_string);
				break;

			case '6':
				value_string = std::string(optarg);
				SetRtcListenIPv6(value_string);
				break;

			case 'm':
				Settings::configuration.RTC.minPort = std::stoi(optarg);
				break;

			case 'M':
				Settings::configuration.RTC.maxPort = std::stoi(optarg);
				break;

			case 'c':
				value_string = std::string(optarg);
				Settings::configuration.RTC.dtlsCertificateFile = value_string;
				break;

			case 'p':
				value_string = std::string(optarg);
				Settings::configuration.RTC.dtlsPrivateKeyFile = value_string;
				break;

			// Invalid option.
			case '?':
				if (isprint(optopt))
					MS_THROW_ERROR("invalid option '-%c'", (char)optopt);
				else
					MS_THROW_ERROR("unknown long option given as argument");

			// Valid option, but it requires and argument that is not given.
			case ':':
				MS_THROW_ERROR("option '%c' requires an argument", (char)optopt);

			// This should never happen.
			default:
				MS_THROW_ERROR("'default' should never happen");
		}
	}

	// Ensure there are no more command line arguments after parsed options.
	if (optind != argc)
		MS_THROW_ERROR("there are remaining arguments after parsing command line options");

	/* Post configuration. */

	// RTC must have at least 'listenIPv4' or 'listenIPv6'.
	if (!Settings::configuration.RTC.hasIPv4 && !Settings::configuration.RTC.hasIPv6)
		MS_THROW_ERROR("at least RTC.listenIPv4 or RTC.listenIPv6 must be enabled");

	// Validate RTC ports.
	Settings::SetRtcPorts();

	// Set DTLS certificate files (if provided),
	Settings::SetDtlsCertificateAndPrivateKeyFiles();
}

void Settings::PrintConfiguration()
{
	MS_TRACE();

	MS_DEBUG("[configuration]");

	MS_DEBUG("- logLevel: \"%s\"", Settings::logLevel2String[Settings::configuration.logLevel].c_str());
	MS_DEBUG("- syslogFacility: \"%s\"", Settings::syslogFacility2String[Settings::configuration.syslogFacility].c_str());

	MS_DEBUG("- RTC:");
	if (Settings::configuration.RTC.hasIPv4)
		MS_DEBUG("  - listenIPv4: \"%s\"", Settings::configuration.RTC.listenIPv4.c_str());
	else
		MS_DEBUG("  - listenIPv4: (unavailable)");
	if (Settings::configuration.RTC.hasIPv6)
		MS_DEBUG("  - listenIPv6: \"%s\"", Settings::configuration.RTC.listenIPv6.c_str());
	else
		MS_DEBUG("  - listenIPv6: (unavailable)");
	MS_DEBUG("  - minPort: %d", Settings::configuration.RTC.minPort);
	MS_DEBUG("  - maxPort: %d", Settings::configuration.RTC.maxPort);
	if (!Settings::configuration.RTC.dtlsCertificateFile.empty())
	{
		MS_DEBUG("  - dtlsCertificateFile: \"%s\"", Settings::configuration.RTC.dtlsCertificateFile.c_str());
		MS_DEBUG("  - dtlsPrivateKeyFile: \"%s\"", Settings::configuration.RTC.dtlsPrivateKeyFile.c_str());
	}

	MS_DEBUG("[/configuration]");
}

void Settings::SetDefaultRtcListenIP(int requested_family)
{
	MS_TRACE();

	int err;
	uv_interface_address_t* addresses;
	int num_addresses;
	std::string ipv4;
	std::string ipv6;
	int bind_errno;

	err = uv_interface_addresses(&addresses, &num_addresses);
	if (err)
		MS_ABORT("uv_interface_addresses() failed: %s", uv_strerror(err));

	for (int i = 0; i < num_addresses; i++)
	{
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

		switch (family)
		{
			case AF_INET:
				// Ignore if already got an IPv4.
				if (!ipv4.empty())
					continue;

				// Check if it is bindable.
				if (!IsBindableIP(ip, AF_INET, &bind_errno))
					continue;

				ipv4 = ip;
				break;

			case AF_INET6:
				// Ignore if already got an IPv6.
				if (!ipv6.empty())
					continue;

				// Check if it is bindable.
				if (!IsBindableIP(ip, AF_INET6, &bind_errno))
					continue;

				ipv6 = ip;
				break;
		}
	}

	if (!ipv4.empty())
	{
		Settings::configuration.RTC.listenIPv4 = ipv4;
		Settings::configuration.RTC.hasIPv4 = true;
	}

	if (!ipv6.empty())
	{
		Settings::configuration.RTC.listenIPv6 = ipv6;
		Settings::configuration.RTC.hasIPv6 = true;
	}

	uv_free_interface_addresses(addresses, num_addresses);
}

void Settings::SetLogLevel(std::string &level)
{
	MS_TRACE();

	// Downcase given level.
	std::transform(level.begin(), level.end(), level.begin(), ::tolower);

	if (Settings::string2LogLevel.find(level) == Settings::string2LogLevel.end())
		MS_THROW_ERROR("invalid value '%s' for Logging.level", level.c_str());

	Settings::configuration.logLevel = Settings::string2LogLevel[level];
}

void Settings::SetSyslogFacility(std::string &facility)
{
	MS_TRACE();

	// Downcase given facility.
	std::transform(facility.begin(), facility.end(), facility.begin(), ::tolower);

	if (Settings::string2SyslogFacility.find(facility) == Settings::string2SyslogFacility.end())
		MS_THROW_ERROR("invalid value '%s' for Logging.syslogFacility", facility.c_str());

	Settings::configuration.syslogFacility = Settings::string2SyslogFacility[facility];
}

void Settings::SetRtcListenIPv4(const std::string &ip)
{
	MS_TRACE();

	if (ip.empty())
	{
		Settings::configuration.RTC.listenIPv4.clear();
		Settings::configuration.RTC.hasIPv4 = false;
		return;
	}

	switch (Utils::IP::GetFamily(ip))
	{
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

	int bind_errno;
	if (!IsBindableIP(ip, AF_INET, &bind_errno))
		MS_THROW_ERROR("cannot bind on '%s' for RTC.listenIPv4: %s", ip.c_str(), std::strerror(bind_errno));
}

void Settings::SetRtcListenIPv6(const std::string &ip)
{
	MS_TRACE();

	if (ip.empty())
	{
		Settings::configuration.RTC.listenIPv6.clear();
		Settings::configuration.RTC.hasIPv6 = false;
		return;
	}

	switch (Utils::IP::GetFamily(ip))
	{
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

	int bind_errno;
	if (!IsBindableIP(ip, AF_INET6, &bind_errno))
		MS_THROW_ERROR("cannot bind on '%s' for RTC.listenIPv6: %s", ip.c_str(), std::strerror(bind_errno));
}

void Settings::SetRtcPorts()
{
	MS_TRACE();

	MS_PORT minPort = Settings::configuration.RTC.minPort;
	MS_PORT maxPort = Settings::configuration.RTC.maxPort;

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

void Settings::SetDtlsCertificateAndPrivateKeyFiles()
{
	MS_TRACE();

	if (Settings::configuration.RTC.dtlsCertificateFile.empty() || Settings::configuration.RTC.dtlsPrivateKeyFile.empty())
	{
		Settings::configuration.RTC.dtlsCertificateFile = "";
		Settings::configuration.RTC.dtlsPrivateKeyFile = "";
		return;
	}

	std::string dtlsCertificateFile = Settings::configuration.RTC.dtlsCertificateFile;
	std::string dtlsPrivateKeyFile = Settings::configuration.RTC.dtlsPrivateKeyFile;

	try
	{
		Utils::File::CheckFile(dtlsCertificateFile.c_str());
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("RTC.dtlsCertificateFile: %s", error.what());
	}

	try
	{
		Utils::File::CheckFile(dtlsPrivateKeyFile.c_str());
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("RTC.dtlsPrivateKeyFile: %s", error.what());
	}

	Settings::configuration.RTC.dtlsCertificateFile = dtlsCertificateFile;
	Settings::configuration.RTC.dtlsPrivateKeyFile = dtlsPrivateKeyFile;
}

/* Helpers. */

bool IsBindableIP(const std::string &ip, int family, int* bind_errno)
{
	MS_TRACE();

	struct sockaddr_storage bind_addr;
	int bind_socket;
	int err = 0;
	bool success;

	switch (family)
	{
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

	if (err == 0)
	{
		success = true;
	}
	else
	{
		success = false;
		*bind_errno = errno;
	}

	err = close(bind_socket);
	if (err)
		MS_ABORT("close() failed: %s", std::strerror(errno));

	return success;
}
