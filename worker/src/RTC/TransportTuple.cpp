#define MS_CLASS "RTC::TransportTuple"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TransportTuple.hpp"
#include "Logger.hpp"
#include <string>

namespace RTC
{
	/* Instance methods. */

	void TransportTuple::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		int family;
		std::string ip;
		uint16_t port;

		Utils::IP::GetAddressInfo(GetLocalAddress(), family, ip, port);

		// Add localIp.
		if (this->localAnnouncedIp.empty())
			jsonObject["localIp"] = ip;
		else
			jsonObject["localIp"] = this->localAnnouncedIp;

		// Add localPort.
		jsonObject["localPort"] = port;

		Utils::IP::GetAddressInfo(GetRemoteAddress(), family, ip, port);

		// Add remoteIp.
		jsonObject["remoteIp"] = ip;

		// Add remotePort.
		jsonObject["remotePort"] = port;

		// Add protocol.
		switch (GetProtocol())
		{
			case Protocol::UDP:
				jsonObject["protocol"] = "udp";
				break;

			case Protocol::TCP:
				jsonObject["protocol"] = "tcp";
				break;
		}
	}

	void TransportTuple::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<TransportTuple>");

		int family;
		std::string ip;
		uint16_t port;

		Utils::IP::GetAddressInfo(GetLocalAddress(), family, ip, port);

		MS_DUMP("  localIp    : %s", ip.c_str());
		MS_DUMP("  localPort  : %" PRIu16, port);

		Utils::IP::GetAddressInfo(GetRemoteAddress(), family, ip, port);

		MS_DUMP("  remoteIp   : %s", ip.c_str());
		MS_DUMP("  remotePort : %" PRIu16, port);

		switch (GetProtocol())
		{
			case Protocol::UDP:
				MS_DUMP("  protocol   : udp");
				break;

			case Protocol::TCP:
				MS_DUMP("  protocol   : tcp");
				break;
		}

		MS_DUMP("</TransportTuple>");
	}

	// TODO: Move to .hpp once done.
	void TransportTuple::SetHash()
	{
		const struct sockaddr* localSockAddr  = GetLocalAddress();
		const struct sockaddr* remoteSockAddr = GetRemoteAddress();

		switch (localSockAddr->sa_family)
		{
			case AF_INET:
			{
				auto* localSockAddrIn  = reinterpret_cast<const struct sockaddr_in*>(localSockAddr);
				auto* remoteSockAddrIn = reinterpret_cast<const struct sockaddr_in*>(remoteSockAddr);

				this->hash += ntohl(remoteSockAddrIn->sin_addr.s_addr);
				// TODO: I'd like that port (8 bits) is most significant bits from 1..8
				// (bit 0 is for protocol, see below).
				this->hash += ntohs(remoteSockAddrIn->sin_port);

				break;
			}

			case AF_INET6:
			{
				auto* localSockAddrIn6  = reinterpret_cast<const struct sockaddr_in6*>(localSockAddr);
				auto* remoteSockAddrIn6 = reinterpret_cast<const struct sockaddr_in6*>(remoteSockAddr);
				auto* a = reinterpret_cast<const uint32_t*>(std::addressof(remoteSockAddrIn6->sin6_addr));

				this->hash += a[0] ^ a[1] ^ a[2] ^ a[3];
				// TODO: I'd like that port (8 bits) is most significant bits from 1..8
				// (bit 0 is for protocol, see below).
				this->hash += ntohs(remoteSockAddrIn6->sin6_port);

				break;
			}
		}

		MS_ERROR("--- temp hash  : %" PRIu64, this->hash);

		// Override most significant bit with protocol information:
		// - If UDP, start with 0.
		// - If TCP, start with 1.
		if (this->protocol == Protocol::UDP)
		{
			this->hash &= 0b0111111111111111111111111111111101111111111111111111111111111111;
		}
		else
		{
			this->hash |= 0b1000000000000000000000000000000000000000000000000000000000000000;
		}

		MS_ERROR("--- final hash : %" PRIu64, this->hash);
	}
} // namespace RTC
