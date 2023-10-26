#ifndef MS_RTC_UDP_SOCKET_HPP
#define MS_RTC_UDP_SOCKET_HPP

#include "common.hpp"
#include "handles/UdpSocketHandler.hpp"
#include <string>
#include <unordered_map>
namespace RTC
{
	class UdpSocket : public ::UdpSocketHandler
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnUdpSocketPacketReceived(
			  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) = 0;
		};

	public:
		UdpSocket(Listener* listener, std::string& ip);
		UdpSocket(Listener* listener, std::string& ip, uint16_t port);
		~UdpSocket() override;

		/* Pure virtual methods inherited from ::UdpSocketHandler. */
	public:
		void UserOnUdpDatagramReceived(const uint8_t* data, size_t len, const struct sockaddr* addr) override;
		void SetTransportByUserName(RTC::UdpSocket::Listener* listener, const std::string& name);
		void SetTransportByPeerId(RTC::UdpSocket::Listener* listener, const std::string& name);
		void DeleteTransport(RTC::UdpSocket::Listener* listener);
		void GetPeerId(const struct sockaddr* remoteAddr, std::string& peer_id);

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		bool fixedPort{ false };
		std::unordered_map<std::string, RTC::UdpSocket::Listener*> mapTransportPeerId;
		std::unordered_map<std::string, RTC::UdpSocket::Listener*> mapTransportName;
	};
} // namespace RTC

#endif
