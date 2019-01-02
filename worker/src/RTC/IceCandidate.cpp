#define MS_CLASS "RTC::IceCandidate"
// #define MS_LOG_DEV

#include "Logger.hpp"
#include "RTC/IceCandidate.hpp"

namespace RTC
{
	/* Instance methods. */

	IceCandidate::IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority)
	  : foundation("udpcandidate"), priority(priority), ip(udpSocket->GetLocalIP()), protocol(Protocol::UDP), port(udpSocket->GetLocalPort()), type(CandidateType::HOST)
	{
		MS_TRACE();
	}

	IceCandidate::IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority, std::string& announcedIp)
	  : foundation("udpcandidate"), priority(priority), ip(announcedIp), protocol(Protocol::UDP), port(udpSocket->GetLocalPort()), type(CandidateType::HOST)
	{
		MS_TRACE();
	}

	IceCandidate::IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority)
	  : foundation("tcpcandidate"), priority(priority), ip(tcpServer->GetLocalIP()), protocol(Protocol::TCP), port(tcpServer->GetLocalPort()), type(CandidateType::HOST), tcpType(TcpCandidateType::PASSIVE)
	{
		MS_TRACE();
	}

	IceCandidate::IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority, std::string& announcedIp)
	  : foundation("tcpcandidate"), priority(priority), ip(announcedIp), protocol(Protocol::TCP), port(tcpServer->GetLocalPort()), type(CandidateType::HOST), tcpType(TcpCandidateType::PASSIVE)
	{
		MS_TRACE();
	}

	void IceCandidate::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add foundation.
		jsonObject["foundation"] = this->foundation;

		// Add priority.
		jsonObject["priority"] = this->priority;

		// Add ip.
		jsonObject["ip"] = this->ip;

		// Add protocol.
		switch (this->protocol)
		{
			case Protocol::UDP:
				jsonObject["protocol"] = "udp";
				break;

			case Protocol::TCP:
				jsonObject["protocol"] = "tcp";
				break;
		}

		// Add port.
		jsonObject["port"] = this->port;

		// Add type.
		switch (this->type)
		{
			case CandidateType::HOST:
				jsonObject["type"] = "host";
				break;
		}

		// Add tcpType.
		switch (this->tcpType)
		{
			case TcpCandidateType::PASSIVE:
				jsonObject["tcpType"] = "passive";
				break;

			default:;
		}
	}
} // namespace RTC
