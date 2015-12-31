#define MS_CLASS "RTC::IceCandidate"

#include "RTC/IceCandidate.h"

namespace RTC
{
	/* Instance methods. */

	IceCandidate::IceCandidate(RTC::UDPSocket* udpSocket, unsigned long priority) :
		foundation("udpcandidate"),
		priority(priority),
		ip(udpSocket->GetLocalIP()),
		protocol(Protocol::UDP),
		port(udpSocket->GetLocalPort()),
		type(CandidateType::HOST)
	{}

	IceCandidate::IceCandidate(RTC::TCPServer* tcpServer, unsigned long priority) :
		foundation("tcpcandidate"),
		priority(priority),
		ip(tcpServer->GetLocalIP()),
		protocol(Protocol::TCP),
		port(tcpServer->GetLocalPort()),
		type(CandidateType::HOST),
		tcpType(TcpCandidateType::PASSIVE)
	{}

	Json::Value IceCandidate::toJson()
	{
		static const Json::StaticString k_foundation("foundation");
		static const Json::StaticString k_priority("priority");
		static const Json::StaticString k_ip("ip");
		static const Json::StaticString k_port("port");
		static const Json::StaticString k_type("type");
		static const Json::StaticString v_host("host");
		static const Json::StaticString k_protocol("protocol");
		static const Json::StaticString v_udp("udp");
		static const Json::StaticString v_tcp("tcp");
		static const Json::StaticString k_tcpType("tcpType");
		static const Json::StaticString v_passive("passive");

		Json::Value data;

		data[k_foundation] = this->foundation;
		data[k_priority] = (unsigned int)this->priority;
		data[k_ip] = this->ip;
		data[k_port] = this->port;

		switch (this->type)
		{
			case CandidateType::HOST:
				data[k_type] = v_host;
				break;
		}

		switch (this->protocol)
		{
			case Protocol::UDP:
				data[k_protocol] = v_udp;
				break;
			case Protocol::TCP:
				data[k_protocol] = v_tcp;
				switch (this->tcpType)
				{
					case TcpCandidateType::PASSIVE:
						data[k_tcpType] = v_passive;
						break;
				}
				break;
		}

		return data;
	}
}
