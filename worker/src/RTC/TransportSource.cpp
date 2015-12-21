#define MS_CLASS "RTC::TransportSource"

#include "RTC/TransportSource.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	void TransportSource::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		switch (this->type)
		{
			case Type::UDP:
			{
				int remote_family;
				std::string remote_ip;
				MS_PORT remote_port;

				Utils::IP::GetAddressInfo(GetRemoteAddress(), &remote_family, remote_ip, &remote_port);

				MS_DEBUG("[UDP | local: %s : %u | remote: %s : %u]",
					this->udpSocket->GetLocalIP().c_str(), (unsigned int)this->udpSocket->GetLocalPort(),
					remote_ip.c_str(), remote_port);
				break;
			}

			case Type::TCP:
			{
				MS_DEBUG("[TCP | local: %s : %u | remote: %s : %u]",
					this->tcpConnection->GetLocalIP().c_str(), (unsigned int)this->tcpConnection->GetLocalPort(),
					this->tcpConnection->GetPeerIP().c_str(), (unsigned int)this->tcpConnection->GetPeerPort());
				break;
			}
		}
	}
}
