#ifndef MS_RTC_TRANSPORT_TUPLE_HPP
#define MS_RTC_TRANSPORT_TUPLE_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "FBS/transport.h"
#include "RTC/TcpConnection.hpp"
#include "RTC/UdpSocket.hpp"
#include <flatbuffers/flatbuffers.h>
#include <string>

namespace RTC
{
	class TransportTuple
	{
	protected:
		using onSendCallback = const std::function<void(bool sent)>;

	public:
		enum class Protocol
		{
			UDP = 1,
			TCP
		};

		static Protocol ProtocolFromFbs(FBS::Transport::Protocol protocol);
		static FBS::Transport::Protocol ProtocolToFbs(Protocol protocol);

	public:
		TransportTuple(RTC::UdpSocket* udpSocket, const struct sockaddr* udpRemoteAddr)
		  : udpSocket(udpSocket), udpRemoteAddr((struct sockaddr*)udpRemoteAddr), protocol(Protocol::UDP)
		{
			SetHash();
		}

		explicit TransportTuple(RTC::TcpConnection* tcpConnection)
		  : tcpConnection(tcpConnection), protocol(Protocol::TCP)
		{
			SetHash();
		}

		explicit TransportTuple(const TransportTuple* tuple)
		  : hash(tuple->hash), udpSocket(tuple->udpSocket), udpRemoteAddr(tuple->udpRemoteAddr),
		    tcpConnection(tuple->tcpConnection), localAnnouncedIp(tuple->localAnnouncedIp),
		    protocol(tuple->protocol)
		{
			if (protocol == TransportTuple::Protocol::UDP)
			{
				StoreUdpRemoteAddress();
			}
		}

	public:
		void Close()
		{
			if (this->protocol == Protocol::UDP)
			{
				this->udpSocket->Close();
			}
			else
			{
				this->tcpConnection->Close();
			}
		}

		bool IsClosed()
		{
			if (this->protocol == Protocol::UDP)
			{
				return this->udpSocket->IsClosed();
			}
			else
			{
				return this->tcpConnection->IsClosed();
			}
		}

		flatbuffers::Offset<FBS::Transport::Tuple> FillBuffer(flatbuffers::FlatBufferBuilder& builder) const;

		void Dump() const;

		void StoreUdpRemoteAddress()
		{
			// Clone the given address into our address storage and make the sockaddr
			// pointer point to it.
			this->udpRemoteAddrStorage = Utils::IP::CopyAddress(this->udpRemoteAddr);
			this->udpRemoteAddr        = (struct sockaddr*)&this->udpRemoteAddrStorage;
		}

		bool Compare(const TransportTuple* tuple) const
		{
			return this->hash == tuple->hash;
		}

		void SetLocalAnnouncedIp(std::string& localAnnouncedIp)
		{
			this->localAnnouncedIp = localAnnouncedIp;
		}

		void Send(const uint8_t* data, size_t len, RTC::TransportTuple::onSendCallback* cb = nullptr)
		{
			if (this->protocol == Protocol::UDP)
			{
				this->udpSocket->Send(data, len, this->udpRemoteAddr, cb);
			}
			else
			{
				this->tcpConnection->Send(data, len, cb);
			}
		}

		Protocol GetProtocol() const
		{
			return this->protocol;
		}

		const struct sockaddr* GetLocalAddress() const
		{
			if (this->protocol == Protocol::UDP)
			{
				return this->udpSocket->GetLocalAddress();
			}
			else
			{
				return this->tcpConnection->GetLocalAddress();
			}
		}

		const struct sockaddr* GetRemoteAddress() const
		{
			if (this->protocol == Protocol::UDP)
			{
				return (const struct sockaddr*)this->udpRemoteAddr;
			}
			else
			{
				return this->tcpConnection->GetPeerAddress();
			}
		}

		size_t GetRecvBytes() const
		{
			if (this->protocol == Protocol::UDP)
			{
				return this->udpSocket->GetRecvBytes();
			}
			else
			{
				return this->tcpConnection->GetRecvBytes();
			}
		}

		size_t GetSentBytes() const
		{
			if (this->protocol == Protocol::UDP)
			{
				return this->udpSocket->GetSentBytes();
			}
			else
			{
				return this->tcpConnection->GetSentBytes();
			}
		}

	private:
		/*
		 * Hash for IPv4
		 *
		 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 |              PORT             |             IP                |
		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 |              IP               |                           |F|P|
		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * Hash for IPv6
		 *
		 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 |              PORT             | IP[0] ^  IP[1] ^ IP[2] ^ IP[3]|
		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 |IP[0] ^  IP[1] ^ IP[2] ^ IP[3] |          IP[0] >> 16      |F|P|
		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */
		void SetHash()
		{
			const struct sockaddr* remoteSockAddr = GetRemoteAddress();

			switch (remoteSockAddr->sa_family)
			{
				case AF_INET:
				{
					const auto* remoteSockAddrIn = reinterpret_cast<const struct sockaddr_in*>(remoteSockAddr);

					const uint64_t address = ntohl(remoteSockAddrIn->sin_addr.s_addr);
					const uint64_t port    = (ntohs(remoteSockAddrIn->sin_port));

					this->hash = port << 48;
					this->hash |= address << 16;
					this->hash |= 0x0000; // AF_INET.

					break;
				}

				case AF_INET6:
				{
					const auto* remoteSockAddrIn6 =
					  reinterpret_cast<const struct sockaddr_in6*>(remoteSockAddr);
					const auto* a =
					  reinterpret_cast<const uint32_t*>(std::addressof(remoteSockAddrIn6->sin6_addr));

					const auto address1 = a[0] ^ a[1] ^ a[2] ^ a[3];
					const auto address2 = a[0];
					const uint64_t port = ntohs(remoteSockAddrIn6->sin6_port);

					this->hash = port << 48;
					this->hash |= static_cast<uint64_t>(address1) << 16;
					this->hash |= address2 >> 16 & 0xFFFC;
					this->hash |= 0x0002; // AF_INET6.

					break;
				}
			}

			// Override least significant bit with protocol information:
			// - If UDP, start with 0.
			// - If TCP, start with 1.
			if (this->protocol == Protocol::UDP)
			{
				this->hash |= 0x0000;
			}
			else
			{
				this->hash |= 0x0001;
			}
		}

	public:
		uint64_t hash{ 0u };

	private:
		// Passed by argument.
		RTC::UdpSocket* udpSocket{ nullptr };
		struct sockaddr* udpRemoteAddr{ nullptr };
		RTC::TcpConnection* tcpConnection{ nullptr };
		std::string localAnnouncedIp;
		// Others.
		struct sockaddr_storage udpRemoteAddrStorage
		{
		};
		Protocol protocol;
	};
} // namespace RTC

#endif
