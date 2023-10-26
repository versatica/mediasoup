#define MS_CLASS "RTC::UdpSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/UdpSocket.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "RTC/PortManager.hpp"
#include "RTC/StunPacket.hpp"
#include <string>

namespace RTC
{
	/* Instance methods. */
	static constexpr size_t StunBufferSize{ 65536 };
	static uint8_t StunBuffer[StunBufferSize];

	UdpSocket::UdpSocket(Listener* listener, std::string& ip)
	  : // This may throw.
	    ::UdpSocketHandler::UdpSocketHandler(PortManager::BindUdp(ip)), listener(listener)
	{
		MS_TRACE();
	}

	UdpSocket::UdpSocket(Listener* listener, std::string& ip, uint16_t port)
	  : // This may throw.
	    ::UdpSocketHandler::UdpSocketHandler(PortManager::BindUdp(ip, port)), listener(listener),
	    fixedPort(true)
	{
		MS_TRACE();
	}

	UdpSocket::~UdpSocket()
	{
		MS_TRACE();

		if (!fixedPort)
		{
			PortManager::UnbindUdp(this->localIp, this->localPort);
		}
	}

	void UdpSocket::GetPeerId(const struct sockaddr* remoteAddr, std::string& peer_id)
	{
		int family = 0;
		std::string ip;
		uint16_t port;
		Utils::IP::GetAddressInfo(remoteAddr, family, ip, port);

		char id_buf[512] = { 0 };
		int len          = snprintf(id_buf, sizeof(id_buf), "%s:%d", ip.c_str(), port);
		peer_id          = std::move(std::string(id_buf, len));
	}

	void UdpSocket::UserOnUdpDatagramReceived(const uint8_t* data, size_t len, const struct sockaddr* addr)
	{
		MS_TRACE();

		// if (!this->listener)
		// {
		// 	MS_ERROR("no listener set");

		// 	return;
		// }

		// // Notify the reader.
		// this->listener->OnUdpSocketPacketReceived(this, data, len, addr);

		std::string peer_id;
		GetPeerId(addr, peer_id);

		RTC::UdpSocket::Listener* listener = NULL;

		auto peer_iter = mapTransportPeerId.find(peer_id);
		if (peer_iter != mapTransportPeerId.end())
		{
			listener = peer_iter->second;
		}

		if (listener)
		{
			listener->OnUdpSocketPacketReceived(this, data, len, addr);
			return;
		}

		RTC::StunPacket* packet = RTC::StunPacket::Parse(data, len);

		if (!packet)
		{
			MS_WARN_DEV("ignoring wrong STUN packet received");
			return;
		}
		std::string username = packet->GetUsername();
		auto size            = username.find(":");
		if (size != std::string::npos)
		{
			username = username.substr(0, size);
		}
		auto name_iter = mapTransportName.find(username);
		if (name_iter == mapTransportName.end())
		{
			// Reply 400.
			RTC::StunPacket* response = packet->CreateErrorResponse(400);
			response->Serialize(StunBuffer);
			this->Send(response->GetData(), response->GetSize(), addr, NULL);
			delete packet;
			delete response;
			return;
		}

		listener = name_iter->second;
		SetTransportByPeerId(listener, peer_id);

		if (listener)
		{
			listener->OnUdpSocketPacketReceived(this, data, len, addr);
		}
		delete packet;
	}

	void UdpSocket::SetTransportByUserName(RTC::UdpSocket::Listener* listener, const std::string& name)
	{
		mapTransportName[name] = listener;
	}

	void UdpSocket::SetTransportByPeerId(RTC::UdpSocket::Listener* listener, const std::string& peerId)
	{
		mapTransportPeerId[peerId] = listener;
	}

	void UdpSocket::DeleteTransport(RTC::UdpSocket::Listener* listener)
	{
		for (auto iter = mapTransportName.begin(); iter != mapTransportName.end();)
		{
			if (iter->second == listener)
			{
				mapTransportName.erase(iter++);
			}
			else
			{
				iter++;
			}
		}

		for (auto iter = mapTransportPeerId.begin(); iter != mapTransportPeerId.end();)
		{
			if (iter->second == listener)
			{
				mapTransportPeerId.erase(iter++);
			}
			else
			{
				iter++;
			}
		}
	}
} // namespace RTC
