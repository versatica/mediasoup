#define MS_CLASS "RTC::IceCandidate"
// #define MS_LOG_DEV

#include "RTC/IceCandidate.hpp"
#include "Settings.hpp"

namespace RTC
{
	/* Instance methods. */

	IceCandidate::IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority)
	    : foundation("udpcandidate"), priority(priority), family(udpSocket->GetLocalFamily()),
	      protocol(Protocol::UDP), port(udpSocket->GetLocalPort()), type(CandidateType::HOST)
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

	IceCandidate::IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority)
	    : foundation("tcpcandidate"), priority(priority), family(tcpServer->GetLocalFamily()),
	      protocol(Protocol::TCP), port(tcpServer->GetLocalPort()), type(CandidateType::HOST),
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

	Json::Value IceCandidate::toJson() const
	{
		static const Json::StaticString JsonString_foundation("foundation");
		static const Json::StaticString JsonString_priority("priority");
		static const Json::StaticString JsonString_family("family");
		static const Json::StaticString JsonString_ipv4("ipv4");
		static const Json::StaticString JsonString_ipv6("ipv6");
		static const Json::StaticString JsonString_ip("ip");
		static const Json::StaticString JsonString_port("port");
		static const Json::StaticString JsonString_type("type");
		static const Json::StaticString JsonString_host("host");
		static const Json::StaticString JsonString_protocol("protocol");
		static const Json::StaticString JsonString_udp("udp");
		static const Json::StaticString JsonString_tcp("tcp");
		static const Json::StaticString JsonString_tcpType("tcpType");
		static const Json::StaticString JsonString_passive("passive");

		Json::Value json(Json::objectValue);

		json[JsonString_foundation] = this->foundation;

		switch (this->family)
		{
			case AF_INET:
				json[JsonString_family] = JsonString_ipv4;
				break;

			case AF_INET6:
				json[JsonString_family] = JsonString_ipv6;
				break;
		}

		json[JsonString_priority] = (Json::UInt)this->priority;
		json[JsonString_ip]       = this->ip;
		json[JsonString_port]     = (Json::UInt)this->port;

		switch (this->type)
		{
			case CandidateType::HOST:
				json[JsonString_type] = JsonString_host;
				break;
		}

		switch (this->protocol)
		{
			case Protocol::UDP:
				json[JsonString_protocol] = JsonString_udp;
				break;
			case Protocol::TCP:
				json[JsonString_protocol] = JsonString_tcp;
				switch (this->tcpType)
				{
					case TcpCandidateType::PASSIVE:
						json[JsonString_tcpType] = JsonString_passive;
						break;
				}
				break;
		}

		return json;
	}
} // namespace RTC
