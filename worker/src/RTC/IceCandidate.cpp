#define MS_CLASS "RTC::IceCandidate"
// #define MS_LOG_DEV

#include "RTC/IceCandidate.hpp"
#include "Settings.hpp"

namespace RTC
{
	/* Instance methods. */

	IceCandidate::IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority) :
		foundation("udpcandidate"),
		priority(priority),
		family(udpSocket->GetLocalFamily()),
		protocol(Protocol::UDP),
		port(udpSocket->GetLocalPort()),
		type(CandidateType::HOST)
	{
		switch (this->family)
		{
			case AF_INET:
			{
				if (Settings::configuration.hasAnnouncedIPv4)
					this->ip = Settings::configuration.rtcAnnouncedIPv4;
				else
					this->ip = udpSocket->GetLocalIP();

				break;
			}

			case AF_INET6:
			{
				if (Settings::configuration.hasAnnouncedIPv6)
					this->ip = Settings::configuration.rtcAnnouncedIPv6;
				else
					this->ip = udpSocket->GetLocalIP();

				break;
			}
		}
	}

	IceCandidate::IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority) :
		foundation("tcpcandidate"),
		priority(priority),
		family(tcpServer->GetLocalFamily()),
		protocol(Protocol::TCP),
		port(tcpServer->GetLocalPort()),
		type(CandidateType::HOST),
		tcpType(TcpCandidateType::PASSIVE)
	{
		switch (this->family)
		{
			case AF_INET:
			{
				if (Settings::configuration.hasAnnouncedIPv4)
					this->ip = Settings::configuration.rtcAnnouncedIPv4;
				else
					this->ip = tcpServer->GetLocalIP();

				break;
			}

			case AF_INET6:
			{
				if (Settings::configuration.hasAnnouncedIPv6)
					this->ip = Settings::configuration.rtcAnnouncedIPv6;
				else
					this->ip = tcpServer->GetLocalIP();

				break;
			}
		}
	}

	Json::Value IceCandidate::toJson()
	{
		static const Json::StaticString k_foundation("foundation");
		static const Json::StaticString k_priority("priority");
		static const Json::StaticString k_family("family");
		static const Json::StaticString v_ipv4("ipv4");
		static const Json::StaticString v_ipv6("ipv6");
		static const Json::StaticString k_ip("ip");
		static const Json::StaticString k_port("port");
		static const Json::StaticString k_type("type");
		static const Json::StaticString v_host("host");
		static const Json::StaticString k_protocol("protocol");
		static const Json::StaticString v_udp("udp");
		static const Json::StaticString v_tcp("tcp");
		static const Json::StaticString k_tcpType("tcpType");
		static const Json::StaticString v_passive("passive");

		Json::Value json(Json::objectValue);

		json[k_foundation] = this->foundation;

		switch (this->family)
		{
			case AF_INET:
				json[k_family] = v_ipv4;
				break;

			case AF_INET6:
				json[k_family] = v_ipv6;
				break;
		}

		json[k_priority] = (Json::UInt)this->priority;
		json[k_ip] = this->ip;
		json[k_port] = (Json::UInt)this->port;

		switch (this->type)
		{
			case CandidateType::HOST:
				json[k_type] = v_host;
				break;
		}

		switch (this->protocol)
		{
			case Protocol::UDP:
				json[k_protocol] = v_udp;
				break;
			case Protocol::TCP:
				json[k_protocol] = v_tcp;
				switch (this->tcpType)
				{
					case TcpCandidateType::PASSIVE:
						json[k_tcpType] = v_passive;
						break;
				}
				break;
		}

		return json;
	}
}
