#ifndef MS_RTC_ICE_CANDIDATE_HPP
#define MS_RTC_ICE_CANDIDATE_HPP

#include "common.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/UdpSocket.hpp"
#include <json.hpp>
#include <string>

using json = nlohmann::json;

namespace RTC
{
	class IceCandidate
	{
	public:
		enum class Protocol
		{
			UDP = 1,
			TCP
		};

	public:
		enum class CandidateType
		{
			HOST = 1
		};

	public:
		enum class TcpCandidateType
		{
			PASSIVE = 1
		};

	public:
		IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority);
		IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority, std::string& announcedIp);
		IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority);
		IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority, std::string& announcedIp);

		void FillJson(json& jsonObject) const;

	private:
		// Others.
		std::string foundation;
		uint32_t priority{ 0 };
		std::string ip;
		Protocol protocol;
		uint16_t port{ 0 };
		CandidateType type{ CandidateType::HOST };
		TcpCandidateType tcpType{ TcpCandidateType::PASSIVE };
	};

	/* Inline methods. */

	inline IceCandidate::IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority)
	  : foundation("udpcandidate"), priority(priority), ip(udpSocket->GetLocalIp()),
	    protocol(Protocol::UDP), port(udpSocket->GetLocalPort()), type(CandidateType::HOST)
	{
	}

	inline IceCandidate::IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority, std::string& announcedIp)
	  : foundation("udpcandidate"), priority(priority), ip(announcedIp), protocol(Protocol::UDP),
	    port(udpSocket->GetLocalPort()), type(CandidateType::HOST)
	{
	}

	inline IceCandidate::IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority)
	  : foundation("tcpcandidate"), priority(priority), ip(tcpServer->GetLocalIp()),
	    protocol(Protocol::TCP), port(tcpServer->GetLocalPort()), type(CandidateType::HOST),
	    tcpType(TcpCandidateType::PASSIVE)
	{
	}

	inline IceCandidate::IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority, std::string& announcedIp)
	  : foundation("tcpcandidate"), priority(priority), ip(announcedIp), protocol(Protocol::TCP),
	    port(tcpServer->GetLocalPort()), type(CandidateType::HOST), tcpType(TcpCandidateType::PASSIVE)
	{
	}
} // namespace RTC

#endif
