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
	    protocol(Protocol::TCP), port(tcpServer->GetLocalPort()), type(CandidateType::HOST)
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

	Json::Value IceCandidate::ToJson() const
	{
		static const Json::StaticString JsonStringFoundation{ "foundation" };
		static const Json::StaticString JsonStringPriority{ "priority" };
		static const Json::StaticString JsonStringFamily{ "family" };
		static const Json::StaticString JsonStringIpv4{ "ipv4" };
		static const Json::StaticString JsonStringIpv6{ "ipv6" };
		static const Json::StaticString JsonStringIp{ "ip" };
		static const Json::StaticString JsonStringPort{ "port" };
		static const Json::StaticString JsonStringType{ "type" };
		static const Json::StaticString JsonStringHost{ "host" };
		static const Json::StaticString JsonStringProtocol{ "protocol" };
		static const Json::StaticString JsonStringUdp{ "udp" };
		static const Json::StaticString JsonStringTcp{ "tcp" };
		static const Json::StaticString JsonStringTcpType{ "tcpType" };
		static const Json::StaticString JsonStringPassive{ "passive" };

		Json::Value json(Json::objectValue);

		json[JsonStringFoundation] = this->foundation;

		switch (this->family)
		{
			case AF_INET:
				json[JsonStringFamily] = JsonStringIpv4;
				break;

			case AF_INET6:
				json[JsonStringFamily] = JsonStringIpv6;
				break;
		}

		json[JsonStringPriority] = Json::UInt{ this->priority };
		json[JsonStringIp]       = this->ip;
		json[JsonStringPort]     = Json::UInt{ this->port };

		switch (this->type)
		{
			case CandidateType::HOST:
				json[JsonStringType] = JsonStringHost;
				break;
		}

		switch (this->protocol)
		{
			case Protocol::UDP:
				json[JsonStringProtocol] = JsonStringUdp;
				break;
			case Protocol::TCP:
				json[JsonStringProtocol] = JsonStringTcp;
				switch (this->tcpType)
				{
					case TcpCandidateType::PASSIVE:
						json[JsonStringTcpType] = JsonStringPassive;
						break;
				}
				break;
		}

		return json;
	}
} // namespace RTC
