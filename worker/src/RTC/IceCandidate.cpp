#define MS_CLASS "RTC::IceCandidate"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/IceCandidate.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	flatbuffers::Offset<FBS::WebRtcTransport::IceCandidate> IceCandidate::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		auto protocol = TransportTuple::ProtocolToFbs(this->protocol);

		std::string type;

		switch (this->type)
		{
			case CandidateType::HOST:
				type = "host";
				break;
		}

		std::string tcpType;

		if (this->protocol == Protocol::TCP)
		{
			switch (this->tcpType)
			{
				case TcpCandidateType::PASSIVE:
					tcpType = "passive";
					break;
			}
		}

		return FBS::WebRtcTransport::CreateIceCandidateDirect(
		  builder,
		  // foundation.
		  this->foundation.c_str(),
		  // priority.
		  this->priority,
		  // ip.
		  this->ip.c_str(),
		  // protocol.
		  protocol,
		  // port.
		  this->port,
		  // type.
		  type.c_str(),
		  // tcpType.
		  tcpType.c_str());
	}
} // namespace RTC
