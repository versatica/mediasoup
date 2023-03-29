#define MS_CLASS "RTC::WebRtcServer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/WebRtcServer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
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

	WebRtcServer::WebRtcServer(RTC::Shared* shared, const std::string& id, json& data)
	  : id(id), shared(shared)
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

			uint16_t port{ 0 };
			auto jsonPortIt = jsonListenInfo.find("port");

			if (jsonPortIt != jsonListenInfo.end())
			{
				if (!(jsonPortIt->is_number() && Utils::Json::IsPositiveInteger(*jsonPortIt)))
					MS_THROW_TYPE_ERROR("wrong port (not a positive number)");

				port = jsonPortIt->get<uint16_t>();
			}

			listenInfo.port = port;
		}

		try
		{
			for (auto& listenInfo : listenInfos)
			{
				if (listenInfo.protocol == RTC::TransportTuple::Protocol::UDP)
				{
					// This may throw.
					RTC::UdpSocket* udpSocket;

					if (listenInfo.port != 0)
						udpSocket = new RTC::UdpSocket(this, listenInfo.ip, listenInfo.port);
					else
						udpSocket = new RTC::UdpSocket(this, listenInfo.ip);

					this->udpSocketOrTcpServers.emplace_back(udpSocket, nullptr, listenInfo.announcedIp);
				}
				else if (listenInfo.protocol == RTC::TransportTuple::Protocol::TCP)
				{
					// This may throw.
					RTC::TcpServer* tcpServer;

					if (listenInfo.port != 0)
						tcpServer = new RTC::TcpServer(this, this, listenInfo.ip, listenInfo.port);
					else
						tcpServer = new RTC::TcpServer(this, this, listenInfo.ip);

					this->udpSocketOrTcpServers.emplace_back(nullptr, tcpServer, listenInfo.announcedIp);
				}
			}

			// NOTE: This may throw.
			this->shared->channelMessageRegistrator->RegisterHandler(
			  this->id,
			  /*channelRequestHandler*/ this,
			  /*payloadChannelRequestHandler*/ nullptr,
			  /*payloadChannelNotificationHandler*/ nullptr);
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

		this->shared->channelMessageRegistrator->UnregisterHandler(this->id);

		for (auto& item : this->udpSocketOrTcpServers)
		{
			delete item.udpSocket;
			item.udpSocket = nullptr;

			delete item.tcpServer;
			item.tcpServer = nullptr;
		}
		this->udpSocketOrTcpServers.clear();

		for (auto* webRtcTransport : this->webRtcTransports)
		{
			webRtcTransport->ListenServerClosed();
		}
		this->webRtcTransports.clear();
	}

	void WebRtcServer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add udpSockets and tcpServers.
		jsonObject["udpSockets"] = json::array();
		auto jsonUdpSocketsIt    = jsonObject.find("udpSockets");
		jsonObject["tcpServers"] = json::array();
		auto jsonTcpServersIt    = jsonObject.find("tcpServers");

		size_t udpSocketIdx{ 0 };
		size_t tcpServerIdx{ 0 };

		for (auto& item : this->udpSocketOrTcpServers)
		{
			if (item.udpSocket)
			{
				jsonUdpSocketsIt->emplace_back(json::value_t::object);

				auto& jsonEntry = (*jsonUdpSocketsIt)[udpSocketIdx];

				jsonEntry["ip"]   = item.udpSocket->GetLocalIp();
				jsonEntry["port"] = item.udpSocket->GetLocalPort();

				++udpSocketIdx;
			}
			else if (item.tcpServer)
			{
				jsonTcpServersIt->emplace_back(json::value_t::object);

				auto& jsonEntry = (*jsonTcpServersIt)[tcpServerIdx];

				jsonEntry["ip"]   = item.tcpServer->GetLocalIp();
				jsonEntry["port"] = item.tcpServer->GetLocalPort();

				++tcpServerIdx;
			}
		}

		// Add webRtcTransportIds.
		jsonObject["webRtcTransportIds"] = json::array();
		auto jsonWebRtcTransportIdsIt    = jsonObject.find("webRtcTransportIds");

		for (auto* webRtcTransport : this->webRtcTransports)
		{
			jsonWebRtcTransportIdsIt->emplace_back(webRtcTransport->id);
		}

		size_t idx;

		// Add localIceUsernameFragments.
		jsonObject["localIceUsernameFragments"] = json::array();
		auto jsonLocalIceUsernamesIt            = jsonObject.find("localIceUsernameFragments");

		idx = 0;
		for (auto& kv : this->mapLocalIceUsernameFragmentWebRtcTransport)
		{
			const auto& localIceUsernameFragment = kv.first;
			const auto* webRtcTransport          = kv.second;

			jsonLocalIceUsernamesIt->emplace_back(json::value_t::object);

			auto& jsonEntry = (*jsonLocalIceUsernamesIt)[idx];

			jsonEntry["localIceUsernameFragment"] = localIceUsernameFragment;
			jsonEntry["webRtcTransportId"]        = webRtcTransport->id;

			++idx;
		}

		// Add tupleHashes.
		jsonObject["tupleHashes"] = json::array();
		auto jsonTupleHashesIt    = jsonObject.find("tupleHashes");

		idx = 0;
		for (auto& kv : this->mapTupleWebRtcTransport)
		{
			const auto& tupleHash       = kv.first;
			const auto* webRtcTransport = kv.second;

			jsonTupleHashesIt->emplace_back(json::value_t::object);

			auto& jsonEntry = (*jsonTupleHashesIt)[idx];

			jsonEntry["tupleHash"]         = tupleHash;
			jsonEntry["webRtcTransportId"] = webRtcTransport->id;

			++idx;
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

	inline std::string WebRtcServer::GetLocalIceUsernameFragmentFromReceivedStunPacket(
	  RTC::StunPacket* packet) const
	{
		MS_TRACE();

		// Here we inspect the USERNAME attribute of a received STUN request and
		// extract its remote usernameFragment (the one given to our IceServer as
		// local usernameFragment) which is the first value in the attribute value
		// before the ":" symbol.

		const auto& username  = packet->GetUsername();
		const size_t colonPos = username.find(':');

		// If no colon is found just return the whole USERNAME attribute anyway.
		if (colonPos == std::string::npos)
			return username;

		return username.substr(0, colonPos);
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
			MS_WARN_TAG(ice, "ignoring wrong STUN packet received");

			return;
		}

		// First try doing lookup in the tuples table.
		auto it1 = this->mapTupleWebRtcTransport.find(tuple->hash);

		if (it1 != this->mapTupleWebRtcTransport.end())
		{
			auto* webRtcTransport = it1->second;

			webRtcTransport->ProcessStunPacketFromWebRtcServer(tuple, packet);

			delete packet;

			return;
		}

		// Otherwise try to match the local ICE username fragment.
		auto key = GetLocalIceUsernameFragmentFromReceivedStunPacket(packet);
		auto it2 = this->mapLocalIceUsernameFragmentWebRtcTransport.find(key);

		if (it2 == this->mapLocalIceUsernameFragmentWebRtcTransport.end())
		{
			MS_WARN_TAG(ice, "ignoring received STUN packet with unknown remote ICE usernameFragment");

			delete packet;

			return;
		}

		auto* webRtcTransport = it2->second;

		webRtcTransport->ProcessStunPacketFromWebRtcServer(tuple, packet);

		delete packet;
	}

	inline void WebRtcServer::OnNonStunDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		auto it = this->mapTupleWebRtcTransport.find(tuple->hash);

		if (it == this->mapTupleWebRtcTransport.end())
		{
			MS_WARN_TAG(ice, "ignoring received non STUN data from unknown tuple");

			return;
		}

		auto* webRtcTransport = it->second;

		webRtcTransport->ProcessNonStunPacketFromWebRtcServer(tuple, data, len);
	}

	inline void WebRtcServer::OnWebRtcTransportCreated(RTC::WebRtcTransport* webRtcTransport)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->webRtcTransports.find(webRtcTransport) == this->webRtcTransports.end(),
		  "WebRtcTransport already handled");

		this->webRtcTransports.insert(webRtcTransport);
	}

	inline void WebRtcServer::OnWebRtcTransportClosed(RTC::WebRtcTransport* webRtcTransport)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->webRtcTransports.find(webRtcTransport) != this->webRtcTransports.end(),
		  "WebRtcTransport not handled");

		this->webRtcTransports.erase(webRtcTransport);
	}

	inline void WebRtcServer::OnWebRtcTransportLocalIceUsernameFragmentAdded(
	  RTC::WebRtcTransport* webRtcTransport, const std::string& usernameFragment)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapLocalIceUsernameFragmentWebRtcTransport.find(usernameFragment) ==
		    this->mapLocalIceUsernameFragmentWebRtcTransport.end(),
		  "local ICE username fragment already exists in the table");

		this->mapLocalIceUsernameFragmentWebRtcTransport[usernameFragment] = webRtcTransport;
	}

	inline void WebRtcServer::OnWebRtcTransportLocalIceUsernameFragmentRemoved(
	  RTC::WebRtcTransport* webRtcTransport, const std::string& usernameFragment)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapLocalIceUsernameFragmentWebRtcTransport.find(usernameFragment) !=
		    this->mapLocalIceUsernameFragmentWebRtcTransport.end(),
		  "local ICE username fragment not found in the table");

		this->mapLocalIceUsernameFragmentWebRtcTransport.erase(usernameFragment);
	}

	inline void WebRtcServer::OnWebRtcTransportTransportTupleAdded(
	  RTC::WebRtcTransport* webRtcTransport, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		if (this->mapTupleWebRtcTransport.find(tuple->hash) != this->mapTupleWebRtcTransport.end())
		{
			MS_WARN_TAG(ice, "tuple hash already exists in the table");

			return;
		}

		this->mapTupleWebRtcTransport[tuple->hash] = webRtcTransport;
	}

	inline void WebRtcServer::OnWebRtcTransportTransportTupleRemoved(
	  RTC::WebRtcTransport* webRtcTransport, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		if (this->mapTupleWebRtcTransport.find(tuple->hash) == this->mapTupleWebRtcTransport.end())
		{
			MS_WARN_TAG(ice, "tuple hash not found in the table");

			return;
		}

		this->mapTupleWebRtcTransport.erase(tuple->hash);
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

		// NOTE: We cannot assert whether this tuple is still in our
		// mapTupleWebRtcTransport because this event may be called after the tuple
		// was removed from it.

		auto it = this->mapTupleWebRtcTransport.find(tuple.hash);

		if (it == this->mapTupleWebRtcTransport.end())
			return;

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
