#ifndef MS_RTC_TRANSPORT_TUPLE_HPP
#define MS_RTC_TRANSPORT_TUPLE_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/UdpSocket.hpp"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

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

	public:
		TransportTuple(RTC::UdpSocket* udpSocket, const struct sockaddr* udpRemoteAddr)
		  : udpSocket(udpSocket), udpRemoteAddr((struct sockaddr*)udpRemoteAddr), protocol(Protocol::UDP)
		{
		}

		explicit TransportTuple(RTC::TcpConnection* tcpConnection)
		  : tcpConnection(tcpConnection), protocol(Protocol::TCP)
		{
		}

		explicit TransportTuple(const TransportTuple* tuple)
		  : udpSocket(tuple->udpSocket), udpRemoteAddr(tuple->udpRemoteAddr),
		    tcpConnection(tuple->tcpConnection), localAnnouncedIp(tuple->localAnnouncedIp),
		    protocol(tuple->protocol)
		{
			if (protocol == TransportTuple::Protocol::UDP)
				StoreUdpRemoteAddress();
		}

	public:
		void FillJson(json& jsonObject) const;

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
			if (this->protocol == Protocol::UDP && tuple->GetProtocol() == Protocol::UDP)
			{
				return (
				  this->udpSocket == tuple->udpSocket &&
				  Utils::IP::CompareAddresses(this->udpRemoteAddr, tuple->GetRemoteAddress()));
			}
			else if (this->protocol == Protocol::TCP && tuple->GetProtocol() == Protocol::TCP)
			{
				return (this->tcpConnection == tuple->tcpConnection);
			}
			else
			{
				return false;
			}
		}

		void SetLocalAnnouncedIp(std::string& localAnnouncedIp)
		{
			this->localAnnouncedIp = localAnnouncedIp;
		}

		void Send(const uint8_t* data, size_t len, RTC::TransportTuple::onSendCallback* cb = nullptr)
		{
			if (this->protocol == Protocol::UDP)
				this->udpSocket->Send(data, len, this->udpRemoteAddr, cb);
			else
				this->tcpConnection->Send(data, len, cb);
		}

		Protocol GetProtocol() const
		{
			return this->protocol;
		}

		const struct sockaddr* GetLocalAddress() const
		{
			if (this->protocol == Protocol::UDP)
				return this->udpSocket->GetLocalAddress();
			else
				return this->tcpConnection->GetLocalAddress();
		}

		const struct sockaddr* GetRemoteAddress() const
		{
			if (this->protocol == Protocol::UDP)
				return (const struct sockaddr*)this->udpRemoteAddr;
			else
				return this->tcpConnection->GetPeerAddress();
		}

		size_t GetRecvBytes() const
		{
			if (this->protocol == Protocol::UDP)
				return this->udpSocket->GetRecvBytes();
			else
				return this->tcpConnection->GetRecvBytes();
		}

		size_t GetSentBytes() const
		{
			if (this->protocol == Protocol::UDP)
				return this->udpSocket->GetSentBytes();
			else
				return this->tcpConnection->GetSentBytes();
		}

	private:
		// Passed by argument.
		RTC::UdpSocket* udpSocket{ nullptr };
		struct sockaddr* udpRemoteAddr{ nullptr };
		RTC::TcpConnection* tcpConnection{ nullptr };
		std::string localAnnouncedIp;
		// Others.
		struct sockaddr_storage udpRemoteAddrStorage;
		Protocol protocol;
	};
} // namespace RTC

#endif
