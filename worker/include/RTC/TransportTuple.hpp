#ifndef MS_RTC_TRANSPORT_TUPLE_HPP
#define MS_RTC_TRANSPORT_TUPLE_HPP

#include "common.hpp"
#include "FBS/transport.h"
#include "RTC/TcpConnection.hpp"
#include "RTC/UdpSocket.hpp"
#include "Utils.hpp"
#include <flatbuffers/flatbuffers.h>
#include <string>

namespace RTC
{
	class TransportTuple
	{
	protected:
		using onSendCallback = const std::function<void(bool sent)>;

	public:
		enum class Protocol : uint8_t
		{
			UDP = 1,
			TCP = 2
		};

		class TupleKey
		{
		private:
			friend class TransportTuple;
			friend class TupleKeyHash;

		public:
			TupleKey() = default;
			TupleKey(Protocol protocol, const void* udpSocketOrTcpConnection, const struct sockaddr* udpRemoteAddr)
			  : protocol(protocol),
			    udpSocketOrTcpConnection(udpSocketOrTcpConnection),
			    udpRemoteAddr(udpRemoteAddr) {};

			bool operator==(const TupleKey& other) const noexcept;
			bool operator!=(const TupleKey& other) const noexcept
			{
				return !(*this == other);
			};

		private:
			Protocol protocol{ Protocol::UDP };
			// The local endpoint is identified by object pointer identity: the
			// UdpSocket or the TcpConnection. For TCP the connection alone identifies
			// the whole tuple, so `udpRemoteAddr` is unused.
			const void* udpSocketOrTcpConnection{ nullptr };
			const struct sockaddr* udpRemoteAddr{ nullptr };
		};

		struct TupleKeyHash
		{
			size_t operator()(const TupleKey& key) const noexcept;
		};

		static Protocol ProtocolFromFbs(FBS::Transport::Protocol protocol);
		static FBS::Transport::Protocol ProtocolToFbs(Protocol protocol);

	public:
		TransportTuple(RTC::UdpSocket* udpSocket, const struct sockaddr* udpRemoteAddr)
		  : udpSocket(udpSocket),
		    udpRemoteAddr(const_cast<struct sockaddr*>(udpRemoteAddr)),
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
		    localAnnouncedAddress(tuple->localAnnouncedAddress),
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
		void CloseTcpConnection();

		flatbuffers::Offset<FBS::Transport::Tuple> FillBuffer(flatbuffers::FlatBufferBuilder& builder) const;

		void Dump(int indentation = 0) const;

		void StoreUdpRemoteAddress()
		{
			// Clone the given address into our address storage and make the sockaddr
			// pointer point to it.
			this->udpRemoteAddrStorage = Utils::IP::CopyAddress(this->udpRemoteAddr);
			this->udpRemoteAddr =
			  reinterpret_cast<struct sockaddr*>(std::addressof(this->udpRemoteAddrStorage));
			this->tupleKey = TupleKey(this->protocol, this->udpSocket, this->udpRemoteAddr);
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

		void SetLocalAnnouncedAddress(std::string& localAnnouncedAddress)
		{
			this->localAnnouncedAddress = localAnnouncedAddress;
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
				return static_cast<const struct sockaddr*>(this->udpRemoteAddr);
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
		struct sockaddr* udpRemoteAddr{ nullptr };
		RTC::TcpConnection* tcpConnection{ nullptr };
		std::string localAnnouncedAddress;
		// Others.
		struct sockaddr_storage udpRemoteAddrStorage{};
		Protocol protocol;
		TupleKey tupleKey;
	};
} // namespace RTC

#endif
