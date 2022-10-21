#define MS_CLASS "RTC::IceCandidate"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/IceCandidate.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

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
		if (this->protocol == Protocol::TCP)
		{
			switch (this->tcpType)
			{
				case TcpCandidateType::PASSIVE:
					jsonObject["tcpType"] = "passive";
					break;
			}
		}
	}

	flatbuffers::Offset<FBS::Transport::IceCandidate> IceCandidate::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		std::string protocol;

		switch (this->protocol)
		{
			case Protocol::UDP:
				protocol = "udp";
				break;

			case Protocol::TCP:
				protocol = "tcp";
				break;
		}

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

		return FBS::Transport::CreateIceCandidateDirect(
		  builder,
		  // foundation.
		  this->foundation.c_str(),
		  // priority.
		  this->priority,
		  // ip.
		  this->ip.c_str(),
		  // protocol.
		  protocol.c_str(),
		  // port.
		  this->port,
		  // type.
		  type.c_str(),
		  // tcpType.
		  tcpType.c_str());
	}
} // namespace RTC
