#define MS_CLASS "RTC::ICE::IceCandidate"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/ICE/IceCandidate.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace ICE
	{
		/* Class methods. */

		IceCandidate::CandidateType IceCandidate::CandidateTypeFromFbs(
		  FBS::WebRtcTransport::IceCandidateType type)
		{
			MS_TRACE();

			switch (type)
			{
				case FBS::WebRtcTransport::IceCandidateType::HOST:
				{
					return IceCandidate::CandidateType::HOST;
				}

				default:
				{
					MS_ABORT("invalid ICE candidate type [value:%" PRIu8 "]", static_cast<uint8_t>(type));
				}
			}
		}

		FBS::WebRtcTransport::IceCandidateType IceCandidate::CandidateTypeToFbs(
		  IceCandidate::CandidateType type)
		{
			MS_TRACE();

			switch (type)
			{
				case IceCandidate::CandidateType::HOST:
				{
					return FBS::WebRtcTransport::IceCandidateType::HOST;
				}

					NO_DEFAULT();
			}
		}

		IceCandidate::TcpCandidateType IceCandidate::TcpCandidateTypeFromFbs(
		  FBS::WebRtcTransport::IceCandidateTcpType type)
		{
			MS_TRACE();

			switch (type)
			{
				case FBS::WebRtcTransport::IceCandidateTcpType::PASSIVE:
				{
					return IceCandidate::TcpCandidateType::PASSIVE;
				}

				default:
				{
					MS_ABORT("invalid ICE candidate TCP type [value:%" PRIu8 "]", static_cast<uint8_t>(type));
				}
			}
		}

		FBS::WebRtcTransport::IceCandidateTcpType IceCandidate::TcpCandidateTypeToFbs(
		  IceCandidate::TcpCandidateType type)
		{
			MS_TRACE();

			switch (type)
			{
				case IceCandidate::TcpCandidateType::PASSIVE:
				{
					return FBS::WebRtcTransport::IceCandidateTcpType::PASSIVE;
				}

					NO_DEFAULT();
			}
		}

		/* Instance methods. */

		IceCandidate::IceCandidate(RTC::UdpSocket* udpSocket, uint32_t priority)
		  : foundation("udpcandidate"),
		    priority(priority),
		    address(udpSocket->GetLocalIp()),
		    protocol(Protocol::UDP),
		    port(udpSocket->GetLocalPort())
		{
			MS_TRACE();
		}

		IceCandidate::IceCandidate(
		  RTC::UdpSocket* udpSocket, uint32_t priority, const std::string& announcedAddress)
		  : foundation("udpcandidate"),
		    priority(priority),
		    address(announcedAddress),
		    protocol(Protocol::UDP),
		    port(udpSocket->GetLocalPort())
		{
			MS_TRACE();
		}

		IceCandidate::IceCandidate(RTC::TcpServer* tcpServer, uint32_t priority)
		  : foundation("tcpcandidate"),
		    priority(priority),
		    address(tcpServer->GetLocalIp()),
		    protocol(Protocol::TCP),
		    port(tcpServer->GetLocalPort())

		{
			MS_TRACE();
		}

		IceCandidate::IceCandidate(
		  RTC::TcpServer* tcpServer, uint32_t priority, const std::string& announcedAddress)
		  : foundation("tcpcandidate"),
		    priority(priority),
		    address(announcedAddress),
		    protocol(Protocol::TCP),
		    port(tcpServer->GetLocalPort())

		{
			MS_TRACE();
		}

		flatbuffers::Offset<FBS::WebRtcTransport::IceCandidate> IceCandidate::FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const
		{
			MS_TRACE();

			auto protocol = TransportTuple::ProtocolToFbs(this->protocol);
			auto type     = CandidateTypeToFbs(this->type);
			flatbuffers::Optional<FBS::WebRtcTransport::IceCandidateTcpType> tcpType;

			if (this->protocol == Protocol::TCP)
			{
				tcpType.emplace(TcpCandidateTypeToFbs(this->tcpType));
			}

			return FBS::WebRtcTransport::CreateIceCandidateDirect(
			  builder,
			  // foundation.
			  this->foundation.c_str(),
			  // priority.
			  this->priority,
			  // address.
			  this->address.c_str(),
			  // protocol.
			  protocol,
			  // port.
			  this->port,
			  // type.
			  type,
			  // tcpType.
			  tcpType);
		}
	} // namespace ICE
} // namespace RTC
