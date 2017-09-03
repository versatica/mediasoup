#define MS_CLASS "Settings"
// #define MS_LOG_DEV

#include "Settings.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include <uv.h>
#include <cctype> // isprint()
#include <cerrno>
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream
#include <unistd.h> // close()
extern "C" {
#include <getopt.h>
}

/* Helpers declaration. */

static bool isBindableIp(const std::string& ip, int family, int* bindErrno);

/* Class variables. */

struct Settings::Configuration Settings::configuration;
// clang-format off
std::map<std::string, LogLevel> Settings::string2LogLevel =
{
	{ "debug", LogLevel::LOG_DEBUG },
	{ "warn",  LogLevel::LOG_WARN  },
	{ "error", LogLevel::LOG_ERROR }
};
std::map<LogLevel, std::string> Settings::logLevel2String =
{
	{ LogLevel::LOG_DEBUG, "debug" },
	{ LogLevel::LOG_WARN,  "warn"  },
	{ LogLevel::LOG_ERROR, "error" }
};
// clang-format on

/* Class methods. */

void Settings::SetConfiguration(int argc, char* argv[])
{
	MS_TRACE();

	/* Set default configuration. */

	SetDefaultRtcIP(AF_INET);
	SetDefaultRtcIP(AF_INET6);

	/* Variables for getopt. */

	int c;
	int optionIdx{ 0 };
	std::string stringValue;
	std::vector<std::string> logTags;
	// clang-format off
	struct option options[] =
	{
		{ "logLevel",            optional_argument, nullptr, 'l' },
		{ "logTag",              optional_argument, nullptr, 't' },
		{ "rtcIPv4",             optional_argument, nullptr, '4' },
		{ "rtcIPv6",             optional_argument, nullptr, '6' },
		{ "rtcAnnouncedIPv4",    optional_argument, nullptr, '5' },
		{ "rtcAnnouncedIPv6",    optional_argument, nullptr, '7' },
		{ "rtcMinPort",          optional_argument, nullptr, 'm' },
		{ "rtcMaxPort",          optional_argument, nullptr, 'M' },
		{ "dtlsCertificateFile", optional_argument, nullptr, 'c' },
		{ "dtlsPrivateKeyFile",  optional_argument, nullptr, 'p' },
		{ nullptr, 0, nullptr, 0 }
	};
	// clang-format on

	/* Parse command line options. */

	opterr = 0; // Don't allow getopt to print error messages.
	while ((c = getopt_long_only(argc, argv, "", options, &optionIdx)) != -1)
	{
		if (optarg == nullptr)
			MS_THROW_ERROR("unknown configuration parameter: %s", optarg);

		switch (c)
		{
			case 'l':
				stringValue = std::string(optarg);
				SetLogLevel(stringValue);
				break;

			case 't':
				stringValue = std::string(optarg);
				logTags.push_back(stringValue);
				break;

			case '4':
				stringValue = std::string(optarg);
				SetRtcIPv4(stringValue);
				break;

			case '6':
				stringValue = std::string(optarg);
				SetRtcIPv6(stringValue);
				break;

			case '5':
				stringValue                              = std::string(optarg);
				Settings::configuration.rtcAnnouncedIPv4 = stringValue;
				Settings::configuration.hasAnnouncedIPv4 = true;
				break;

			case '7':
				stringValue                              = std::string(optarg);
				Settings::configuration.rtcAnnouncedIPv6 = stringValue;
				Settings::configuration.hasAnnouncedIPv6 = true;
				break;

			case 'm':
				Settings::configuration.rtcMinPort = std::stoi(optarg);
				break;

			case 'M':
				Settings::configuration.rtcMaxPort = std::stoi(optarg);
				break;

			case 'c':
				stringValue                                 = std::string(optarg);
				Settings::configuration.dtlsCertificateFile = stringValue;
				break;

			case 'p':
				stringValue                                = std::string(optarg);
				Settings::configuration.dtlsPrivateKeyFile = stringValue;
				break;

			// Invalid option.
			case '?':
				if (isprint(optopt) != 0)
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

	// Set logTags.
	if (!logTags.empty())
		Settings::SetLogTags(logTags);

	// RTC must have at least 'IPv4' or 'IPv6'.
	if (!Settings::configuration.hasIPv4 && !Settings::configuration.hasIPv6)
		MS_THROW_ERROR("at least rtcIPv4 or rtcIPv6 must be enabled");

	// Clean RTP announced IPs if not available.
	if (!Settings::configuration.hasIPv4)
	{
		Settings::configuration.rtcAnnouncedIPv4 = "";
		Settings::configuration.hasAnnouncedIPv4 = false;
	}
	if (!Settings::configuration.hasIPv6)
	{
		Settings::configuration.rtcAnnouncedIPv6 = "";
		Settings::configuration.hasAnnouncedIPv6 = false;
	}

	// Validate RTC ports.
	Settings::SetRtcPorts();

	// Set DTLS certificate files (if provided),
	Settings::SetDtlsCertificateAndPrivateKeyFiles();
}

void Settings::PrintConfiguration()
{
	MS_TRACE();

	std::vector<std::string> logTags;
	std::ostringstream logTagsStream;

	if (Settings::configuration.logTags.info)
		logTags.emplace_back("info");
	if (Settings::configuration.logTags.ice)
		logTags.emplace_back("ice");
	if (Settings::configuration.logTags.dtls)
		logTags.emplace_back("dtls");
	if (Settings::configuration.logTags.rtp)
		logTags.emplace_back("rtp");
	if (Settings::configuration.logTags.srtp)
		logTags.emplace_back("srtp");
	if (Settings::configuration.logTags.rtcp)
		logTags.emplace_back("rtcp");
	if (Settings::configuration.logTags.rbe)
		logTags.emplace_back("rbe");
	if (Settings::configuration.logTags.rtx)
		logTags.emplace_back("rtx");

	if (!logTags.empty())
	{
		std::copy(
		  logTags.begin(), logTags.end() - 1, std::ostream_iterator<std::string>(logTagsStream, ","));
		logTagsStream << logTags.back();
	}

	MS_DEBUG_TAG(info, "<configuration>");

	MS_DEBUG_TAG(
	  info,
	  "  logLevel            : \"%s\"",
	  Settings::logLevel2String[Settings::configuration.logLevel].c_str());
	MS_DEBUG_TAG(info, "  logTags             : \"%s\"", logTagsStream.str().c_str());
	if (Settings::configuration.hasIPv4)
	{
		MS_DEBUG_TAG(info, "  rtcIPv4             : \"%s\"", Settings::configuration.rtcIPv4.c_str());
	}
	else
	{
		MS_DEBUG_TAG(info, "  rtcIPv4             : (unavailable)");
	}
	if (Settings::configuration.hasIPv6)
	{
		MS_DEBUG_TAG(info, "  rtcIPv6             : \"%s\"", Settings::configuration.rtcIPv6.c_str());
	}
	else
	{
		MS_DEBUG_TAG(info, "  rtcIPv6             : (unavailable)");
	}
	if (Settings::configuration.hasAnnouncedIPv4)
	{
		MS_DEBUG_TAG(
		  info, "  rtcAnnouncedIPv4    : \"%s\"", Settings::configuration.rtcAnnouncedIPv4.c_str());
	}
	else
	{
		MS_DEBUG_TAG(info, "  rtcAnnouncedIPv4    : (unset)");
	}
	if (Settings::configuration.hasAnnouncedIPv6)
	{
		MS_DEBUG_TAG(
		  info, "  rtcAnnouncedIPv6    : \"%s\"", Settings::configuration.rtcAnnouncedIPv6.c_str());
	}
	else
	{
		MS_DEBUG_TAG(info, "  rtcAnnouncedIPv6    : (unset)");
	}
	MS_DEBUG_TAG(info, "  rtcMinPort          : %" PRIu16, Settings::configuration.rtcMinPort);
	MS_DEBUG_TAG(info, "  rtcMaxPort          : %" PRIu16, Settings::configuration.rtcMaxPort);
	if (!Settings::configuration.dtlsCertificateFile.empty())
	{
		MS_DEBUG_TAG(
		  info, "  dtlsCertificateFile : \"%s\"", Settings::configuration.dtlsCertificateFile.c_str());
		MS_DEBUG_TAG(
		  info, "  dtlsPrivateKeyFile  : \"%s\"", Settings::configuration.dtlsPrivateKeyFile.c_str());
	}

	MS_DEBUG_TAG(info, "</configuration>");
}

void Settings::HandleRequest(Channel::Request* request)
{
	MS_TRACE();

	switch (request->methodId)
	{
		case Channel::Request::MethodId::WORKER_UPDATE_SETTINGS:
		{
			static const Json::StaticString JsonStringLogLevel{ "logLevel" };
			static const Json::StaticString JsonStringLogTags{ "logTags" };

			Json::Value jsonLogLevel = request->data[JsonStringLogLevel];
			Json::Value jsonLogTags  = request->data[JsonStringLogTags];

			try
			{
				// Update logLevel if requested.
				if (jsonLogLevel.isString())
				{
					std::string logLevel = jsonLogLevel.asString();

					Settings::SetLogLevel(logLevel);
				}

				// Update logTags if requested.
				if (jsonLogTags.isArray())
				{
					Settings::SetLogTags(jsonLogTags);
				}
			}
			catch (const MediaSoupError& error)
			{
				request->Reject(error.what());

				return;
			}

			// Print the new effective configuration.
			Settings::PrintConfiguration();

			request->Accept();

			break;
		}

		default:
		{
			MS_ERROR("unknown method");

			request->Reject("unknown method");
		}
	}
}

void Settings::SetDefaultRtcIP(int requestedFamily)
{
	MS_TRACE();

	int err;
	uv_interface_address_t* addresses;
	int numAddresses;
	std::string ipv4;
	std::string ipv6;
	int bindErrno;

	err = uv_interface_addresses(&addresses, &numAddresses);
	if (err != 0)
		MS_ABORT("uv_interface_addresses() failed: %s", uv_strerror(err));

	for (int i{ 0 }; i < numAddresses; ++i)
	{
		uv_interface_address_t address = addresses[i];

		// Ignore internal addresses.
		if (address.is_internal != 0)
			continue;

		int family;
		uint16_t port;
		std::string ip;

		Utils::IP::GetAddressInfo(
		  reinterpret_cast<struct sockaddr*>(&address.address.address4), &family, ip, &port);

		if (family != requestedFamily)
			continue;

		switch (family)
		{
			case AF_INET:
				// Ignore if already got an IPv4.
				if (!ipv4.empty())
					continue;

				// Check if it is bindable.
				if (!isBindableIp(ip, AF_INET, &bindErrno))
					continue;

				ipv4 = ip;
				break;

			case AF_INET6:
				// Ignore if already got an IPv6.
				if (!ipv6.empty())
					continue;

				// Check if it is bindable.
				if (!isBindableIp(ip, AF_INET6, &bindErrno))
					continue;

				ipv6 = ip;
				break;
		}
	}

	if (!ipv4.empty())
	{
		Settings::configuration.rtcIPv4 = ipv4;
		Settings::configuration.hasIPv4 = true;
	}

	if (!ipv6.empty())
	{
		Settings::configuration.rtcIPv6 = ipv6;
		Settings::configuration.hasIPv6 = true;
	}

	uv_free_interface_addresses(addresses, numAddresses);
}

void Settings::SetLogLevel(std::string& level)
{
	MS_TRACE();

	// Lowcase given level.
	Utils::String::ToLowerCase(level);

	if (Settings::string2LogLevel.find(level) == Settings::string2LogLevel.end())
		MS_THROW_ERROR("invalid value '%s' for logLevel", level.c_str());

	Settings::configuration.logLevel = Settings::string2LogLevel[level];
}

void Settings::SetRtcIPv4(const std::string& ip)
{
	MS_TRACE();

	if (ip == "true")
		return;

	if (ip.empty() || ip == "false")
	{
		Settings::configuration.rtcIPv4.clear();
		Settings::configuration.hasIPv4 = false;

		return;
	}

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET:
			if (ip == "0.0.0.0")
				MS_THROW_ERROR("rtcIPv4 cannot be '0.0.0.0'");
			Settings::configuration.rtcIPv4 = ip;
			Settings::configuration.hasIPv4 = true;
			break;
		case AF_INET6:
			MS_THROW_ERROR("invalid IPv6 '%s' for rtcIPv4", ip.c_str());
		default:
			MS_THROW_ERROR("invalid value '%s' for rtcIPv4", ip.c_str());
	}

	int bindErrno;

	if (!isBindableIp(ip, AF_INET, &bindErrno))
		MS_THROW_ERROR("cannot bind on '%s' for rtcIPv4: %s", ip.c_str(), std::strerror(bindErrno));
}

void Settings::SetRtcIPv6(const std::string& ip)
{
	MS_TRACE();

	if (ip == "true")
		return;

	if (ip.empty() || ip == "false")
	{
		Settings::configuration.rtcIPv6.clear();
		Settings::configuration.hasIPv6 = false;

		return;
	}

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET6:
			if (ip == "::")
				MS_THROW_ERROR("rtcIPv6 cannot be '::'");
			Settings::configuration.rtcIPv6 = ip;
			Settings::configuration.hasIPv6 = true;
			break;
		case AF_INET:
			MS_THROW_ERROR("invalid IPv4 '%s' for rtcIPv6", ip.c_str());
		default:
			MS_THROW_ERROR("invalid value '%s' for rtcIPv6", ip.c_str());
	}

	int bindErrno;

	if (!isBindableIp(ip, AF_INET6, &bindErrno))
		MS_THROW_ERROR("cannot bind on '%s' for rtcIPv6: %s", ip.c_str(), std::strerror(bindErrno));
}

