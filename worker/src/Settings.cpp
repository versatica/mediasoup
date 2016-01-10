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
}

/* Helpers declaration. */

static
bool IsBindableIP(const std::string &ip, int family, int* _bind_err);

/* Class variables. */

struct Settings::Configuration Settings::configuration;
std::map<std::string, uint8_t> Settings::string2LogLevel =
{
	{ "debug", MS_LOG_LEVEL_DEBUG },
	{ "warn",  MS_LOG_LEVEL_WARN  },
	{ "error", MS_LOG_LEVEL_ERROR }
};
std::map<uint8_t, std::string> Settings::logLevel2String =
{
	{ MS_LOG_LEVEL_DEBUG, "debug" },
	{ MS_LOG_LEVEL_WARN,  "warn"  },
	{ MS_LOG_LEVEL_ERROR, "error" }
};

/* Class methods. */

void Settings::SetConfiguration(int argc, char* argv[])
{
	MS_TRACE();

	/* Set default configuration. */

	SetDefaultRtcListenIP(AF_INET);
	SetDefaultRtcListenIP(AF_INET6);

	/* Variables for getopt. */

	extern char *optarg;
	extern int opterr, optopt;
	int c;
	int option_index = 0;
	std::string value_string;

	struct option options[] =
	{
		{ "logLevel",            optional_argument, nullptr, 'l' },
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

			case '4':
				value_string = std::string(optarg);
				SetRtcListenIPv4(value_string);
				break;

			case '6':
				value_string = std::string(optarg);
				SetRtcListenIPv6(value_string);
				break;

			case 'm':
				Settings::configuration.rtcMinPort = std::stoi(optarg);
				break;

			case 'M':
				Settings::configuration.rtcMaxPort = std::stoi(optarg);
				break;

			case 'c':
				value_string = std::string(optarg);
				Settings::configuration.dtlsCertificateFile = value_string;
				break;

			case 'p':
				value_string = std::string(optarg);
				Settings::configuration.dtlsPrivateKeyFile = value_string;
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

	/* Post configuration. */

	// RTC must have at least 'listenIPv4' or 'listenIPv6'.
	if (!Settings::configuration.hasIPv4 && !Settings::configuration.hasIPv6)
		MS_THROW_ERROR("at least rtcListenIPv4 or rtcListenIPv6 must be enabled");

	// Validate RTC ports.
	Settings::SetRtcPorts();

	// Set DTLS certificate files (if provided),
	Settings::SetDtlsCertificateAndPrivateKeyFiles();
}

void Settings::PrintConfiguration()
{
	MS_TRACE();

	MS_DEBUG("<configuration>");

	MS_DEBUG("logLevel: \"%s\"", Settings::logLevel2String[Settings::configuration.logLevel].c_str());
	if (Settings::configuration.hasIPv4)
		MS_DEBUG("rtcListenIPv4: \"%s\"", Settings::configuration.rtcListenIPv4.c_str());
	else
		MS_DEBUG("rtcListenIPv4: (unavailable)");
	if (Settings::configuration.hasIPv6)
		MS_DEBUG("rtcListenIPv6: \"%s\"", Settings::configuration.rtcListenIPv6.c_str());
	else
		MS_DEBUG("rtcListenIPv6: (unavailable)");
	MS_DEBUG("rtcMinPort: %" PRIu16, Settings::configuration.rtcMinPort);
	MS_DEBUG("rtcMaxPort: %" PRIu16, Settings::configuration.rtcMaxPort);
	if (!Settings::configuration.dtlsCertificateFile.empty())
	{
		MS_DEBUG("dtlsCertificateFile: \"%s\"", Settings::configuration.dtlsCertificateFile.c_str());
		MS_DEBUG("dtlsPrivateKeyFile: \"%s\"", Settings::configuration.dtlsPrivateKeyFile.c_str());
	}

	MS_DEBUG("</configuration>");
}

void Settings::HandleRequest(Channel::Request* request)
{
	MS_TRACE();

	switch (request->methodId)
	{
		case Channel::Request::MethodId::worker_updateSettings:
		{
			static const Json::StaticString k_logLevel("logLevel");

			Json::Value jsonLogLevel = request->data[k_logLevel];

			try
			{
				// Update logLevel if requested.
				if (jsonLogLevel.isString())
				{
					std::string logLevel = jsonLogLevel.asString();

					Settings::SetLogLevel(logLevel);
				}
			}
			catch (const MediaSoupError &error)
			{
				request->Reject(500, error.what());
				return;
			}

			MS_DEBUG("updated settings:");

			// Print configuration.
			Settings::PrintConfiguration();

			request->Accept();

			break;
		}

		default:
		{
			MS_ABORT("unknown method");
		}
	}
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
		Settings::configuration.rtcListenIPv4 = ipv4;
		Settings::configuration.hasIPv4 = true;
	}

	if (!ipv6.empty())
	{
		Settings::configuration.rtcListenIPv6 = ipv6;
		Settings::configuration.hasIPv6 = true;
	}

	uv_free_interface_addresses(addresses, num_addresses);
}

