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

		static const Json::StaticString k_localIP("localIP");
		static const Json::StaticString k_remoteIP("remoteIP");
		static const Json::StaticString k_localPort("localPort");
		static const Json::StaticString k_remotePort("remotePort");
		static const Json::StaticString k_protocol("protocol");
		static const Json::StaticString v_udp("udp");
		static const Json::StaticString v_tcp("tcp");

		Json::Value json(Json::objectValue);
		int ip_family;
		std::string ip;
		uint16_t port;

		Utils::IP::GetAddressInfo(this->GetLocalAddress(), &ip_family, ip, &port);
		json[k_localIP] = ip;
		json[k_localPort] = (Json::UInt)port;
		if (this->GetProtocol() == RTC::TransportTuple::Protocol::UDP)
			json[k_protocol] = v_udp;
		else
			json[k_protocol] = v_tcp;

		Utils::IP::GetAddressInfo(this->GetRemoteAddress(), &ip_family, ip, &port);
		json[k_remoteIP] = ip;
		json[k_remotePort] = (Json::UInt)port;

		return json;
	}

	void TransportTuple::Dump() const
	{
		MS_TRACE();

		switch (this->protocol)
		{
			case Protocol::UDP:
			{
				int remote_family;
				std::string remote_ip;
				uint16_t remote_port;

				Utils::IP::GetAddressInfo(GetRemoteAddress(), &remote_family, remote_ip, &remote_port);

				MS_DUMP("<TransportTuple>");
				MS_DUMP("  [UDP, local:%s :%" PRIu16 ", remote:%s :%" PRIu16 "]",
					this->udpSocket->GetLocalIP().c_str(), this->udpSocket->GetLocalPort(),
					remote_ip.c_str(), remote_port);
				MS_DUMP("</TransportTuple>");
				break;
			}

			case Protocol::TCP:
			{
				MS_DUMP("<TransportTuple>");
				MS_DUMP("  [TCP, local:%s :%" PRIu16 ", remote:%s :%" PRIu16 "]",
					this->tcpConnection->GetLocalIP().c_str(), this->tcpConnection->GetLocalPort(),
					this->tcpConnection->GetPeerIP().c_str(), this->tcpConnection->GetPeerPort());
				MS_DUMP("</TransportTuple>");
				break;
			}
		}
	}
}