void Settings::SetRtcPorts()
{
	MS_TRACE();

	uint16_t minPort = Settings::configuration.rtcMinPort;
	uint16_t maxPort = Settings::configuration.rtcMaxPort;

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

	if (
	  Settings::configuration.dtlsCertificateFile.empty() ||
	  Settings::configuration.dtlsPrivateKeyFile.empty())
	{
		Settings::configuration.dtlsCertificateFile = "";
		Settings::configuration.dtlsPrivateKeyFile  = "";

		return;
	}

	std::string dtlsCertificateFile = Settings::configuration.dtlsCertificateFile;
	std::string dtlsPrivateKeyFile  = Settings::configuration.dtlsPrivateKeyFile;

	try
	{
		Utils::File::CheckFile(dtlsCertificateFile.c_str());
	}
	catch (const MediaSoupError& error)
	{
		MS_THROW_ERROR("dtlsCertificateFile: %s", error.what());
	}

	try
	{
		Utils::File::CheckFile(dtlsPrivateKeyFile.c_str());
	}
	catch (const MediaSoupError& error)
	{
		MS_THROW_ERROR("dtlsPrivateKeyFile: %s", error.what());
	}

	Settings::configuration.dtlsCertificateFile = dtlsCertificateFile;
	Settings::configuration.dtlsPrivateKeyFile  = dtlsPrivateKeyFile;
}

