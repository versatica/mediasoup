#ifndef MS_RTC_TRANSPORT_TUPLE_H
#define MS_RTC_TRANSPORT_TUPLE_H

#include "common.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPConnection.h"
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
		TransportTuple(RTC::UDPSocket* udpSocket, const struct sockaddr* udpRemoteAddr);
		TransportTuple(RTC::TCPConnection* tcpConnection);

		void Close();
		Json::Value toJson();
		void Dump();
		void StoreUdpRemoteAddress();
		bool Compare(TransportTuple* tuple);
		void Send(const MS_BYTE* data, size_t len);
		Protocol GetProtocol();
		const struct sockaddr* GetLocalAddress();
		const struct sockaddr* GetRemoteAddress();

	private:
		// Passed by argument.
		RTC::UDPSocket* udpSocket = nullptr;
		struct sockaddr* udpRemoteAddr = nullptr;
		sockaddr_storage udpRemoteAddrStorage;
		RTC::TCPConnection* tcpConnection = nullptr;
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
	TransportTuple::TransportTuple(RTC::UDPSocket* udpSocket, const struct sockaddr* udpRemoteAddr) :
		udpSocket(udpSocket),
		udpRemoteAddr((struct sockaddr*)udpRemoteAddr),
		protocol(Protocol::UDP)
	{}

	inline
	TransportTuple::TransportTuple(RTC::TCPConnection* tcpConnection) :
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
	void TransportTuple::Send(const MS_BYTE* data, size_t len)
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
