#define MS_CLASS "RTC::TransportTuple"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TransportTuple.hpp"
#include "Logger.hpp"

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

		if (!Utils::IP::CompareAddresses(
		      reinterpret_cast<const sockaddr*>(std::addressof(this->remoteAddr)),
		      reinterpret_cast<const sockaddr*>(std::addressof(other.remoteAddr))))
		{
			return false;
		}

		if (!Utils::IP::CompareAddresses(
		      reinterpret_cast<const sockaddr*>(std::addressof(this->localAddr)),
		      reinterpret_cast<const sockaddr*>(std::addressof(other.localAddr))))
		{
			return false;
		}

		return true;
	}

	size_t TransportTuple::TupleKeyHash::operator()(const TupleKey& key) const noexcept
	{
		MS_TRACE();

		const auto protocolBits = static_cast<uint8_t>(key.protocol);
		const auto familyBits   = static_cast<uint16_t>(key.localAddr.ss_family);

		auto hashCombine = [](size_t& seed, size_t value)
		{
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		};

		size_t seed = 0;

		switch (key.localAddr.ss_family)
		{
			case AF_INET:
			{
				const auto* localIn  = reinterpret_cast<const sockaddr_in*>(std::addressof(key.localAddr));
				const auto* remoteIn = reinterpret_cast<const sockaddr_in*>(std::addressof(key.remoteAddr));

				hashCombine(seed, ankerl::unordered_dense::hash<uint8_t>{}(protocolBits));
				hashCombine(seed, ankerl::unordered_dense::hash<uint16_t>{}(familyBits));
				hashCombine(seed, ankerl::unordered_dense::hash<uint32_t>{}(localIn->sin_addr.s_addr));
				hashCombine(seed, ankerl::unordered_dense::hash<uint16_t>{}(localIn->sin_port));
				hashCombine(seed, ankerl::unordered_dense::hash<uint32_t>{}(remoteIn->sin_addr.s_addr));
				hashCombine(seed, ankerl::unordered_dense::hash<uint16_t>{}(remoteIn->sin_port));

				break;
			}

			case AF_INET6:
			{
				const auto* localIn6 = reinterpret_cast<const sockaddr_in6*>(std::addressof(key.localAddr));
				const auto* remoteIn6 = reinterpret_cast<const sockaddr_in6*>(std::addressof(key.remoteAddr));

				const auto* addr = localIn6->sin6_addr.s6_addr;

				uint64_t hi;
				uint64_t lo;
				std::memcpy(std::addressof(hi), addr, sizeof(uint64_t));
				std::memcpy(std::addressof(lo), addr + sizeof(uint64_t), sizeof(uint64_t));

				hashCombine(seed, ankerl::unordered_dense::hash<uint8_t>{}(protocolBits));
				hashCombine(seed, ankerl::unordered_dense::hash<uint16_t>{}(familyBits));
				hashCombine(seed, ankerl::unordered_dense::hash<uint64_t>{}(hi));
				hashCombine(seed, ankerl::unordered_dense::hash<uint64_t>{}(lo));
				hashCombine(seed, ankerl::unordered_dense::hash<uint16_t>{}(localIn6->sin6_port));

				addr = remoteIn6->sin6_addr.s6_addr;

				std::memcpy(std::addressof(hi), addr, sizeof(uint64_t));
				std::memcpy(std::addressof(lo), addr + sizeof(uint64_t), sizeof(uint64_t));

				hashCombine(seed, ankerl::unordered_dense::hash<uint64_t>{}(hi));
				hashCombine(seed, ankerl::unordered_dense::hash<uint64_t>{}(lo));
				hashCombine(seed, ankerl::unordered_dense::hash<uint16_t>{}(remoteIn6->sin6_port));

				break;
			}

			default:
			{
				hashCombine(seed, ankerl::unordered_dense::hash<uint8_t>{}(protocolBits));
				hashCombine(seed, ankerl::unordered_dense::hash<uint16_t>{}(familyBits));

				break;
			}
		}

		return seed;
	}
} // namespace RTC
