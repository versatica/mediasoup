#ifndef MS_RTC_TRANSPORT_TUPLE_HPP
#define MS_RTC_TRANSPORT_TUPLE_HPP

#include "common.hpp"
#include "FBS/transport.h"
#include "handles/SendCallbacks.hpp"
#include "RTC/Consts.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/UdpSocket.hpp"
#include "Utils.hpp"
#include <flatbuffers/flatbuffers.h>
#include <uv.h>
#include <string>

namespace RTC
{
	class TransportTuple
	{
	public:
		enum class Protocol : uint8_t
		{
			UDP = 1,
			TCP = 2
		};

	public:
		class TupleKey
		{
		private:
			friend class TransportTuple;
			friend struct TupleKeyHash;

		public:
			TupleKey() = default;

			TupleKey(Protocol protocol, const void* udpSocketOrTcpConnection, const sockaddr* udpRemoteAddr)
			  : protocol(protocol),
			    udpSocketOrTcpConnection(udpSocketOrTcpConnection),
			    udpRemoteAddr(udpRemoteAddr)
			{
			}

			bool operator==(const TupleKey& other) const noexcept;

			bool operator!=(const TupleKey& other) const noexcept
			{
				return !(*this == other);
			}

		private:
			Protocol protocol{ Protocol::UDP };
			// The local endpoint is identified by object pointer identity: the
			// UdpSocket or the TcpConnection. For TCP the connection alone identifies
			// the whole tuple, so `udpRemoteAddr` is unused.
			const void* udpSocketOrTcpConnection{ nullptr };
			const sockaddr* udpRemoteAddr{ nullptr };
		};

	public:
		struct TupleKeyHash
		{
			size_t operator()(const TupleKey& key) const noexcept;
		};

	public:
		static Protocol ProtocolFromFbs(FBS::Transport::Protocol protocol);

		static FBS::Transport::Protocol ProtocolToFbs(Protocol protocol);

	public:
		TransportTuple(RTC::UdpSocket* udpSocket, const sockaddr* udpRemoteAddr)
		  : udpSocket(udpSocket),
		    udpRemoteAddr(const_cast<sockaddr*>(udpRemoteAddr)),
		    protocol(Protocol::UDP),
		    tupleKey(Protocol::UDP, udpSocket, udpRemoteAddr)
		{
		}

		explicit TransportTuple(RTC::TcpConnection* tcpConnection)
		  : tcpConnection(tcpConnection),
		    protocol(Protocol::TCP),
		    tupleKey(Protocol::TCP, tcpConnection, nullptr)
		{
		}

		explicit TransportTuple(const TransportTuple* tuple)
		  : udpSocket(tuple->udpSocket),
		    udpRemoteAddr(tuple->udpRemoteAddr),
		    tcpConnection(tuple->tcpConnection),
		    protocol(tuple->protocol)
		{
			if (protocol == TransportTuple::Protocol::UDP)
			{
				StoreUdpRemoteAddress();
			}
			else
			{
				this->tupleKey = TupleKey(this->protocol, this->tcpConnection, nullptr);
			}
		}

	public:
		void Dump(int indentation = 0) const;

		flatbuffers::Offset<FBS::Transport::Tuple> FillBuffer(flatbuffers::FlatBufferBuilder& builder) const;

		void CloseTcpConnection();

		void StoreUdpRemoteAddress()
		{
			// Clone the given address into our address storage and make the sockaddr
			// pointer point to it.
			this->udpRemoteAddrStorage = Utils::IP::CopyAddress(this->udpRemoteAddr);
			this->udpRemoteAddr = reinterpret_cast<sockaddr*>(std::addressof(this->udpRemoteAddrStorage));
			this->tupleKey      = TupleKey(this->protocol, this->udpSocket, this->udpRemoteAddr);
		}

		bool Compare(const TransportTuple* tuple) const
		{
			if (this->protocol != tuple->protocol)
			{
				return false;
			}

			switch (this->protocol)
			{
				case Protocol::UDP:
				{
					return (
					  this->udpSocket == tuple->udpSocket &&
					  Utils::IP::CompareAddresses(this->udpRemoteAddr, tuple->udpRemoteAddr));
				}

				case Protocol::TCP:
				{
					return (this->tcpConnection == tuple->tcpConnection);
				}
			}

			return true;
		}

		void Send(const uint8_t* data, size_t len, onSendCallback cb = {})
		{
			if (this->protocol == Protocol::UDP)
			{
				this->udpSocket->Send(data, len, this->udpRemoteAddr, std::move(cb));
			}
			else
			{
				this->tcpConnection->Send(data, len, std::move(cb));
			}
		}

		Protocol GetProtocol() const
		{
			return this->protocol;
		}

		/**
		 * Bytes that every packet sent through this tuple carries on top of its own
		 * length, which is what the network sees but nothing in here counts.
		 *
		 * @remarks
		 * - It only contemplates IPv4 and IPv6 for the network layer and UDP and
		 *   TCP for the transport one, both without options, plus the length field
		 *   that frames a packet sent over TCP. Anything below that, such as the
		 *   Ethernet frame or a tunnel, is not counted.
		 */
		size_t GetPacketOverhead() const
		{
			size_t overhead{ 0 };

			switch (GetLocalAddress()->sa_family)
			{
				case AF_INET:
				{
					overhead += RTC::Consts::Ipv4HeaderSize;

					break;
				}

				case AF_INET6:
				{
					overhead += RTC::Consts::Ipv6HeaderSize;

					break;
				}

				default:
				{
					break;
				}
			}

			switch (this->protocol)
			{
				case Protocol::UDP:
				{
					overhead += RTC::Consts::UdpHeaderSize;

					break;
				}

				case Protocol::TCP:
				{
					overhead += RTC::Consts::TcpHeaderSize + RTC::Consts::TcpFramingSize;

					break;
				}
			}

			return overhead;
		}

		const sockaddr* GetLocalAddress() const
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

		const std::string& GetLocalAnnouncedAddress() const
		{
			if (this->protocol == Protocol::UDP)
			{
				return this->udpSocket->GetAnnouncedAddress();
			}
			else
			{
				return this->tcpConnection->GetAnnouncedAddress();
			}
		}

		const sockaddr* GetRemoteAddress() const
		{
			if (this->protocol == Protocol::UDP)
			{
				return this->udpRemoteAddr;
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

		const TupleKey& GetTupleKey() const
		{
			return this->tupleKey;
		}

	private:
		// Passed by argument.
		RTC::UdpSocket* udpSocket{ nullptr };
		sockaddr* udpRemoteAddr{ nullptr };
		RTC::TcpConnection* tcpConnection{ nullptr };
		// Others.
		sockaddr_storage udpRemoteAddrStorage{};
		Protocol protocol;
		TupleKey tupleKey;
	};
} // namespace RTC

#endif
