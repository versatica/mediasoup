#ifndef MS_RTC_TRANSPORT_TUPLE_H
#define MS_RTC_TRANSPORT_TUPLE_H

#include "common.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPConnection.h"
#include "Utils.h"

namespace RTC
{
	class TransportTuple
	{
	public:
		enum class Type
		{
			UDP = 1,
			TCP
		};

	public:
		TransportTuple(RTC::UDPSocket* udpSocket, const struct sockaddr* udpRemoteAddr);
		TransportTuple(RTC::TCPConnection* tcpConnection);

		void Close();
		void Dump();
		void StoreUdpRemoteAddress();
		bool IsUDP();
		bool IsTCP();
		bool Compare(TransportTuple* tuple);
		void Send(const MS_BYTE* data, size_t len);
		const struct sockaddr* GetRemoteAddress();

	private:
		// Passed by argument.
		RTC::UDPSocket* udpSocket = nullptr;
		struct sockaddr* udpRemoteAddr = nullptr;
		sockaddr_storage udpRemoteAddrStorage;
		RTC::TCPConnection* tcpConnection = nullptr;
		// Others.
		Type type;
	};

	/* Inline methods. */

	inline
	void TransportTuple::Close()
	{
		if (this->type == Type::TCP)
			this->tcpConnection->Close();
	}

	inline
	TransportTuple::TransportTuple(RTC::UDPSocket* udpSocket, const struct sockaddr* udpRemoteAddr) :
		udpSocket(udpSocket),
		udpRemoteAddr((struct sockaddr*)udpRemoteAddr),
		type(Type::UDP)
	{}

	inline
	TransportTuple::TransportTuple(RTC::TCPConnection* tcpConnection) :
		tcpConnection(tcpConnection),
		type(Type::TCP)
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
	bool TransportTuple::IsUDP()
	{
		return (this->type == Type::UDP);
	}

	inline
	bool TransportTuple::IsTCP()
	{
		return (this->type == Type::TCP);
	}

	inline
	bool TransportTuple::Compare(TransportTuple* tuple)
	{
		if (this->type == Type::UDP && tuple->IsUDP())
		{
			return (this->udpSocket == tuple->udpSocket && Utils::IP::CompareAddresses(this->udpRemoteAddr, tuple->GetRemoteAddress()));
		}
		else if (this->type == Type::TCP && tuple->IsTCP())
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
		if (this->type == Type::UDP)
			this->udpSocket->Send(data, len, this->udpRemoteAddr);
		else
			this->tcpConnection->Send(data, len);
	}

	inline
	const struct sockaddr* TransportTuple::GetRemoteAddress()
	{
		if (this->type == Type::UDP)
			return (const struct sockaddr*)this->udpRemoteAddr;
		else
			return this->tcpConnection->GetPeerAddress();
	}
}

#endif
