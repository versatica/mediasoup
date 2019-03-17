#define MS_CLASS "RTC::PortManager"
// #define MS_LOG_DEV

#include "RTC/PortManager.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include <tuple>   // std:make_tuple()
#include <utility> // std::piecewise_construct

/* Static methods for UV callbacks. */

static inline void onClose(uv_handle_t* handle)
{
	delete handle;
}

inline static void onFakeConnection(uv_stream_t* /*handle*/, int /*status*/)
{
	// Do nothing.
}

namespace RTC
{
	/* Static. */

	static constexpr size_t MaxBindAttempts{ 20 };

	/* Class variables. */

	std::unordered_map<std::string, std::vector<bool>> PortManager::mapUdpIpPorts;
	std::unordered_map<std::string, std::vector<bool>> PortManager::mapTcpIpPorts;

	/* Class methods. */

	uv_handle_t* PortManager::Bind(Transport transport, std::string& ip)
	{
		MS_TRACE();

		// First normalize the IP. This may throw if invalid IP.
		Utils::IP::NormalizeIp(ip);

		int err;
		int family = Utils::IP::GetFamily(ip);
		struct sockaddr_storage bindAddr; // NOLINT(cppcoreguidelines-pro-type-member-init)
		size_t initialPortIdx;
		size_t portIdx;
		size_t attempt{ 0 };
		size_t bindAttempt{ 0 };
		int flags{ 0 };
		std::vector<bool>& ports = PortManager::GetPorts(transport, ip);
		uv_handle_t* uvHandle{ nullptr };
		uint16_t port;
		std::string transportStr;

		switch (transport)
		{
			case Transport::UDP:
				transportStr.assign("udp");
				break;

			case Transport::TCP:
				transportStr.assign("tcp");
				break;
		}

		switch (family)
		{
			case AF_INET:
			{
				err = uv_ip4_addr(ip.c_str(), 0, reinterpret_cast<struct sockaddr_in*>(&bindAddr));

				if (err != 0)
					MS_ABORT("uv_ip4_addr() failed: %s", uv_strerror(err));

				break;
			}

			case AF_INET6:
			{
				err = uv_ip6_addr(ip.c_str(), 0, reinterpret_cast<struct sockaddr_in6*>(&bindAddr));

				if (err != 0)
					MS_ABORT("uv_ip6_addr() failed: %s", uv_strerror(err));

				// Don't also bind into IPv4 when listening in IPv6.
				flags |= UV_UDP_IPV6ONLY;

				break;
			}

			// This cannot happen.
			default:
			{
				MS_ABORT("unknown IP family");
			}
		}

		// Choose a random port to start from.
		initialPortIdx = static_cast<size_t>(Utils::Crypto::GetRandomUInt(
		  static_cast<uint32_t>(0), static_cast<uint32_t>(ports.size() - 1)));

		portIdx = initialPortIdx;

		// Iterate all ports until getting one available. Fail if none found and also
		// if bind() fails N times in theorically available ports.
		while (true)
		{
			++attempt;

			if (portIdx < ports.size() - 1)
				++portIdx;
			else
				portIdx = 0;

			// Check whether this port is not available.
			if (ports[portIdx])
			{
				MS_DEBUG_DEV(
				  "port in use, trying again [%s:%s, attempt:%zu]", transportStr.c_str(), ip.c_str(), attempt);

				// If we have tried all ports in the vector throw.
				if (portIdx == initialPortIdx)
				{
					MS_THROW_ERROR(
					  "no available port [%s:%s, attempt:%zu]", transportStr.c_str(), ip.c_str(), attempt);
				}

				continue;
			}

			// So the found port is the vector position plus the RTC minimum port.
			port = static_cast<uint16_t>(portIdx + Settings::configuration.rtcMinPort);

			// Here we already have a theorically available port. Now let's check
			// whether no other process is binding into it.

			// Set the chosen port into the sockaddr struct.
			switch (family)
			{
				case AF_INET:
					(reinterpret_cast<struct sockaddr_in*>(&bindAddr))->sin_port = htons(port);
					break;

				case AF_INET6:
					(reinterpret_cast<struct sockaddr_in6*>(&bindAddr))->sin6_port = htons(port);
					break;
			}

			// Try to bind on it.
			++bindAttempt;

			switch (transport)
			{
				case Transport::UDP:
					uvHandle = reinterpret_cast<uv_handle_t*>(new uv_udp_t());
					err      = uv_udp_init(DepLibUV::GetLoop(), reinterpret_cast<uv_udp_t*>(uvHandle));
					break;

				case Transport::TCP:
					uvHandle = reinterpret_cast<uv_handle_t*>(new uv_tcp_t());
					err      = uv_tcp_init(DepLibUV::GetLoop(), reinterpret_cast<uv_tcp_t*>(uvHandle));
					break;
			}

			if (err != 0)
			{
				delete uvHandle;

				switch (transport)
				{
					case Transport::UDP:
						MS_THROW_ERROR("uv_udp_init() failed: %s", uv_strerror(err));
						break;

					case Transport::TCP:
						MS_THROW_ERROR("uv_tcp_init() failed: %s", uv_strerror(err));
						break;
				}
			}

			switch (transport)
			{
				case Transport::UDP:
				{
					err = uv_udp_bind(
					  reinterpret_cast<uv_udp_t*>(uvHandle),
					  reinterpret_cast<const struct sockaddr*>(&bindAddr),
					  flags);

					MS_WARN_DEV(
					  "uv_udp_bind() failed [%s:%s, attempt:%zu]: %s",
					  transportStr.c_str(),
					  ip.c_str(),
					  attempt,
					  uv_strerror(err));

					break;
				}

				case Transport::TCP:
				{
					err = uv_tcp_bind(
					  reinterpret_cast<uv_tcp_t*>(uvHandle),
					  reinterpret_cast<const struct sockaddr*>(&bindAddr),
					  flags);

					if (err)
					{
						MS_WARN_DEV(
						  "uv_tcp_bind() failed [%s:%s, attempt:%zu]: %s",
						  transportStr.c_str(),
						  ip.c_str(),
						  attempt,
						  uv_strerror(err));
					}

					// uv_tcp_bind() may succeed even if later uv_listen() fails, so
					// double check it.
					if (err == 0)
					{
						err = uv_listen(
						  reinterpret_cast<uv_stream_t*>(uvHandle),
						  256,
						  static_cast<uv_connection_cb>(onFakeConnection));

						MS_WARN_DEV(
						  "uv_listen() failed [%s:%s, attempt:%zu]: %s",
						  transportStr.c_str(),
						  ip.c_str(),
						  attempt,
						  uv_strerror(err));
					}

					break;
				}
			}

			// If it succeeded, stop here.
			if (err == 0)
				break;

			// If it failed, close the handle and check the reason.
			uv_close(reinterpret_cast<uv_handle_t*>(uvHandle), static_cast<uv_close_cb>(onClose));

			// If bind() fails due to "too many open files" throw.
			if (err == UV_EMFILE)
			{
				MS_THROW_ERROR(
				  "port bind failed due to too many open files [%s:%s, attempt:%zu]",
				  transportStr.c_str(),
				  ip.c_str(),
				  attempt);
			}
			// If cannot bind in the given IP, throw.
			else if (err == UV_EADDRNOTAVAIL)
			{
				MS_THROW_ERROR(
				  "port bind failed due to address not available [%s:%s, attempt:%zu]",
				  transportStr.c_str(),
				  ip.c_str(),
				  attempt);
			}
			// If bind() fails for more that MaxBindAttempts then throw.
			else if (bindAttempt > MaxBindAttempts)
			{
				MS_THROW_ERROR(
				  "port bind failed too many times [%s:%s, attempt:%zu]",
				  transportStr.c_str(),
				  ip.c_str(),
				  attempt);
			}
			// If we have tried all the ports in the range throw.
			else if (portIdx == initialPortIdx)
			{
				MS_THROW_ERROR(
				  "no more available ports [%s:%s, attempt:%zu]", transportStr.c_str(), ip.c_str(), attempt);
			}
			// Otherwise try again.
			else
			{
				continue;
			}
		}

		// Mark the port as unavailable.
		ports[portIdx] = true;

		MS_DEBUG_DEV(
		  "bind succeeded [%s:%s, port:%" PRIu16 ", attempt:%zu]",
		  transportStr.c_str(),
		  ip.c_str(),
		  port,
		  attempt);

		return static_cast<uv_handle_t*>(uvHandle);
	}

