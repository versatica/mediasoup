#ifndef MS_RTC_PORT_MANAGER_HPP
#define MS_RTC_PORT_MANAGER_HPP

#include "common.hpp"
#include "Settings.hpp"
#include <json.hpp>
#include <uv.h>
#include <string>
#include <unordered_map>
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
		static uv_udp_t* BindUdp(std::string& ip)
		{
			return reinterpret_cast<uv_udp_t*>(Bind(Transport::UDP, ip));
		}
		static uv_tcp_t* BindTcp(std::string& ip)
		{
			return reinterpret_cast<uv_tcp_t*>(Bind(Transport::TCP, ip));
		}
		static void UnbindUdp(std::string& ip, uint16_t port)
		{
			return Unbind(Transport::UDP, ip, port);
		}
		static void UnbindTcp(std::string& ip, uint16_t port)
		{
			return Unbind(Transport::TCP, ip, port);
		}
		static void FillJson(json& jsonObject);

	private:
		static uv_handle_t* Bind(Transport transport, std::string& ip);
		static void Unbind(Transport transport, std::string& ip, uint16_t port);
		static std::vector<bool>& GetPorts(Transport transport, const std::string& ip);

	private:
		static std::unordered_map<std::string, std::vector<bool>> mapUdpIpPorts;
		static std::unordered_map<std::string, std::vector<bool>> mapTcpIpPorts;
	};
} // namespace RTC

#endif
