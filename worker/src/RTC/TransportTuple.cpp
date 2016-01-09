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

		Json::Value data;
		Json::Value localTuple;
		Json::Value remoteTuple;
		int ip_family;
		std::string ip;
		MS_PORT port;

		Utils::IP::GetAddressInfo(this->GetLocalAddress(), &ip_family, ip, &port);
		localTuple[k_ip] = ip;
		localTuple[k_port] = port;
		if (this->GetProtocol() == RTC::TransportTuple::Protocol::UDP)
			localTuple[k_protocol] = v_udp;
		else
			localTuple[k_protocol] = v_tcp;

		Utils::IP::GetAddressInfo(this->GetRemoteAddress(), &ip_family, ip, &port);
		remoteTuple[k_ip] = ip;
		remoteTuple[k_port] = port;
		if (this->GetProtocol() == RTC::TransportTuple::Protocol::UDP)
			remoteTuple[k_protocol] = v_udp;
		else
			remoteTuple[k_protocol] = v_tcp;

		data[k_local] = localTuple;
		data[k_remote] = remoteTuple;

		return data;
	}

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

				MS_DEBUG("[UDP | local: %s : %u | remote: %s : %u]",
					this->udpSocket->GetLocalIP().c_str(), (unsigned int)this->udpSocket->GetLocalPort(),
					remote_ip.c_str(), remote_port);
				break;
			}

			case Protocol::TCP:
			{
				MS_DEBUG("[TCP | local: %s : %u | remote: %s : %u]",
					this->tcpConnection->GetLocalIP().c_str(), (unsigned int)this->tcpConnection->GetLocalPort(),
					this->tcpConnection->GetPeerIP().c_str(), (unsigned int)this->tcpConnection->GetPeerPort());
				break;
			}
		}
	}
}
