#define MS_CLASS "RTC::PortManager"
// #define MS_LOG_DEV

#include "RTC/PortManager.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Settings.hpp"
#include "Utils.hpp"

#include <utility>

/* Static methods for UV callbacks. */

static inline void onClose(uv_handle_t* handle)
{
	delete handle;
}

namespace RTC
{
	/* Static. */

	static constexpr uint16_t MaxBindAttempts{ 20 };

	/* Class variables. */

	std::unordered_map<std::string, std::vector<bool>> PortManager::udpBindings;
	std::unordered_map<std::string, std::vector<bool>> PortManager::tcpBindings;

	/* Class methods. */

	uv_udp_t* PortManager::BindUdp(std::string& ip)
	{
		MS_TRACE();

		// First normalize the IP.
		Utils::IP::NormalizeIp(ip);

		// Check if this IP is already handled.
		// MayAddNewIp(ip);

		int family = Utils::IP::GetFamily(ip);

		// uv_udp_t* uvHandle{ nullptr };
		// struct sockaddr_storage bindAddr;
		// uint16_t initialPort;
		// uint16_t iteratingPort;
		// uint16_t attempt{ 0 };
		// uint16_t bindAttempt{ 0 };
		// int flags{ 0 };
		// int err;


		return new uv_udp_t;
	}

	std::vector<bool>& PortManager::GetPorts(Transport transport, const std::string& ip)
	{
		MS_TRACE();

		switch (transport)
		{
			case Transport::UDP:
			{
				// If the IP is already handled, return its ports vector.
				if (PortManager::udpBindings.find(ip) != PortManager::udpBindings.end())
					return PortManager::udpBindings[ip];

				// Otherwise add an entry in the map.
				// PortManager::udpBindings.emplace(
				// 	std::make_pair<std::string, std::vector<bool>>(ip, 10, false));

				PortManager::udpBindings.emplace(
					std::piecewise_construct, std::make_tuple(ip), std::make_tuple(10, false));

				// return pair.second;
			}
		}
	}
} // namespace RTC
