#ifndef MS_RTC_ICE_CANDIDATE_HPP
#define MS_RTC_ICE_CANDIDATE_HPP

#include "common.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/UdpSocket.hpp"
#include <json/json.h>
#include <string>

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
		IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority);

		Json::Value ToJson() const;

	private:
		// Others.
		std::string foundation;
		uint32_t priority{ 0 };
		int family{ 0 };
		std::string ip;
		Protocol protocol;
		uint16_t port{ 0 };
		CandidateType type;
		TcpCandidateType tcpType{ TcpCandidateType::PASSIVE };
	};
} // namespace RTC

#endif
