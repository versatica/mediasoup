#ifndef MS_RTC_PORT_MANAGER_HPP
#define MS_RTC_PORT_MANAGER_HPP

#include "common.hpp"
#include "RTC/Transport.hpp"
#include <uv.h>
#include <ankerl/unordered_dense.h>
#include <cstddef>
#include <string>
#include <vector>

namespace RTC
{
	class PortManager
	{
	public:
		enum class Protocol : uint8_t
		{
			UDP = 1,
			TCP
		};

		// Opaque-ish key identifying one (protocol, bind address, port range)
		// tuple. Issued by Bind*() and consumed by Unbind(). Callers store it
		// as an opaque token; equality and hashing are exact-tuple based, so
		// distinct tuples never collide regardless of how close their numeric
		// representations are.
		class PortRangeKey
		{
		public:
			PortRangeKey() = default;
			PortRangeKey(
			  Protocol protocol, const sockaddr_storage& bindAddr, uint16_t minPort, uint16_t maxPort);

			bool operator==(const PortRangeKey& other) const noexcept;
			bool operator!=(const PortRangeKey& other) const noexcept
			{
				return !(*this == other);
			}

		private:
			friend class PortManager;
			friend struct PortRangeKeyHash;

			Protocol protocol{ Protocol::UDP };
			sockaddr_storage bindAddr{};
			uint16_t minPort{ 0u };
			uint16_t maxPort{ 0u };
		};

		struct PortRangeKeyHash
		{
			size_t operator()(const PortRangeKey& key) const noexcept;
		};

	private:
		struct PortRange
		{
			explicit PortRange(uint16_t numPorts, uint16_t minPort)
			  : ports(numPorts, false), minPort(minPort)
			{
			}

			std::vector<bool> ports;
			uint16_t minPort{ 0u };
			uint16_t numUsedPorts{ 0u };
		};

	public:
		static uv_udp_t* BindUdp(std::string& ip, uint16_t port, RTC::Transport::SocketFlags& flags)
		{
			return reinterpret_cast<uv_udp_t*>(Bind(Protocol::UDP, ip, port, flags));
		}
		static uv_udp_t* BindUdp(
		  std::string& ip,
		  uint16_t minPort,
		  uint16_t maxPort,
		  RTC::Transport::SocketFlags& flags,
		  PortRangeKey& key)
		{
			return reinterpret_cast<uv_udp_t*>(Bind(Protocol::UDP, ip, minPort, maxPort, flags, key));
		}
		static uv_tcp_t* BindTcp(std::string& ip, uint16_t port, RTC::Transport::SocketFlags& flags)
		{
			return reinterpret_cast<uv_tcp_t*>(Bind(Protocol::TCP, ip, port, flags));
		}
		static uv_tcp_t* BindTcp(
		  std::string& ip,
		  uint16_t minPort,
		  uint16_t maxPort,
		  RTC::Transport::SocketFlags& flags,
		  PortRangeKey& key)
		{
			return reinterpret_cast<uv_tcp_t*>(Bind(Protocol::TCP, ip, minPort, maxPort, flags, key));
		}
		static void Unbind(const PortRangeKey& key, uint16_t port);
		void Dump(int indentation = 0) const;

	private:
		static uv_handle_t* Bind(
		  Protocol protocol, std::string& ip, uint16_t port, RTC::Transport::SocketFlags& flags);
		static uv_handle_t* Bind(
		  Protocol protocol,
		  std::string& ip,
		  uint16_t minPort,
		  uint16_t maxPort,
		  RTC::Transport::SocketFlags& flags,
		  PortRangeKey& key);
		static PortRange& GetOrCreatePortRange(
		  const PortRangeKey& key, uint16_t minPort, uint16_t maxPort);
		static uint8_t ConvertSocketFlags(RTC::Transport::SocketFlags& flags, Protocol protocol, int family);

	private:
		static thread_local ankerl::unordered_dense::map<PortRangeKey, PortRange, PortRangeKeyHash>
		  mapPortRanges;
	};
} // namespace RTC

#endif
