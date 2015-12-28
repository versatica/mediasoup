#ifndef MS_RTC_ICE_CANDIDATE_H
#define MS_RTC_ICE_CANDIDATE_H

#include "common.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include <string>
#include <json/json.h>

namespace RTC
{
	class IceCandidate
	{
	public:
		enum class IceComponent
		{
			RTP  = 1,
			RTCP = 2
		};

	public:
		enum class IceProtocol
		{
			UDP = 1,
			TCP
		};

	public:
		enum class IceCandidateType
		{
			HOST = 1
		};

	public:
		enum class IceTcpCandidateType
		{
			PASSIVE = 1
		};

	public:
		IceCandidate(RTC::UDPSocket* udpSocket);
		IceCandidate(RTC::TCPServer* tcpServer);

		Json::Value toJson();

	private:
		// Others.
		std::string foundation;
		unsigned long priority;
		std::string ip;
		IceProtocol protocol;
		MS_PORT port;
		IceCandidateType type;
		IceTcpCandidateType tcpType;
	};

	/* Inline methods. */

	inline
	IceCandidate::IceCandidate(RTC::UDPSocket* udpSocket) :
		foundation("udpcandidate"),
		ip(udpSocket->GetLocalIP()),
		protocol(IceProtocol::UDP),
		port(udpSocket->GetLocalPort()),
		type(IceCandidateType::HOST)
	{}

	inline
	IceCandidate::IceCandidate(RTC::TCPServer* tcpServer) :
		foundation("tcpcandidate"),
		ip(tcpServer->GetLocalIP()),
		protocol(IceProtocol::TCP),
		port(tcpServer->GetLocalPort()),
		type(IceCandidateType::HOST),
		tcpType(IceTcpCandidateType::PASSIVE)
	{}

	inline
	Json::Value IceCandidate::toJson()
	{
		Json::Value data;

		data["foundation"] = this->foundation;
		data["priority"] = (unsigned int)this->priority;
		data["ip"] = this->ip;
		data["port"] = this->port;

		if (this->protocol == IceProtocol::UDP)
			data["protocol"] = "udp";
		else
			data["protocol"] = "tcp";

		if (this->type == IceCandidateType::HOST)
			data["type"] = "host";

		if (this->protocol == IceProtocol::TCP)
		{
			if (this->tcpType == IceTcpCandidateType::PASSIVE)
				data["tcpType"] = "passive";
		}

		return data;
	}
}

#endif
