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

		auto jsonListenInfosIt = data.find("listenInfos");

		if (jsonListenInfosIt == data.end())
			MS_THROW_TYPE_ERROR("missing listenInfos");
		else if (!jsonListenInfosIt->is_array())
			MS_THROW_TYPE_ERROR("wrong listenInfos (not an array)");
		else if (jsonListenInfosIt->empty())
			MS_THROW_TYPE_ERROR("wrong listenInfos (empty array)");
		else if (jsonListenInfosIt->size() > 8)
			MS_THROW_TYPE_ERROR("wrong listenInfos (too many entries)");

		std::vector<ListenInfo> listenInfos(jsonListenInfosIt->size());

		for (size_t i{ 0 }; i < jsonListenInfosIt->size(); ++i)
		{
			auto& jsonListenInfo = (*jsonListenInfosIt)[i];
			auto& listenInfo     = listenInfos[i];

			if (!jsonListenInfo.is_object())
				MS_THROW_TYPE_ERROR("wrong listenInfo (not an object)");

			auto jsonProtocolIt = jsonListenInfo.find("protocol");

			if (jsonProtocolIt == jsonListenInfo.end())
				MS_THROW_TYPE_ERROR("missing listenInfo.protocol");
			else if (!jsonProtocolIt->is_string())
				MS_THROW_TYPE_ERROR("wrong listenInfo.protocol (not an string");

			std::string protocolStr = jsonProtocolIt->get<std::string>();

			Utils::String::ToLowerCase(protocolStr);

			if (protocolStr == "udp")
				listenInfo.protocol = RTC::TransportTuple::Protocol::UDP;
			else if (protocolStr == "tcp")
				listenInfo.protocol = RTC::TransportTuple::Protocol::TCP;
			else
				MS_THROW_TYPE_ERROR("invalid listenInfo.protocol (must be 'udp' or 'tcp'");

			auto jsonIpIt = jsonListenInfo.find("ip");

			if (jsonIpIt == jsonListenInfo.end())
				MS_THROW_TYPE_ERROR("missing listenInfo.ip");
			else if (!jsonIpIt->is_string())
				MS_THROW_TYPE_ERROR("wrong listenInfo.ip (not an string");

			listenInfo.ip.assign(jsonIpIt->get<std::string>());

			// This may throw.
			Utils::IP::NormalizeIp(listenInfo.ip);

			auto jsonAnnouncedIpIt = jsonListenInfo.find("announcedIp");

			if (jsonAnnouncedIpIt != jsonListenInfo.end())
			{
				if (!jsonAnnouncedIpIt->is_string())
					MS_THROW_TYPE_ERROR("wrong listenInfo.announcedIp (not an string)");

				listenInfo.announcedIp.assign(jsonAnnouncedIpIt->get<std::string>());
			}

			auto jsonPortIt = jsonListenInfo.find("port");

			if (jsonPortIt == jsonListenInfo.end())
				MS_THROW_TYPE_ERROR("missing listenInfo.port");
			else if (!(jsonPortIt->is_number() && Utils::Json::IsPositiveInteger(*jsonPortIt)))
				MS_THROW_TYPE_ERROR("wrong listenInfo.port (not a positive number)");

			listenInfo.port = jsonPortIt->get<uint16_t>();
		}

		try
		{
			for (auto& listenInfo : listenInfos)
			{
				if (listenInfo.protocol == RTC::TransportTuple::Protocol::UDP)
				{
					// This may throw.
					auto* udpSocket = new RTC::UdpSocket(this, listenInfo.ip, listenInfo.port);

					this->udpSocketOrTcpServers.emplace_back(udpSocket, nullptr, listenInfo.announcedIp);
				}
				else if (listenInfo.protocol == RTC::TransportTuple::Protocol::TCP)
				{
					// This may throw.
					auto* tcpServer = new RTC::TcpServer(this, this, listenInfo.ip, listenInfo.port);

					this->udpSocketOrTcpServers.emplace_back(nullptr, tcpServer, listenInfo.announcedIp);
				}
			}
		}
		catch (const MediaSoupError& error)
		{
			// Must delete everything since the destructor won't be called.

			for (auto& item : this->udpSocketOrTcpServers)
			{
				delete item.udpSocket;
				item.udpSocket = nullptr;

				delete item.tcpServer;
				item.tcpServer = nullptr;
			}
			this->udpSocketOrTcpServers.clear();

			throw;
		}
	}

	WebRtcServer::~WebRtcServer()
	{
		MS_TRACE();

		for (auto& item : this->udpSocketOrTcpServers)
		{
			delete item.udpSocket;
			item.udpSocket = nullptr;

			delete item.tcpServer;
			item.tcpServer = nullptr;
		}
		this->udpSocketOrTcpServers.clear();

		// Tell all WebRtcTransports that the WebRtcServer must has been closed.
		// NOTE: The WebRtcTransport destructor will invoke N events in its
		// webRtcTransportListener (this is, WebRtcServer) that will affect these
		// maps so caution.

		for (auto it = this->mapIceUsernameFragmentPasswordWebRtcTransports.begin();
		     it != this->mapIceUsernameFragmentPasswordWebRtcTransports.end();
		     ++it)
		{
			auto* webRtcTransport = it->second;

			this->mapIceUsernameFragmentPasswordWebRtcTransports.erase(it);

			webRtcTransport->WebRtcServerClosed();
		}

		for (auto it = this->mapTupleWebRtcTransports.begin(); it != this->mapTupleWebRtcTransports.end();
		     ++it)
		{
			auto* webRtcTransport = it->second;

			this->mapTupleWebRtcTransports.erase(it);

			webRtcTransport->WebRtcServerClosed();
		}
	}

	void WebRtcServer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// TODO: Dump udpSockets and tcpServers? and webRtcTransports?.
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

	std::vector<RTC::IceCandidate> WebRtcServer::GetIceCandidates(
	  bool enableUdp, bool enableTcp, bool preferUdp, bool preferTcp)
	{
		MS_TRACE();

		std::vector<RTC::IceCandidate> iceCandidates;
		uint16_t iceLocalPreferenceDecrement{ 0 };

		for (auto& item : this->udpSocketOrTcpServers)
		{
			if (item.udpSocket && enableUdp)
			{
				uint16_t iceLocalPreference = IceCandidateDefaultLocalPriority - iceLocalPreferenceDecrement;

				if (preferUdp)
					iceLocalPreference += 1000;

				uint32_t icePriority = generateIceCandidatePriority(iceLocalPreference);

				if (item.announcedIp.empty())
					iceCandidates.emplace_back(item.udpSocket, icePriority);
				else
					iceCandidates.emplace_back(item.udpSocket, icePriority, item.announcedIp);
			}
			else if (item.tcpServer && enableTcp)
			{
				uint16_t iceLocalPreference = IceCandidateDefaultLocalPriority - iceLocalPreferenceDecrement;

				if (preferTcp)
					iceLocalPreference += 1000;

				uint32_t icePriority = generateIceCandidatePriority(iceLocalPreference);

				if (item.announcedIp.empty())
					iceCandidates.emplace_back(item.tcpServer, icePriority);
				else
					iceCandidates.emplace_back(item.tcpServer, icePriority, item.announcedIp);
			}

			// Decrement initial ICE local preference for next IP.
			iceLocalPreferenceDecrement += 100;
		}

		return iceCandidates;
	}

	inline std::string WebRtcServer::GetIceUsernameFragmentPasswordKey(RTC::StunPacket* packet) const
	{
		MS_TRACE();

		// TODO: If this ok? is this gonna return `usernameFragment:password` and those values
		// will match those in the IceServer? Must check.
		// See https://datatracker.ietf.org/doc/html/rfc5389
		// No, it's not ok. See TODO_WEBRTC_SERVER.md file.

		return packet->GetUsername();
	}

	inline std::string WebRtcServer::GetIceUsernameFragmentPasswordKey(
	  const std::string& usernameFragment, const std::string& password) const
	{
		MS_TRACE();

		return usernameFragment + ":" + password;
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

		auto key = GetIceUsernameFragmentPasswordKey(packet);
		auto it  = this->mapIceUsernameFragmentPasswordWebRtcTransports.find(key);

		if (it == this->mapIceUsernameFragmentPasswordWebRtcTransports.end())
		{
			MS_WARN_DEV("ignoring received STUN packet with unknown associated WebRtcTransport");

			delete packet;

			return;
		}

		auto* webRtcTransport = it->second;

		webRtcTransport->ProcessStunPacketFromWebRtcServer(tuple, packet);

		delete packet;
	}

	inline void WebRtcServer::OnNonStunDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		auto it = this->mapTupleWebRtcTransports.find(tuple->id);

		if (it == this->mapTupleWebRtcTransports.end())
		{
			MS_WARN_DEV("ignoring received non STUN data from unknown source");

			return;
		}

		auto* webRtcTransport = it->second;

		webRtcTransport->ProcessNonStunPacketFromWebRtcServer(tuple, data, len);
	}

	inline void WebRtcServer::OnWebRtcTransportIceUsernameFragmentPasswordAdded(
	  RTC::WebRtcTransport* webRtcTransport,
	  const std::string& usernameFragment,
	  const std::string& password)
	{
		MS_TRACE();

		auto key = GetIceUsernameFragmentPasswordKey(usernameFragment, password);

		this->mapIceUsernameFragmentPasswordWebRtcTransports[key] = webRtcTransport;
	}

	inline void WebRtcServer::OnWebRtcTransportIceUsernameFragmentPasswordRemoved(
	  RTC::WebRtcTransport* webRtcTransport,
	  const std::string& usernameFragment,
	  const std::string& password)
	{
		MS_TRACE();

		auto key = GetIceUsernameFragmentPasswordKey(usernameFragment, password);

		this->mapIceUsernameFragmentPasswordWebRtcTransports.erase(key);
	}

	inline void WebRtcServer::OnWebRtcTransportTransportTupleAdded(
	  RTC::WebRtcTransport* webRtcTransport, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		this->mapTupleWebRtcTransports[tuple->id] = webRtcTransport;
	}

	inline void WebRtcServer::OnWebRtcTransportTransportTupleRemoved(
	  RTC::WebRtcTransport* webRtcTransport, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		this->mapTupleWebRtcTransports.erase(tuple->id);
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

		auto it = this->mapTupleWebRtcTransports.find(tuple.id);

		MS_ASSERT(
		  it != this->mapTupleWebRtcTransports.end(),
		  "closed TCP connection not managed by this WebRtcServer");

		auto* webRtcTransport = it->second;

		webRtcTransport->RemoveTuple(&tuple);
	}

	inline void WebRtcServer::OnTcpConnectionPacketReceived(
	  RTC::TcpConnection* connection, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		OnPacketReceived(&tuple, data, len);
	}
} // namespace RTC
