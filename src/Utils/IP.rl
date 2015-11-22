#define MS_CLASS "Utils::IP"

#include "Utils.h"
#include "Logger.h"
#include <uv.h>

namespace Utils
{
	int IP::GetFamily(const char *ip, size_t ip_len)
	{
		MS_TRACE();

		int ip_family = 0;

		/**
		 * Ragel: machine definition
		 */
		%%{
			machine IPParser;

			alphtype unsigned char;

			action on_ipv4
			{
				ip_family = AF_INET;
			}

			action on_ipv6
			{
				ip_family = AF_INET6;
			}

			include core_grammar "../core_grammar.rl";

			main := IPv4address @on_ipv4 |
			        IPv6address @on_ipv6;
		}%%


		/**
		 * Ragel: %%write data
		 * This generates Ragel's static variables.
		 */
		%%write data;

		// Used by Ragel:
		size_t cs;
		const unsigned char* p;
		const unsigned char* pe;

		p = (const unsigned char*)ip;
		pe = p + ip_len;

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
		if (ip_len == (size_t)(p - (const unsigned char*)ip))
			return ip_family;
		else
			return AF_UNSPEC;
	}

	void IP::GetAddressInfo(const struct sockaddr* addr, int* family, std::string &ip, MS_PORT* port)
	{
		MS_TRACE();

		char _ip[INET6_ADDRSTRLEN+1];
		int err;

		switch (addr->sa_family)
		{
			case AF_INET:
				err = uv_inet_ntop(AF_INET, &((struct sockaddr_in*)addr)->sin_addr, _ip, INET_ADDRSTRLEN);
				if (err)
					MS_ABORT("uv_inet_ntop() failed: %s", uv_strerror(err));
				*port = (MS_PORT)ntohs(((struct sockaddr_in*)addr)->sin_port);
				break;
			case AF_INET6:
				err = uv_inet_ntop(AF_INET6, &((struct sockaddr_in6*)addr)->sin6_addr, _ip, INET6_ADDRSTRLEN);
				if (err)
					MS_ABORT("uv_inet_ntop() failed: %s", uv_strerror(err));
				*port = (MS_PORT)ntohs(((struct sockaddr_in6*)addr)->sin6_port);
				break;
			default:
				MS_ABORT("unknown network family: %d", (int)addr->sa_family);
		}

		*family = addr->sa_family;
		ip.assign(_ip);
	}
}  // namespace Utils
