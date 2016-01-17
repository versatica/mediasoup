#define MS_CLASS "RTC::TransportTuple"

#include "RTC/TransportTuple.h"
#include "Logger.h"
#include "Utils.h"
#include <string>

namespace RTC
{
	/* Instance methods. */

	Json::Value TransportTuple::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_local("local");
		static const Json::StaticString k_remote("remote");
		static const Json::StaticString k_ip("ip");
		static const Json::StaticString k_port("port");
		static const Json::StaticString k_protocol("protocol");
		static const Json::StaticString v_udp("udp");
		static const Json::StaticString v_tcp("tcp");

		Json::Value json;
		Json::Value json_local;
		Json::Value json_remote;
		int ip_family;
		std::string ip;
		MS_PORT port;

		Utils::IP::GetAddressInfo(this->GetLocalAddress(), &ip_family, ip, &port);
		json_local[k_ip] = ip;
		json_local[k_port] = port;
		if (this->GetProtocol() == RTC::TransportTuple::Protocol::UDP)
			json_local[k_protocol] = v_udp;
		else
			json_local[k_protocol] = v_tcp;

		Utils::IP::GetAddressInfo(this->GetRemoteAddress(), &ip_family, ip, &port);
		json_remote[k_ip] = ip;
		json_remote[k_port] = port;
		if (this->GetProtocol() == RTC::TransportTuple::Protocol::UDP)
			json_remote[k_protocol] = v_udp;
		else
			json_remote[k_protocol] = v_tcp;

		json[k_local] = json_local;
		json[k_remote] = json_remote;

		return json;
	}

	// TODO: TMP
	void TransportTuple::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		switch (this->protocol)
		{
			case Protocol::UDP:
			{
				int remote_family;
				std::string remote_ip;
				MS_PORT remote_port;

				Utils::IP::GetAddressInfo(GetRemoteAddress(), &remote_family, remote_ip, &remote_port);

				MS_DEBUG("[UDP, local:%s :%" PRIu16 ", remote:%s :%" PRIu16 "]",
					this->udpSocket->GetLocalIP().c_str(), this->udpSocket->GetLocalPort(),
					remote_ip.c_str(), remote_port);
				break;
			}

			case Protocol::TCP:
			{
				MS_DEBUG("[TCP, local:%s :%" PRIu16 ", remote:%s :%" PRIu16 "]",
					this->tcpConnection->GetLocalIP().c_str(), this->tcpConnection->GetLocalPort(),
					this->tcpConnection->GetPeerIP().c_str(), this->tcpConnection->GetPeerPort());
				break;
			}
		}
	}
}
