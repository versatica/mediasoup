#ifndef MS_RTC_PORT_MANAGER_HPP
#define MS_RTC_PORT_MANAGER_HPP

#include "common.hpp"
#include "Settings.hpp"
#include "RTC/Transport.hpp"
#include <uv.h>
#include <absl/container/flat_hash_map.h>
#include <string>
#include <vector>

namespace RTC
{
	class PortManager
	{
	private:
		enum class Transport : uint8_t
		{
			UDP = 1,
			TCP
		};

	public:
		static uv_udp_t* BindUdp(std::string& ip, RTC::Transport::SocketFlags& flags)
		{
			return reinterpret_cast<uv_udp_t*>(Bind(Transport::UDP, ip, flags));
		}
		static uv_udp_t* BindUdp(std::string& ip, uint16_t port, RTC::Transport::SocketFlags& flags)
		{
			return reinterpret_cast<uv_udp_t*>(Bind(Transport::UDP, ip, port, flags));
		}
		static uv_tcp_t* BindTcp(std::string& ip, RTC::Transport::SocketFlags& flags)
		{
			return reinterpret_cast<uv_tcp_t*>(Bind(Transport::TCP, ip, flags));
		}
		static uv_tcp_t* BindTcp(std::string& ip, uint16_t port, RTC::Transport::SocketFlags& flags)
		{
			return reinterpret_cast<uv_tcp_t*>(Bind(Transport::TCP, ip, port, flags));
		}
		static void UnbindUdp(std::string& ip, uint16_t port)
		{
			return Unbind(Transport::UDP, ip, port);
		}
		static void UnbindTcp(std::string& ip, uint16_t port)
		{
			return Unbind(Transport::TCP, ip, port);
		}

	private:
		static uv_handle_t* Bind(Transport transport, std::string& ip, RTC::Transport::SocketFlags& flags);
		static uv_handle_t* Bind(
		  Transport transport, std::string& ip, uint16_t port, RTC::Transport::SocketFlags& flags);
		static void Unbind(Transport transport, std::string& ip, uint16_t port);
		static std::vector<bool>& GetPorts(Transport transport, const std::string& ip);
		static uint8_t ConvertSocketFlags(
		  RTC::Transport::SocketFlags& flags, Transport transport, int family);

	private:
		thread_local static absl::flat_hash_map<std::string, std::vector<bool>> mapUdpIpPorts;
		thread_local static absl::flat_hash_map<std::string, std::vector<bool>> mapTcpIpPorts;
	};
} // namespace RTC

#endif
