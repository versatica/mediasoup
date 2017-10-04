#define MS_CLASS "RTC::TransportTuple"
// #define MS_LOG_DEV

#include "RTC/TransportTuple.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <string>

namespace RTC
{
	/* Instance methods. */

	Json::Value TransportTuple::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringLocalIp{ "localIP" };
		static const Json::StaticString JsonStringRemoteIp{ "remoteIP" };
		static const Json::StaticString JsonStringLocalPort{ "localPort" };
		static const Json::StaticString JsonStringRemotePort{ "remotePort" };
		static const Json::StaticString JsonStringProtocol{ "protocol" };
		static const Json::StaticString JsonStringUdp{ "udp" };
		static const Json::StaticString JsonStringTcp{ "tcp" };
		static const Json::StaticString JsonStringRecvBytes{ "recvBytes" };
		static const Json::StaticString JsonStringSentBytes{ "sentBytes" };

		Json::Value json(Json::objectValue);
		int ipFamily;
		std::string ip;
		uint16_t port;

		Utils::IP::GetAddressInfo(this->GetLocalAddress(), &ipFamily, ip, &port);
		json[JsonStringLocalIp]   = ip;
		json[JsonStringLocalPort] = Json::UInt{ port };
		if (this->GetProtocol() == RTC::TransportTuple::Protocol::UDP)
			json[JsonStringProtocol] = JsonStringUdp;
		else
			json[JsonStringProtocol] = JsonStringTcp;

		Utils::IP::GetAddressInfo(this->GetRemoteAddress(), &ipFamily, ip, &port);
		json[JsonStringRemoteIp]   = ip;
		json[JsonStringRemotePort] = Json::UInt{ port };
		json[JsonStringRecvBytes]  = Json::UInt64{ GetRecvBytes() };
		json[JsonStringSentBytes]  = Json::UInt64{ GetSentBytes() };

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
