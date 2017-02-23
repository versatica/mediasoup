#ifndef MS_RTC_ICE_CANDIDATE_HPP
#define MS_RTC_ICE_CANDIDATE_HPP

#include "common.hpp"
#include "RTC/UdpSocket.hpp"
#include "RTC/TcpServer.hpp"
#include <string>
#include <json/json.h>

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

		Json::Value toJson();

	private:
		// Others.
		std::string foundation;
		uint32_t priority;
		std::string ip;
		Protocol protocol;
		uint16_t port;
		CandidateType type;
		TcpCandidateType tcpType = TcpCandidateType::PASSIVE;
	};
}

#endif