void Settings::SetLogTags(std::vector<std::string>& tags)
{
	MS_TRACE();

	// Reset logTags.
	struct LogTags newLogTags;

	Settings::configuration.logTags = newLogTags;

	for (auto& tag : tags)
	{
		if (tag == "info")
			Settings::configuration.logTags.info = true;
		else if (tag == "ice")
			Settings::configuration.logTags.ice = true;
		else if (tag == "dtls")
			Settings::configuration.logTags.dtls = true;
		else if (tag == "rtp")
			Settings::configuration.logTags.rtp = true;
		else if (tag == "srtp")
			Settings::configuration.logTags.srtp = true;
		else if (tag == "rtcp")
			Settings::configuration.logTags.rtcp = true;
		else if (tag == "rbe")
			Settings::configuration.logTags.rbe = true;
		else if (tag == "rtx")
			Settings::configuration.logTags.rtx = true;
	}
}

void Settings::SetLogTags(Json::Value& json)
{
	MS_TRACE();

	std::vector<std::string> tags;

	for (const auto& entry : json)
	{
		if (entry.isString())
			tags.push_back(entry.asString());
	}

	Settings::SetLogTags(tags);
}

/* Helpers. */

bool isBindableIp(const std::string& ip, int family, int* bindErrno)
{
	MS_TRACE();

	// clang-format off
	struct sockaddr_storage bindAddr{};
	// clang-format on
	int bindSocket;
	int err{ 0 };
	bool success;

	switch (family)
	{
		case AF_INET:
			err = uv_ip4_addr(ip.c_str(), 0, reinterpret_cast<struct sockaddr_in*>(&bindAddr));
			if (err != 0)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));

			bindSocket = socket(AF_INET, SOCK_DGRAM, 0);
			if (bindSocket == -1)
				MS_ABORT("socket() failed: %s", std::strerror(errno));

			err = bind(
			  bindSocket, reinterpret_cast<const struct sockaddr*>(&bindAddr), sizeof(struct sockaddr_in));
			break;

		case AF_INET6:
			uv_ip6_addr(ip.c_str(), 0, reinterpret_cast<struct sockaddr_in6*>(&bindAddr));
			if (err != 0)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));
			bindSocket = socket(AF_INET6, SOCK_DGRAM, 0);
			if (bindSocket == -1)
				MS_ABORT("socket() failed: %s", std::strerror(errno));

			err = bind(
			  bindSocket, reinterpret_cast<const struct sockaddr*>(&bindAddr), sizeof(struct sockaddr_in6));
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
		success    = false;
		*bindErrno = errno;
	}

	err = close(bindSocket);
	if (err != 0)
		MS_ABORT("close() failed: %s", std::strerror(errno));

	return success;
}
