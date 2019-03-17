#define MS_CLASS "RTC::TransportTuple"
// #define MS_LOG_DEV

#include "RTC/TransportTuple.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
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
} // namespace RTC
