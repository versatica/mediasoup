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
		Json::Value data;

		data["foundation"] = this->foundation;
		data["priority"] = (unsigned int)this->priority;
		data["ip"] = this->ip;
		data["port"] = this->port;

		switch (this->type)
		{
			case CandidateType::HOST:
				data["type"] = "host";
				break;
		}

		switch (this->protocol)
		{
			case Protocol::UDP:
				data["protocol"] = "udp";
				break;
			case Protocol::TCP:
				data["protocol"] = "tcp";
				switch (this->tcpType)
				{
					case TcpCandidateType::PASSIVE:
						data["tcpType"] = "passive";
						break;
				}
				break;
		}

		return data;
	}
}