void Settings::SetLogLevel(std::string &level)
{
	MS_TRACE();

	// Downcase given level.
	std::transform(level.begin(), level.end(), level.begin(), ::tolower);

	if (Settings::string2LogLevel.find(level) == Settings::string2LogLevel.end())
		MS_THROW_ERROR("invalid value '%s' for logLevel", level.c_str());

	Settings::configuration.logLevel = Settings::string2LogLevel[level];
}

void Settings::SetRtcListenIPv4(const std::string &ip)
{
	MS_TRACE();

	if (ip.compare("true") == 0)
		return;

	if (ip.empty() || ip.compare("false") == 0)
	{
		Settings::configuration.rtcListenIPv4.clear();
		Settings::configuration.hasIPv4 = false;
		return;
	}

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET:
			if (ip == "0.0.0.0")
				MS_THROW_ERROR("rtcListenIPv4 cannot be '0.0.0.0'");
			Settings::configuration.rtcListenIPv4 = ip;
			Settings::configuration.hasIPv4 = true;
			break;
		case AF_INET6:
			MS_THROW_ERROR("invalid IPv6 '%s' for rtcListenIPv4", ip.c_str());
		default:
			MS_THROW_ERROR("invalid value '%s' for rtcListenIPv4", ip.c_str());
	}

	int bind_errno;
	if (!IsBindableIP(ip, AF_INET, &bind_errno))
		MS_THROW_ERROR("cannot bind on '%s' for rtcListenIPv4: %s", ip.c_str(), std::strerror(bind_errno));
}

void Settings::SetRtcListenIPv6(const std::string &ip)
{
	MS_TRACE();

	if (ip.compare("true") == 0)
		return;

	if (ip.empty() || ip.compare("false") == 0)
	{
		Settings::configuration.rtcListenIPv6.clear();
		Settings::configuration.hasIPv6 = false;
		return;
	}

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET6:
			if (ip == "::")
				MS_THROW_ERROR("rtcListenIPv6 cannot be '::'");
			Settings::configuration.rtcListenIPv6 = ip;
			Settings::configuration.hasIPv6 = true;
			break;
		case AF_INET:
			MS_THROW_ERROR("invalid IPv4 '%s' for rtcListenIPv6", ip.c_str());
		default:
			MS_THROW_ERROR("invalid value '%s' for rtcListenIPv6", ip.c_str());
	}

	int bind_errno;
	if (!IsBindableIP(ip, AF_INET6, &bind_errno))
		MS_THROW_ERROR("cannot bind on '%s' for rtcListenIPv6: %s", ip.c_str(), std::strerror(bind_errno));
}

void Settings::SetRtcPorts()
{
	MS_TRACE();

	MS_PORT minPort = Settings::configuration.rtcMinPort;
	MS_PORT maxPort = Settings::configuration.rtcMaxPort;

	if (minPort < 1024)
		MS_THROW_ERROR("rtcMinPort must be greater or equal than 1024");

	if (maxPort == 0)
		MS_THROW_ERROR("rtcMaxPort can not be 0");

	// Make minPort even.
	minPort &= ~1;

	// Make maxPort odd.
	if (maxPort % 2 == 0)
		maxPort--;

	if ((maxPort - minPort) < 99)
		MS_THROW_ERROR("rtcMaxPort must be at least 99 ports higher than rtcMinPort");

	Settings::configuration.rtcMinPort = minPort;
	Settings::configuration.rtcMaxPort = maxPort;
}

void Settings::SetDtlsCertificateAndPrivateKeyFiles()
{
	MS_TRACE();

	if (Settings::configuration.dtlsCertificateFile.empty() || Settings::configuration.dtlsPrivateKeyFile.empty())
	{
		Settings::configuration.dtlsCertificateFile = "";
		Settings::configuration.dtlsPrivateKeyFile = "";
		return;
	}

	std::string dtlsCertificateFile = Settings::configuration.dtlsCertificateFile;
	std::string dtlsPrivateKeyFile = Settings::configuration.dtlsPrivateKeyFile;

	try
	{
		Utils::File::CheckFile(dtlsCertificateFile.c_str());
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("dtlsCertificateFile: %s", error.what());
	}

	try
	{
		Utils::File::CheckFile(dtlsPrivateKeyFile.c_str());
	}
	catch (const MediaSoupError &error)
	{
		MS_THROW_ERROR("dtlsPrivateKeyFile: %s", error.what());
	}

	Settings::configuration.dtlsCertificateFile = dtlsCertificateFile;
	Settings::configuration.dtlsPrivateKeyFile = dtlsPrivateKeyFile;
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
