#define MS_CLASS "RTC::WebRtcServer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/WebRtcServer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/ChannelNotifier.hpp"
#include <cmath> // std::pow()

namespace RTC
{
	/* Static. */

	static constexpr uint16_t IceCandidateDefaultLocalPriority{ 10000 };
	// We just provide "host" candidates so type preference is fixed.
	static constexpr uint16_t IceTypePreference{ 64 };
	// We do not support non rtcp-mux so component is always 1.
	static constexpr uint16_t IceComponent{ 1 };

	static inline uint32_t generateIceCandidatePriority(uint16_t localPreference)
	{
		MS_TRACE();

		return std::pow(2, 24) * IceTypePreference + std::pow(2, 8) * localPreference +
		       std::pow(2, 0) * (256 - IceComponent);
	}

	/* Instance methods. */

	WebRtcServer::WebRtcServer(const std::string& id, json& data) : id(id)
	{
		MS_TRACE();

		bool enableUdp{ true };
		auto jsonEnableUdpIt = data.find("enableUdp");

		if (jsonEnableUdpIt != data.end())
		{
			if (!jsonEnableUdpIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong enableUdp (not a boolean)");

			enableUdp = jsonEnableUdpIt->get<bool>();
		}

		bool enableTcp{ false };
		auto jsonEnableTcpIt = data.find("enableTcp");

		if (jsonEnableTcpIt != data.end())
		{
			if (!jsonEnableTcpIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong enableTcp (not a boolean)");

			enableTcp = jsonEnableTcpIt->get<bool>();
		}

		bool preferUdp{ false };
		auto jsonPreferUdpIt = data.find("preferUdp");

		if (jsonPreferUdpIt != data.end())
		{
			if (!jsonPreferUdpIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong preferUdp (not a boolean)");

			preferUdp = jsonPreferUdpIt->get<bool>();
		}

		bool preferTcp{ false };
		auto jsonPreferTcpIt = data.find("preferTcp");

		if (jsonPreferTcpIt != data.end())
		{
			if (!jsonPreferTcpIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong preferTcp (not a boolean)");

			preferTcp = jsonPreferTcpIt->get<bool>();
		}

		auto jsonListenIpPortsIt = data.find("listenIpPorts");

		if (jsonListenIpPortsIt == data.end())
			MS_THROW_TYPE_ERROR("missing listenIpPorts");
		else if (!jsonListenIpPortsIt->is_array())
			MS_THROW_TYPE_ERROR("wrong listenIpPorts (not an array)");
		else if (jsonListenIpPortsIt->empty())
			MS_THROW_TYPE_ERROR("wrong listenIpPorts (empty array)");
		else if (jsonListenIpPortsIt->size() > 8)
			MS_THROW_TYPE_ERROR("wrong listenIpPorts (too many entries)");

		std::vector<ListenIpPort> listenIpPorts(jsonListenIpPortsIt->size());

		for (size_t i{ 0 }; i < jsonListenIpPortsIt->size(); ++i)
		{
			auto& jsonListenIpPort = (*jsonListenIpPortsIt)[i];
			auto& listenIpPort     = listenIpPorts[i];

			if (!jsonListenIpPort.is_object())
				MS_THROW_TYPE_ERROR("wrong listenIpPort (not an object)");

			auto jsonIpIt = jsonListenIpPort.find("ip");

			if (jsonIpIt == jsonListenIpPort.end())
				MS_THROW_TYPE_ERROR("missing listenIpPort.ip");
			else if (!jsonIpIt->is_string())
				MS_THROW_TYPE_ERROR("wrong listenIpPort.ip (not an string");

			listenIpPort.ip.assign(jsonIpIt->get<std::string>());

			// This may throw.
			Utils::IP::NormalizeIp(listenIpPort.ip);

			auto jsonAnnouncedIpIt = jsonListenIpPort.find("announcedIp");

			if (jsonAnnouncedIpIt != jsonListenIpPort.end())
			{
				if (!jsonAnnouncedIpIt->is_string())
					MS_THROW_TYPE_ERROR("wrong listenIpPort.announcedIp (not an string)");

				listenIpPort.announcedIp.assign(jsonAnnouncedIpIt->get<std::string>());
			}

			auto jsonPortIt = jsonListenIpPort.find("port");

			if (jsonPortIt == jsonListenIpPort.end())
				MS_THROW_TYPE_ERROR("missing listenIpPort.port");
			else if (!(jsonPortIt->is_number() && Utils::Json::IsPositiveInteger(*jsonPortIt)))
				MS_THROW_TYPE_ERROR("wrong listenIpPort.port (not a positive number)");

			listenIpPort.port = jsonPortIt->get<uint16_t>();
		}

		try
		{
			uint16_t iceLocalPreferenceDecrement{ 0 };

			if (enableUdp && enableTcp)
				this->iceCandidates.reserve(2 * jsonListenIpPortsIt->size());
			else
				this->iceCandidates.reserve(jsonListenIpPortsIt->size());

			for (auto& listenIpPort : listenIpPorts)
			{
				if (enableUdp)
				{
					uint16_t iceLocalPreference =
					  IceCandidateDefaultLocalPriority - iceLocalPreferenceDecrement;

					if (preferUdp)
						iceLocalPreference += 1000;

					uint32_t icePriority = generateIceCandidatePriority(iceLocalPreference);

					// This may throw.
					auto* udpSocket = new RTC::UdpSocket(this, listenIpPort.ip, listenIpPort.port);

					this->udpSockets[udpSocket] = listenIpPort.announcedIp;

					if (listenIpPort.announcedIp.empty())
						this->iceCandidates.emplace_back(udpSocket, icePriority);
					else
						this->iceCandidates.emplace_back(udpSocket, icePriority, listenIpPort.announcedIp);
				}

				if (enableTcp)
				{
					uint16_t iceLocalPreference =
					  IceCandidateDefaultLocalPriority - iceLocalPreferenceDecrement;

					if (preferTcp)
						iceLocalPreference += 1000;

					uint32_t icePriority = generateIceCandidatePriority(iceLocalPreference);

					// This may throw.
					auto* tcpServer = new RTC::TcpServer(this, this, listenIpPort.ip, listenIpPort.port);

					this->tcpServers[tcpServer] = listenIpPort.announcedIp;

					if (listenIpPort.announcedIp.empty())
						this->iceCandidates.emplace_back(tcpServer, icePriority);
					else
						this->iceCandidates.emplace_back(tcpServer, icePriority, listenIpPort.announcedIp);
				}

				// Decrement initial ICE local preference for next IP.
				iceLocalPreferenceDecrement += 100;
			}
		}
		catch (const MediaSoupError& error)
		{
			// Must delete everything since the destructor won't be called.

			for (auto& kv : this->udpSockets)
			{
				auto* udpSocket = kv.first;

				delete udpSocket;
			}
			this->udpSockets.clear();

			for (auto& kv : this->tcpServers)
			{
				auto* tcpServer = kv.first;

				delete tcpServer;
			}
			this->tcpServers.clear();

			this->iceCandidates.clear();

			throw;
		}
	}

	WebRtcServer::~WebRtcServer()
	{
		MS_TRACE();

		for (auto& kv : this->udpSockets)
		{
			auto* udpSocket = kv.first;

			delete udpSocket;
		}
		this->udpSockets.clear();

		for (auto& kv : this->tcpServers)
		{
			auto* tcpServer = kv.first;

			delete tcpServer;
		}
		this->tcpServers.clear();

		this->iceCandidates.clear();
	}

	void WebRtcServer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add iceCandidates.
		jsonObject["iceCandidates"] = json::array();
		auto jsonIceCandidatesIt    = jsonObject.find("iceCandidates");

		for (size_t i{ 0 }; i < this->iceCandidates.size(); ++i)
		{
			jsonIceCandidatesIt->emplace_back(json::value_t::object);

			auto& jsonEntry    = (*jsonIceCandidatesIt)[i];
			auto& iceCandidate = this->iceCandidates[i];

			iceCandidate.FillJson(jsonEntry);
		}
	}

	void WebRtcServer::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::WEBRTC_SERVER_DUMP:
			{
				json data = json::object();

				FillJson(data);

				request->Accept(data);

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	inline void WebRtcServer::OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (RTC::StunPacket::IsStun(data, len))
		{
			OnStunDataReceived(tuple, data, len);
		}
		else
		{
			OnNonStunDataReceived(tuple, data, len);
		}
	}

	inline void WebRtcServer::OnStunDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::StunPacket* packet = RTC::StunPacket::Parse(data, len);

		if (!packet)
		{
			MS_WARN_DEV("ignoring wrong STUN packet received");

			return;
		}

		// TODO

		delete packet;
	}

	inline void WebRtcServer::OnNonStunDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline void WebRtcServer::OnUdpSocketPacketReceived(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketReceived(&tuple, data, len);
	}

	inline void WebRtcServer::OnRtcTcpConnectionClosed(
	  RTC::TcpServer* /*tcpServer*/, RTC::TcpConnection* connection)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		// TODO
	}

	inline void WebRtcServer::OnTcpConnectionPacketReceived(
	  RTC::TcpConnection* connection, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		OnPacketReceived(&tuple, data, len);
	}

	inline void WebRtcServer::OnIceServerSendStunPacket(
	  const RTC::IceServer* /*iceServer*/, const RTC::StunPacket* packet, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// TODO
	}

	inline void WebRtcServer::OnIceServerSelectedTuple(
	  const RTC::IceServer* /*iceServer*/, RTC::TransportTuple* /*tuple*/)
	{
		MS_TRACE();

		// TODO
	}

	inline void WebRtcServer::OnIceServerConnected(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		// TODO
	}

	inline void WebRtcServer::OnIceServerCompleted(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		// TODO
	}

	inline void WebRtcServer::OnIceServerDisconnected(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		// TODO
	}
} // namespace RTC