	void PortManager::Unbind(Transport transport, std::string& ip, uint16_t port)
	{
		MS_TRACE();

		if (
		  (static_cast<size_t>(port) < Settings::configuration.rtcMinPort) ||
		  (static_cast<size_t>(port) > Settings::configuration.rtcMaxPort))
		{
			MS_ERROR("given port %" PRIu16 " is out of range", port);

			return;
		}

		size_t portIdx = static_cast<size_t>(port) - Settings::configuration.rtcMinPort;

		switch (transport)
		{
			case Transport::UDP:
			{
				auto it = PortManager::mapUdpIpPorts.find(ip);

				if (it == PortManager::mapUdpIpPorts.end())
					return;

				auto& ports = it->second;

				// Mark the port as available.
				ports[portIdx] = false;

				break;
			}

			case Transport::TCP:
			{
				auto it = PortManager::mapTcpIpPorts.find(ip);

				if (it == PortManager::mapTcpIpPorts.end())
					return;

				auto& ports = it->second;

				// Mark the port as available.
				ports[portIdx] = false;

				break;
			}
		}
	}

	std::vector<bool>& PortManager::GetPorts(Transport transport, const std::string& ip)
	{
		MS_TRACE();

		// Make GCC happy so it does not print:
		// "control reaches end of non-void function [-Wreturn-type]"
		static std::vector<bool> emptyPorts;

		switch (transport)
		{
			case Transport::UDP:
			{
				auto it = PortManager::mapUdpIpPorts.find(ip);

				// If the IP is already handled, return its ports vector.
				if (it != PortManager::mapUdpIpPorts.end())
					return it->second;

				// Otherwise add an entry in the map and return it.
				uint16_t numPorts =
				  Settings::configuration.rtcMaxPort - Settings::configuration.rtcMinPort + 1;

				// Emplace a new vector filled with numPorts false values, meaning that
				// all ports are available.
				auto pair = PortManager::mapUdpIpPorts.emplace(
				  std::piecewise_construct, std::make_tuple(ip), std::make_tuple(numPorts, false));

				// pair.first is an iterator to the inserted value.
				return pair.first->second;
			}

			case Transport::TCP:
			{
				auto it = PortManager::mapTcpIpPorts.find(ip);

				// If the IP is already handled, return its ports vector.
				if (it != PortManager::mapTcpIpPorts.end())
					return it->second;

				// Otherwise add an entry in the map and return it.
				uint16_t numPorts =
				  Settings::configuration.rtcMaxPort - Settings::configuration.rtcMinPort + 1;

				// Emplace a new vector filled with numPorts false values, meaning that
				// all ports are available.
				auto pair = PortManager::mapTcpIpPorts.emplace(
				  std::piecewise_construct, std::make_tuple(ip), std::make_tuple(numPorts, false));

				// pair.first is an iterator to the inserted value.
				return pair.first->second;
			}
		}

		return emptyPorts;
	}

