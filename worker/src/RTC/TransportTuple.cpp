#define MS_CLASS "RTC::TransportTuple"
// #define MS_LOG_DEV

#include "RTC/TransportTuple.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <string>

namespace RTC
{
	/* Instance methods. */

	Json::Value TransportTuple::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonString_localIP("localIP");
		static const Json::StaticString JsonString_remoteIP("remoteIP");
		static const Json::StaticString JsonString_localPort("localPort");
		static const Json::StaticString JsonString_remotePort("remotePort");
		static const Json::StaticString JsonString_protocol("protocol");
		static const Json::StaticString JsonString_udp("udp");
		static const Json::StaticString JsonString_tcp("tcp");

		Json::Value json(Json::objectValue);
		int ipFamily;
		std::string ip;
		uint16_t port;

		Utils::IP::GetAddressInfo(this->GetLocalAddress(), &ipFamily, ip, &port);
		json[JsonString_localIP]   = ip;
		json[JsonString_localPort] = (Json::UInt)port;
		if (this->GetProtocol() == RTC::TransportTuple::Protocol::UDP)
			json[JsonString_protocol] = JsonString_udp;
		else
			json[JsonString_protocol] = JsonString_tcp;

		Utils::IP::GetAddressInfo(this->GetRemoteAddress(), &ipFamily, ip, &port);
		json[JsonString_remoteIP]   = ip;
		json[JsonString_remotePort] = (Json::UInt)port;

		return json;
	}

	void TransportTuple::Dump() const
	{
		MS_TRACE();

		switch (this->protocol)
		{
			case Protocol::UDP:
			{
				int remoteFamily;
				std::string remoteIp;
				uint16_t remotePort;

				Utils::IP::GetAddressInfo(GetRemoteAddress(), &remoteFamily, remoteIp, &remotePort);

				MS_DUMP("<TransportTuple>");
				MS_DUMP(
				    "  [UDP, local:%s :%" PRIu16 ", remote:%s :%" PRIu16 "]",
				    this->udpSocket->GetLocalIP().c_str(),
				    this->udpSocket->GetLocalPort(),
				    remoteIp.c_str(),
				    remotePort);
				MS_DUMP("</TransportTuple>");
				break;
			}

			case Protocol::TCP:
			{
				MS_DUMP("<TransportTuple>");
				MS_DUMP(
				    "  [TCP, local:%s :%" PRIu16 ", remote:%s :%" PRIu16 "]",
				    this->tcpConnection->GetLocalIP().c_str(),
				    this->tcpConnection->GetLocalPort(),
				    this->tcpConnection->GetPeerIP().c_str(),
				    this->tcpConnection->GetPeerPort());
				MS_DUMP("</TransportTuple>");
				break;
			}
		}
	}
} // namespace RTC
