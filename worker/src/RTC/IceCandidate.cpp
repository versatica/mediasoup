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
} // namespace RTC