	void PortManager::FillJson(json& jsonObject)
	{
		MS_TRACE();

		// Add udp.
		jsonObject["udp"] = json::object();
		auto jsonUdpIt    = jsonObject.find("udp");

		for (auto& kv : PortManager::mapUdpIpPorts)
		{
			auto& ip    = kv.first;
			auto& ports = kv.second;

			(*jsonUdpIt)[ip] = json::array();
			auto jsonIpIt    = jsonUdpIt->find(ip);

			for (size_t i{ 0 }; i < ports.size(); ++i)
			{
				if (!ports[i])
					continue;

				auto port = static_cast<uint16_t>(i + Settings::configuration.rtcMinPort);

				jsonIpIt->push_back(port);
			}
		}

		// Add tcp.
		jsonObject["tcp"] = json::object();
		auto jsonTcpIt    = jsonObject.find("tcp");

		for (auto& kv : PortManager::mapTcpIpPorts)
		{
			auto& ip    = kv.first;
			auto& ports = kv.second;

			(*jsonTcpIt)[ip] = json::array();
			auto jsonIpIt    = jsonTcpIt->find(ip);

			for (size_t i{ 0 }; i < ports.size(); ++i)
			{
				if (!ports[i])
					continue;

				auto port = static_cast<uint16_t>(i + Settings::configuration.rtcMinPort);

				jsonIpIt->emplace_back(port);
			}
		}
	}
} // namespace RTC
