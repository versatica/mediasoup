#ifndef MS_RTC_PORT_MANAGER_HPP
#define MS_RTC_PORT_MANAGER_HPP

#include "common.hpp"
#include "Settings.hpp"
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
		static uv_udp_t* BindUdp(std::string& ip);
		// static uv_tcp_t* BindTcp(const std::string& ip);
		// static voip UnbindUdp(const std::string& ip, uint16_t port);
		// static voip UnbindTcp(const std::string& ip, uint16_t port);

	private:
		std::vector<bool>& GetPorts(Transport transport, const std::string& ip);

	private:
		static std::unordered_map<std::string, std::vector<bool>> udpBindings;
		static std::unordered_map<std::string, std::vector<bool>> tcpBindings;
	};
} // namespace RTC

#endif
