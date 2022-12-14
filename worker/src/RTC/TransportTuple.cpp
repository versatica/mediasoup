#define MS_CLASS "RTC::TransportTuple"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TransportTuple.hpp"
#include "Logger.hpp"
#include <string>

namespace RTC
{
	/* Instance methods. */

	flatbuffers::Offset<FBS::Transport::Tuple> TransportTuple::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		int family;
		std::string localIp;
		uint16_t localPort;

		Utils::IP::GetAddressInfo(GetLocalAddress(), family, localIp, localPort);

		localIp = this->localAnnouncedIp.empty() ? localIp : this->localAnnouncedIp;

		std::string remoteIp;
		uint16_t remotePort;

		Utils::IP::GetAddressInfo(GetRemoteAddress(), family, remoteIp, remotePort);

		std::string protocol;

		// Add protocol.
		switch (GetProtocol())
		{
			case Protocol::UDP:
				protocol = "udp";
				break;

			case Protocol::TCP:
				protocol = "tcp";
				break;
		}

		return FBS::Transport::CreateTupleDirect(
		  builder, localIp.c_str(), localPort, remoteIp.c_str(), remotePort, protocol.c_str());
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
} // namespace RTC
