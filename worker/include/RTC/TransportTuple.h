#ifndef MS_RTC_TRANSPORT_TUPLE_H
#define MS_RTC_TRANSPORT_TUPLE_H

#include "common.h"
#include "RTC/UdpSocket.h"
#include "RTC/TcpConnection.h"
#include "Utils.h"
#include <json/json.h>

namespace RTC
{
	class TransportTuple
	{
	public:
		enum class Protocol
		{
			UDP = 1,
			TCP
		};

	public:
		TransportTuple(RTC::UdpSocket* udpSocket, const struct sockaddr* udpRemoteAddr);
		explicit TransportTuple(RTC::TcpConnection* tcpConnection);

		void Close();
		Json::Value toJson();
		void Dump();
		void StoreUdpRemoteAddress();
		bool Compare(TransportTuple* tuple);
		void Send(const uint8_t* data, size_t len);
		Protocol GetProtocol();
		const struct sockaddr* GetLocalAddress();
		const struct sockaddr* GetRemoteAddress();

	private:
		// Passed by argument.
		RTC::UdpSocket* udpSocket = nullptr;
		struct sockaddr* udpRemoteAddr = nullptr;
		sockaddr_storage udpRemoteAddrStorage;
		RTC::TcpConnection* tcpConnection = nullptr;
		// Others.
		Protocol protocol;
	};

	/* Inline methods. */

	inline
	void TransportTuple::Close()
	{
		if (this->protocol == Protocol::TCP)
			this->tcpConnection->Close();
	}

	inline
	TransportTuple::TransportTuple(RTC::UdpSocket* udpSocket, const struct sockaddr* udpRemoteAddr) :
		udpSocket(udpSocket),
		udpRemoteAddr((struct sockaddr*)udpRemoteAddr),
		protocol(Protocol::UDP)
	{}

	inline
	TransportTuple::TransportTuple(RTC::TcpConnection* tcpConnection) :
		tcpConnection(tcpConnection),
		protocol(Protocol::TCP)
	{}

	inline
	void TransportTuple::StoreUdpRemoteAddress()
	{
		// Clone the given address into our address storage and make the sockaddr
		// pointer point to it.
		this->udpRemoteAddrStorage = Utils::IP::CopyAddress(udpRemoteAddr);
		this->udpRemoteAddr = (struct sockaddr*)&this->udpRemoteAddrStorage;
	}

	inline
	TransportTuple::Protocol TransportTuple::GetProtocol()
	{
		return this->protocol;
	}

	inline
	bool TransportTuple::Compare(TransportTuple* tuple)
	{
		if (this->protocol == Protocol::UDP && tuple->GetProtocol() == Protocol::UDP)
		{
			return (this->udpSocket == tuple->udpSocket && Utils::IP::CompareAddresses(this->udpRemoteAddr, tuple->GetRemoteAddress()));
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

	inline
	void TransportTuple::Send(const uint8_t* data, size_t len)
	{
		if (this->protocol == Protocol::UDP)
			this->udpSocket->Send(data, len, this->udpRemoteAddr);
		else
			this->tcpConnection->Send(data, len);
	}

	inline
	const struct sockaddr* TransportTuple::GetLocalAddress()
	{
		if (this->protocol == Protocol::UDP)
			return this->udpSocket->GetLocalAddress();
		else
			return this->tcpConnection->GetLocalAddress();
	}

	inline
	const struct sockaddr* TransportTuple::GetRemoteAddress()
	{
		if (this->protocol == Protocol::UDP)
			return (const struct sockaddr*)this->udpRemoteAddr;
		else
			return this->tcpConnection->GetPeerAddress();
	}
}

#endif
