#define MS_CLASS "Utils::IP"
// #define MS_LOG_DEV_LEVEL 3

#include "Utils.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <uv.h>

namespace Utils
{
	int IP::GetFamily(const char *ip, size_t ipLen)
	{
		MS_TRACE();

		int ipFamily{ 0 };

		/**
		 * Ragel: machine definition.
		 */
		%%{
			machine IPParser;

			alphtype unsigned char;

			action on_ipv4
			{
				ipFamily = AF_INET;
			}

			action on_ipv6
			{
				ipFamily = AF_INET6;
			}

			include grammar "grammar.rl";

			main := IPv4address @on_ipv4 |
			        IPv6address @on_ipv6;
		}%%

		/**
		 * Ragel: %%write data
		 * This generates Ragel's static variables.
		 */
		%%write data;

		// Used by Ragel.
		size_t cs;
		const unsigned char* p;
		const unsigned char* pe;

		p = (const unsigned char*)ip;
		pe = p + ipLen;

		/**
		 * Ragel: %%write init
		 */
		%%write init;

		/**
		 * Ragel: %%write exec
		 * This updates cs variable needed by Ragel.
		 */
		%%write exec;

		// Ensure that the parsing has consumed all the given length.
		if (ipLen == static_cast<size_t>(p - (const unsigned char*)ip))
			return ipFamily;
		else
			return AF_UNSPEC;
	}

	void IP::GetAddressInfo(const struct sockaddr* addr, int& family, std::string& ip, uint16_t& port)
	{
		MS_TRACE();

		char ipBuffer[INET6_ADDRSTRLEN+1];
		int err;

		switch (addr->sa_family)
		{
			case AF_INET:
			{
				err = uv_inet_ntop(
					AF_INET, std::addressof(reinterpret_cast<const struct sockaddr_in*>(addr)->sin_addr), ipBuffer, INET_ADDRSTRLEN);

				if (err)
					MS_ABORT("uv_inet_ntop() failed: %s", uv_strerror(err));

				port = static_cast<uint16_t>(ntohs(reinterpret_cast<const struct sockaddr_in*>(addr)->sin_port));

				break;
			}

			case AF_INET6:
			{
				err = uv_inet_ntop(
					AF_INET, std::addressof(reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_addr), ipBuffer, INET6_ADDRSTRLEN);

				if (err)
					MS_ABORT("uv_inet_ntop() failed: %s", uv_strerror(err));

				port = static_cast<uint16_t>(ntohs(reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_port));

				break;
			}

			default:
			{
				MS_ABORT("unknown network family: %d", static_cast<int>(addr->sa_family));
			}
		}

		family = addr->sa_family;
		ip.assign(ipBuffer);
	}

	void IP::NormalizeIp(std::string& ip)
	{
		MS_TRACE();

		static sockaddr_storage addrStorage;
		char ipBuffer[INET6_ADDRSTRLEN+1];
		int err;

		switch (IP::GetFamily(ip))
		{
			case AF_INET:
			{
				err = uv_ip4_addr(
				  ip.c_str(),
				  0,
				  reinterpret_cast<struct sockaddr_in*>(&addrStorage));

				if (err != 0)
					MS_ABORT("uv_ip4_addr() failed: %s", uv_strerror(err));

				err = uv_ip4_name(
					reinterpret_cast<const struct sockaddr_in*>(std::addressof(addrStorage)),
					ipBuffer,
					INET6_ADDRSTRLEN+1);

				if (err != 0)
					MS_ABORT("uv_ipv4_name() failed: %s", uv_strerror(err));

				ip.assign(ipBuffer);

				break;
			}

			case AF_INET6:
			{
				err = uv_ip6_addr(
					ip.c_str(),
					0,
				  reinterpret_cast<struct sockaddr_in6*>(&addrStorage));

				if (err != 0)
					MS_ABORT("uv_ip6_addr() failed: %s", uv_strerror(err));

				err = uv_ip6_name(
					reinterpret_cast<const struct sockaddr_in6*>(std::addressof(addrStorage)),
					ipBuffer,
					INET6_ADDRSTRLEN+1);

				if (err != 0)
					MS_ABORT("uv_ip6_name() failed: %s", uv_strerror(err));

				ip.assign(ipBuffer);

				break;
			}

			default:
			{
				MS_THROW_TYPE_ERROR("invalid ip '%s'", ip.c_str());
			}
		}
	}
} // namespace Utils
