#ifndef MS_RTC_TRANSPORT_SOURCE_H
#define MS_RTC_TRANSPORT_SOURCE_H

#include "common.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPConnection.h"
#include "Utils.h"
#include <string>

namespace RTC
{
	class TransportSource
	{
	public:
		enum class Type
		{
			UDP = 1,
			TCP
		};

	public:
		TransportSource(RTC::UDPSocket* udpSocket, const struct sockaddr* udpRemoteAddr);
		TransportSource(RTC::TCPConnection* tcpConnection);

		void StoreUdpRemoteAddress();
		bool IsUDP();
		bool IsTCP();
		bool Compare(TransportSource* source);
		void Send(const MS_BYTE* data, size_t len);
		const struct sockaddr* GetRemoteAddress();
		void Close();
		void Dump();

	private:
		// Passed by argument:
		RTC::UDPSocket* udpSocket = nullptr;
		struct sockaddr* udpRemoteAddr = nullptr;
		sockaddr_storage udpRemoteAddrStorage;
		RTC::TCPConnection* tcpConnection = nullptr;
		// Others:
		Type type;
	};

	/* Inline methods. */

	inline
	TransportSource::TransportSource(RTC::UDPSocket* udpSocket, const struct sockaddr* udpRemoteAddr) :
		udpSocket(udpSocket),
		udpRemoteAddr((struct sockaddr*)udpRemoteAddr),
		type(Type::UDP)
	{}

	inline
	TransportSource::TransportSource(RTC::TCPConnection* tcpConnection) :
		tcpConnection(tcpConnection),
		type(Type::TCP)
	{}

	inline
	void TransportSource::StoreUdpRemoteAddress()
	{
		// Clone the given address into our address storage and make the sockaddr
		// pointer point to it.
		this->udpRemoteAddrStorage = Utils::IP::CopyAddress(udpRemoteAddr);
		this->udpRemoteAddr = (struct sockaddr*)&this->udpRemoteAddrStorage;
	}

	inline
	bool TransportSource::IsUDP()
	{
		return (this->type == Type::UDP);
	}

	inline
	bool TransportSource::IsTCP()
	{
		return (this->type == Type::TCP);
	}

	inline
	bool TransportSource::Compare(TransportSource* source)
	{
		if (this->type == Type::UDP && source->IsUDP())
		{
			return (this->udpSocket == source->udpSocket && Utils::IP::CompareAddresses(this->udpRemoteAddr, source->GetRemoteAddress()));
		}
		else if (this->type == Type::TCP && source->IsTCP())
		{
			return (this->tcpConnection == source->tcpConnection);
		}
		else
		{
			return false;
		}
	}

	inline
	void TransportSource::Send(const MS_BYTE* data, size_t len)
	{
		if (this->type == Type::UDP)
			this->udpSocket->Send(data, len, this->udpRemoteAddr);
		else
			this->tcpConnection->Send(data, len);
	}

	inline
	const struct sockaddr* TransportSource::GetRemoteAddress()
	{
		if (this->type == Type::UDP)
			return (const struct sockaddr*)this->udpRemoteAddr;
		else
			return this->tcpConnection->GetPeerAddress();
	}

	inline
	void TransportSource::Close()
	{
		if (this->type == Type::TCP)
			this->tcpConnection->Close();
	}
}  // namespace RTC

#endif
