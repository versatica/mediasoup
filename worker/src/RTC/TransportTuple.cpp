#define MS_CLASS "RTC::TransportTuple"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TransportTuple.hpp"
#include "Logger.hpp"
#include <cstring> // std::memcpy()

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

	bool TransportTuple::TupleKey::operator==(const TupleKey& other) const noexcept
	{
		MS_TRACE();

		if (this->protocol != other.protocol)
		{
			return false;
		}

		switch (this->protocol)
		{
			// For UDP compare the remote address first (it discriminates faster) and then
			// the socket.
			case Protocol::UDP:
			{
				return Utils::IP::CompareAddresses(this->udpRemoteAddr, other.udpRemoteAddr) &&
				       this->udpSocketOrTcpConnection == other.udpSocketOrTcpConnection;
			}

			// For TCP the connection pointer fully identifies the tuple.
			case Protocol::TCP:
			{
				return this->udpSocketOrTcpConnection == other.udpSocketOrTcpConnection;
			}

				NO_DEFAULT_GCC();
		}
	}

	size_t TransportTuple::TupleKeyHash::operator()(const TupleKey& key) const noexcept
	{
		MS_TRACE();

		const auto protocolBits = static_cast<uint8_t>(key.protocol);

		size_t seed = 0;

		Utils::Crypto::HashCombine(seed, ankerl::unordered_dense::hash<uint8_t>{}(protocolBits));
		Utils::Crypto::HashCombine(
		  seed,
		  ankerl::unordered_dense::hash<uintptr_t>{}(
		    reinterpret_cast<uintptr_t>(key.udpSocketOrTcpConnection)));

		switch (key.protocol)
		{
			case Protocol::UDP:
			{
				// For UDP also combine the remote address.
				const auto familyBits = static_cast<uint16_t>(key.udpRemoteAddr->sa_family);

				Utils::Crypto::HashCombine(seed, ankerl::unordered_dense::hash<uint16_t>{}(familyBits));

				switch (key.udpRemoteAddr->sa_family)
				{
					case AF_INET:
					{
						const auto* remoteIn = reinterpret_cast<const sockaddr_in*>(key.udpRemoteAddr);

						Utils::Crypto::HashCombine(
						  seed, ankerl::unordered_dense::hash<uint32_t>{}(remoteIn->sin_addr.s_addr));
						Utils::Crypto::HashCombine(
						  seed, ankerl::unordered_dense::hash<uint16_t>{}(remoteIn->sin_port));

						break;
					}

					case AF_INET6:
					{
						const auto* remoteIn6 = reinterpret_cast<const sockaddr_in6*>(key.udpRemoteAddr);
						const auto* addr      = remoteIn6->sin6_addr.s6_addr;

						uint64_t hi;
						uint64_t lo;

						std::memcpy(std::addressof(hi), addr, sizeof(uint64_t));
						std::memcpy(std::addressof(lo), addr + sizeof(uint64_t), sizeof(uint64_t));

						Utils::Crypto::HashCombine(seed, ankerl::unordered_dense::hash<uint64_t>{}(hi));
						Utils::Crypto::HashCombine(seed, ankerl::unordered_dense::hash<uint64_t>{}(lo));
						Utils::Crypto::HashCombine(
						  seed, ankerl::unordered_dense::hash<uint16_t>{}(remoteIn6->sin6_port));

						break;
					}

					default:;
				}

				return seed;
			}

			case Protocol::TCP:
			{
				// For TCP the connection pointer is enough.
				return seed;
			}

				NO_DEFAULT_GCC();
		}
	}
} // namespace RTC
