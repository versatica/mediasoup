#define MS_CLASS "RTC::TransportTuple"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TransportTuple.hpp"
#include "Logger.hpp"
#include <vector>

namespace RTC
{
	/* Static methods. */

	TransportTuple::Protocol TransportTuple::ProtocolFromFbs(FBS::Transport::Protocol protocol)
	{
		MS_TRACE();

		switch (protocol)
		{
			case FBS::Transport::Protocol::UDP:
			{
				return TransportTuple::Protocol::UDP;
			}

			case FBS::Transport::Protocol::TCP:
			{
				return TransportTuple::Protocol::TCP;
			}

				NO_DEFAULT_GCC();
		}
	}

	FBS::Transport::Protocol TransportTuple::ProtocolToFbs(TransportTuple::Protocol protocol)
	{
		MS_TRACE();

		switch (protocol)
		{
			case TransportTuple::Protocol::UDP:
			{
				return FBS::Transport::Protocol::UDP;
			}

			case TransportTuple::Protocol::TCP:
			{
				return FBS::Transport::Protocol::TCP;
			}

				NO_DEFAULT_GCC();
		}
	}

	/* Instance methods. */

	void TransportTuple::CloseTcpConnection()
	{
		MS_TRACE();

		if (this->protocol == Protocol::UDP)
		{
			MS_ABORT("cannot delete a UDP socket");
		}

		this->tcpConnection->TriggerClose();
	}

	flatbuffers::Offset<FBS::Transport::Tuple> TransportTuple::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		int family;
		std::string localIp;
		uint16_t localPort;

		Utils::IP::GetAddressInfo(GetLocalAddress(), family, localIp, localPort);

		std::string remoteIp;
		uint16_t remotePort;

		Utils::IP::GetAddressInfo(GetRemoteAddress(), family, remoteIp, remotePort);

		auto protocol = TransportTuple::ProtocolToFbs(GetProtocol());

		return FBS::Transport::CreateTupleDirect(
		  builder,
		  (this->localAnnouncedAddress.empty() ? localIp : this->localAnnouncedAddress).c_str(),
		  localPort,
		  remoteIp.c_str(),
		  remotePort,
		  protocol);
	}

	void TransportTuple::Dump(int indentation) const
	{
		MS_TRACE();

		MS_DUMP_CLEAN(indentation, "<TransportTuple>");

		int family;
		std::string ip;
		uint16_t port;

		Utils::IP::GetAddressInfo(GetLocalAddress(), family, ip, port);

		MS_DUMP_CLEAN(indentation, "  localIp: %s", ip.c_str());
		MS_DUMP_CLEAN(indentation, "  localPort: %" PRIu16, port);

		Utils::IP::GetAddressInfo(GetRemoteAddress(), family, ip, port);

		MS_DUMP_CLEAN(indentation, "  remoteIp: %s", ip.c_str());
		MS_DUMP_CLEAN(indentation, "  remotePort: %" PRIu16, port);

		switch (GetProtocol())
		{
			case Protocol::UDP:
			{
				MS_DUMP_CLEAN(indentation, "  protocol: udp");

				break;
			}

			case Protocol::TCP:
			{
				MS_DUMP_CLEAN(indentation, "  protocol: tcp");

				break;
			}
		}

		MS_DUMP_CLEAN(indentation, "</TransportTuple>");
	}

	void TransportTuple::GenerateHash()
	{
		MS_TRACE();

		const auto* localSockAddr  = GetLocalAddress();
		const auto* remoteSockAddr = GetRemoteAddress();

		std::vector<uint8_t> buffer;

		auto appendSockAddr = [&](const sockaddr* addr)
		{
			if (addr->sa_family == AF_INET)
			{
				const sockaddr_in* in = reinterpret_cast<const sockaddr_in*>(addr);
				const uint8_t* ip     = reinterpret_cast<const uint8_t*>(&in->sin_addr.s_addr);
				uint16_t port         = ntohs(in->sin_port);

				buffer.insert(buffer.end(), ip, ip + 4);
				buffer.push_back((port >> 8) & 0xFF);
				buffer.push_back(port & 0xFF);
			}
			else if (addr->sa_family == AF_INET6)
			{
				const sockaddr_in6* in6 = reinterpret_cast<const sockaddr_in6*>(addr);
				const uint8_t* ip       = reinterpret_cast<const uint8_t*>(&in6->sin6_addr);
				uint16_t port           = ntohs(in6->sin6_port);

				buffer.insert(buffer.end(), ip, ip + 16);
				buffer.push_back((port >> 8) & 0xFF);
				buffer.push_back(port & 0xFF);
			}
		};

		appendSockAddr(localSockAddr);
		appendSockAddr(remoteSockAddr);

		buffer.push_back(static_cast<uint8_t>(this->protocol));

		this->hash = GenerateFnv1aHash(buffer.data(), buffer.size());
	}

	uint64_t TransportTuple::GenerateFnv1aHash(const uint8_t* data, size_t size)
	{
		MS_TRACE();

		const uint64_t fnvOffsetBasis = 14695981039346656037ull;
		const uint64_t fnvPrime       = 1099511628211ull;
		uint64_t hash                 = fnvOffsetBasis;

		for (size_t i = 0; i < size; ++i)
		{
			hash = (hash ^ data[i]) * fnvPrime;
		}

		return hash;
	}
} // namespace RTC
