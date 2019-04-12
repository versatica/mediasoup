#define MS_CLASS "RTC::PlainRtpTransport"
// #define MS_LOG_DEV

#include "RTC/PlainRtpTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	PlainRtpTransport::PlainRtpTransport(
	  const std::string& id, RTC::Transport::Listener* listener, json& data)
	  : RTC::Transport::Transport(id, listener)
	{
		MS_TRACE();

		auto jsonListenIpIt = data.find("listenIp");

		if (jsonListenIpIt == data.end())
			MS_THROW_TYPE_ERROR("missing listenIp");
		else if (!jsonListenIpIt->is_object())
			MS_THROW_TYPE_ERROR("wrong listenIp (not an object)");

		auto jsonIpIt = jsonListenIpIt->find("ip");

		if (jsonIpIt == jsonListenIpIt->end())
			MS_THROW_TYPE_ERROR("missing listenIp.ip");
		else if (!jsonIpIt->is_string())
			MS_THROW_TYPE_ERROR("wrong listenIp.ip (not an string)");

		this->listenIp.ip.assign(jsonIpIt->get<std::string>());

		// This may throw.
		Utils::IP::NormalizeIp(this->listenIp.ip);

		auto jsonAnnouncedIpIt = jsonListenIpIt->find("announcedIp");

		if (jsonAnnouncedIpIt != jsonListenIpIt->end())
		{
			if (!jsonAnnouncedIpIt->is_string())
				MS_THROW_TYPE_ERROR("wrong listenIp.announcedIp (not an string");

			this->listenIp.announcedIp.assign(jsonAnnouncedIpIt->get<std::string>());
		}

		auto jsonRtcpMuxIt = data.find("rtcpMux");

		if (jsonRtcpMuxIt != data.end())
		{
			if (!jsonRtcpMuxIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong rtcpMux (not a boolean)");

			this->rtcpMux = jsonRtcpMuxIt->get<bool>();
		}

		auto jsonComediaIt = data.find("comedia");

		if (jsonComediaIt != data.end())
		{
			if (!jsonComediaIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong comedia (not a boolean)");

			this->comedia = jsonComediaIt->get<bool>();
		}

		auto jsonMultiSourceIt = data.find("multiSource");

		if (jsonMultiSourceIt != data.end())
		{
			if (!jsonMultiSourceIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong multiSource (not a boolean)");

			this->multiSource = jsonMultiSourceIt->get<bool>();

			// If multiSource is set disable RTCP-mux and comedia mode.
			if (this->multiSource)
			{
				this->rtcpMux = false;
				this->comedia = false;
			}
		}

		try
		{
			// This may throw.
			this->udpSocket = new RTC::UdpSocket(this, this->listenIp.ip);

			if (!this->rtcpMux)
			{
				// This may throw.
				this->rtcpUdpSocket = new RTC::UdpSocket(this, this->listenIp.ip);
			}
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR("constructor failed: %s", error.what());

			// Must delete everything since the destructor won't be called.

			delete this->udpSocket;
			this->udpSocket = nullptr;

			delete this->rtcpUdpSocket;
			this->rtcpUdpSocket = nullptr;

			throw;
		}
	}

	PlainRtpTransport::~PlainRtpTransport()
	{
		MS_TRACE();

		delete this->udpSocket;

		delete this->rtcpUdpSocket;

		delete this->tuple;

		delete this->rtcpTuple;
	}

	void PlainRtpTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);

		// Add rtcpMux.
		jsonObject["rtcpMux"] = this->rtcpMux;

		// Add comedia.
		jsonObject["comedia"] = this->comedia;

		// Add multiSource.
		jsonObject["multiSource"] = this->multiSource;

		// Add tuple.
		if (this->tuple != nullptr)
		{
			this->tuple->FillJson(jsonObject["tuple"]);
		}
		else
		{
			jsonObject["tuple"] = json::object();
			auto jsonTupleIt    = jsonObject.find("tuple");

			if (this->listenIp.announcedIp.empty())
				(*jsonTupleIt)["localIp"] = this->udpSocket->GetLocalIp();
			else
				(*jsonTupleIt)["localIp"] = this->listenIp.announcedIp;

			(*jsonTupleIt)["localPort"] = this->udpSocket->GetLocalPort();
			(*jsonTupleIt)["protocol"]  = "udp";
		}

		// Add rtcpTuple.
		if (!this->rtcpMux)
		{
			if (this->rtcpTuple != nullptr)
			{
				this->rtcpTuple->FillJson(jsonObject["rtcpTuple"]);
			}
			else
			{
				jsonObject["rtcpTuple"] = json::object();
				auto jsonRtcpTupleIt    = jsonObject.find("rtcpTuple");

				if (this->listenIp.announcedIp.empty())
					(*jsonRtcpTupleIt)["localIp"] = this->rtcpUdpSocket->GetLocalIp();
				else
					(*jsonRtcpTupleIt)["localIp"] = this->listenIp.announcedIp;

				(*jsonRtcpTupleIt)["localPort"] = this->rtcpUdpSocket->GetLocalPort();
				(*jsonRtcpTupleIt)["protocol"]  = "udp";
			}
		}

		// Add headerExtensionIds.
		jsonObject["rtpHeaderExtensions"] = json::object();
		auto jsonRtpHeaderExtensionsIt    = jsonObject.find("rtpHeaderExtensions");

		if (this->rtpHeaderExtensionIds.mid != 0u)
			(*jsonRtpHeaderExtensionsIt)["mid"] = this->rtpHeaderExtensionIds.mid;

		if (this->rtpHeaderExtensionIds.rid != 0u)
			(*jsonRtpHeaderExtensionsIt)["rid"] = this->rtpHeaderExtensionIds.rid;

		if (this->rtpHeaderExtensionIds.rrid != 0u)
			(*jsonRtpHeaderExtensionsIt)["rrid"] = this->rtpHeaderExtensionIds.rrid;

		if (this->rtpHeaderExtensionIds.absSendTime != 0u)
			(*jsonRtpHeaderExtensionsIt)["absSendTime"] = this->rtpHeaderExtensionIds.absSendTime;

		// Add rtpListener.
		this->rtpListener.FillJson(jsonObject["rtpListener"]);
	}

	void PlainRtpTransport::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		jsonArray.emplace_back(json::value_t::object);
		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "plainrtptransport";

		// Add transportId.
		jsonObject["transportId"] = this->id;

		// Add timestamp.
		jsonObject["timestamp"] = DepLibUV::GetTime();

		if (this->tuple != nullptr)
		{
			// Add bytesReceived.
			jsonObject["bytesReceived"] = this->tuple->GetRecvBytes();
			// Add bytesSent.
			jsonObject["bytesSent"] = this->tuple->GetSentBytes();
			// Add tuple.
			this->tuple->FillJson(jsonObject["tuple"]);
		}
		else
		{
			// Add bytesReceived.
			jsonObject["bytesReceived"] = 0;
			// Add bytesSent.
			jsonObject["bytesSent"] = 0;
		}

		// Add rtcpTuple.
		if (!this->rtcpMux && this->rtcpTuple != nullptr)
			this->rtcpTuple->FillJson(jsonObject["rtcpTuple"]);
	}

	void PlainRtpTransport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_CONNECT:
			{
				// Reject if comedia mode or multiSource is set.
				if (this->comedia)
					MS_THROW_ERROR("cannot call connect() when comedia mode is set");
				else if (this->multiSource)
					MS_THROW_ERROR("cannot call connect() when multiSource is set");

				// Ensure this method is not called twice.
				if (this->tuple != nullptr)
					MS_THROW_ERROR("connect() already called");

				try
				{
					std::string ip;
					uint16_t port{ 0u };
					uint16_t rtcpPort{ 0u };

					auto jsonIpIt = request->data.find("ip");

					if (jsonIpIt == request->data.end() || !jsonIpIt->is_string())
						MS_THROW_TYPE_ERROR("missing ip");

					ip = jsonIpIt->get<std::string>();

					// This may throw.
					Utils::IP::NormalizeIp(ip);

					auto jsonPortIt = request->data.find("port");

					if (jsonPortIt == request->data.end() || !jsonPortIt->is_number_unsigned())
						MS_THROW_TYPE_ERROR("missing port");

					port = jsonPortIt->get<uint16_t>();

					auto jsonRtcpPortIt = request->data.find("rtcpPort");

					if (jsonRtcpPortIt != request->data.end() && jsonRtcpPortIt->is_number_unsigned())
					{
						if (this->rtcpMux)
							MS_THROW_TYPE_ERROR("cannot set rtcpPort with rtcpMux enabled");

						rtcpPort = jsonRtcpPortIt->get<uint16_t>();
					}
					else if (jsonRtcpPortIt == request->data.end() || !jsonRtcpPortIt->is_number_unsigned())
					{
						if (!this->rtcpMux)
							MS_THROW_TYPE_ERROR("missing rtcpPort (required with rtcpMux disabled)");
					}

					int err;

					switch (Utils::IP::GetFamily(ip))
					{
						case AF_INET:
						{
							err = uv_ip4_addr(
							  ip.c_str(),
							  static_cast<int>(port),
							  reinterpret_cast<struct sockaddr_in*>(&this->remoteAddrStorage));

							if (err != 0)
								MS_ABORT("uv_ip4_addr() failed: %s", uv_strerror(err));

							break;
						}

						case AF_INET6:
						{
							err = uv_ip6_addr(
							  ip.c_str(),
							  static_cast<int>(port),
							  reinterpret_cast<struct sockaddr_in6*>(&this->remoteAddrStorage));

							if (err != 0)
								MS_ABORT("uv_ip6_addr() failed: %s", uv_strerror(err));

							break;
						}

						default:
						{
							MS_THROW_ERROR("invalid IP '%s'", ip.c_str());
						}
					}

					// Create the tuple.
					this->tuple = new RTC::TransportTuple(
					  this->udpSocket, reinterpret_cast<struct sockaddr*>(&this->remoteAddrStorage));

					if (!this->listenIp.announcedIp.empty())
						this->tuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);

					if (!this->rtcpMux)
					{
						switch (Utils::IP::GetFamily(ip))
						{
							case AF_INET:
							{
								err = uv_ip4_addr(
								  ip.c_str(),
								  static_cast<int>(rtcpPort),
								  reinterpret_cast<struct sockaddr_in*>(&this->rtcpRemoteAddrStorage));

								if (err != 0)
									MS_ABORT("uv_ip4_addr() failed: %s", uv_strerror(err));

								break;
							}

							case AF_INET6:
							{
								err = uv_ip6_addr(
								  ip.c_str(),
								  static_cast<int>(rtcpPort),
								  reinterpret_cast<struct sockaddr_in6*>(&this->rtcpRemoteAddrStorage));

								if (err != 0)
									MS_ABORT("uv_ip6_addr() failed: %s", uv_strerror(err));

								break;
							}

							default:
							{
								MS_THROW_ERROR("invalid IP '%s'", ip.c_str());
							}
						}

						// Create the tuple.
						this->rtcpTuple = new RTC::TransportTuple(
						  this->rtcpUdpSocket, reinterpret_cast<struct sockaddr*>(&this->rtcpRemoteAddrStorage));

						if (!this->listenIp.announcedIp.empty())
							this->rtcpTuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);
					}
				}
				catch (const MediaSoupError& error)
				{
					if (this->tuple != nullptr)
					{
						delete this->tuple;
						this->tuple = nullptr;
					}

					if (this->rtcpTuple != nullptr)
					{
						delete this->rtcpTuple;
						this->rtcpTuple = nullptr;
					}

					throw;
				}

				// Tell the caller about the selected local DTLS role.
				json data(json::object());

				this->tuple->FillJson(data["tuple"]);

				if (!this->rtcpMux)
					this->rtcpTuple->FillJson(data["rtcpTuple"]);

				request->Accept(data);

				// Assume we are connected (there is no much more we can do to know it)
				// and tell the parent class.
				RTC::Transport::Connected();

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Transport::HandleRequest(request);
			}
		}
	}

	inline bool PlainRtpTransport::IsConnected() const
	{
		return this->tuple != nullptr;
	}

	void PlainRtpTransport::SendRtpPacket(RTC::RtpPacket* packet, RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		this->tuple->Send(data, len);
	}

	void PlainRtpTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		if (this->rtcpMux)
			this->tuple->Send(data, len);
		else if (this->rtcpTuple)
			this->rtcpTuple->Send(data, len);
	}

	void PlainRtpTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		if (this->rtcpMux)
			this->tuple->Send(data, len);
		else if (this->rtcpTuple)
			this->rtcpTuple->Send(data, len);
	}

	inline void PlainRtpTransport::OnPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Check if it's RTCP.
		if (RTC::RTCP::Packet::IsRtcp(data, len))
		{
			OnRtcpDataRecv(tuple, data, len);
		}
		// Check if it's RTP.
		else if (RTC::RtpPacket::IsRtp(data, len))
		{
			OnRtpDataRecv(tuple, data, len);
		}
		else
		{
			MS_WARN_DEV("ignoring received packet of unknown type");
		}
	}

	inline void PlainRtpTransport::OnRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// If multiSource is set allow it without any checking.
		if (this->multiSource)
		{
			// Do nothing.
		}
		// If multiSource is not set, check whether we have RTP tuple or whether
		// comedia mode is set.
		else
		{
			// If RTP tuple is unset, set it if we are in comedia mode.
			if (!this->tuple)
			{
				if (!this->comedia)
				{
					MS_DEBUG_TAG(rtp, "ignoring RTP packet while not connected");

					return;
				}

				MS_DEBUG_TAG(rtp, "setting RTP tuple (comedia mode enabled)");

				this->tuple = new RTC::TransportTuple(tuple);

				if (!this->listenIp.announcedIp.empty())
					this->tuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);

				// If not yet connected do it now.
				if (!IsConnected())
					RTC::Transport::Connected();
			}

			// Verify that the packet's tuple matches our RTP tuple.
			if (!this->tuple->Compare(tuple))
			{
				MS_DEBUG_TAG(rtp, "ignoring RTP packet from unknown IP:port");

				return;
			}
		}

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Apply the Transport RTP header extension ids so the RTP listener can use them.
		packet->SetMidExtensionId(this->rtpHeaderExtensionIds.mid);
		packet->SetRidExtensionId(this->rtpHeaderExtensionIds.rid);
		packet->SetRepairedRidExtensionId(this->rtpHeaderExtensionIds.rrid);
		packet->SetAbsSendTimeExtensionId(this->rtpHeaderExtensionIds.absSendTime);

		// Get the associated Producer.
		RTC::Producer* producer = this->rtpListener.GetProducer(packet);

		if (producer == nullptr)
		{
			MS_WARN_TAG(
			  rtp,
			  "no suitable Producer for received RTP packet [ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]",
			  packet->GetSsrc(),
			  packet->GetPayloadType());

			delete packet;

			return;
		}

		MS_DEBUG_DEV(
		  "RTP packet received [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", producer:%" PRIu32 "]",
		  packet->GetSsrc(),
		  packet->GetPayloadType(),
		  producer->producerId);

		// Pass the RTP packet to the corresponding Producer.
		producer->ReceiveRtpPacket(packet);

		delete packet;
	}

	inline void PlainRtpTransport::OnRtcpDataRecv(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// If multiSource is set allow it without any checking.
		if (this->multiSource)
		{
			// Just allow it.
		}
		// If multiSource is not set, check whether we have RTP tuple (if RTCP-mux)
		// or RTCP tuple or whether comedia mode is set.
		else
		{
			// If RTCP-mux and RTP tuple is unset, set it if we are in comedia mode.
			if (this->rtcpMux && !this->tuple)
			{
				if (!this->comedia)
				{
					MS_DEBUG_TAG(rtcp, "ignoring RTCP packet while not connected");

					return;
				}

				MS_DEBUG_TAG(rtp, "setting RTP tuple (comedia mode enabled)");

				this->tuple = new RTC::TransportTuple(tuple);

				if (!this->listenIp.announcedIp.empty())
					this->tuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);

				// If not yet connected do it now.
				if (!IsConnected())
					RTC::Transport::Connected();
			}
			// If no RTCP-mux and RTCP tuple is unset, set it if we are in comedia mode.
			else if (!this->rtcpMux && !this->rtcpTuple)
			{
				if (!this->comedia)
				{
					MS_DEBUG_TAG(rtcp, "ignoring RTCP packet while not connected");

					return;
				}

				MS_DEBUG_TAG(rtcp, "setting RTCP tuple (comedia mode enabled)");

				this->rtcpTuple = new RTC::TransportTuple(tuple);

				if (!this->listenIp.announcedIp.empty())
					this->rtcpTuple->SetLocalAnnouncedIp(this->listenIp.announcedIp);
			}

			// If RTCP-mux verify that the packet's tuple matches our RTP tuple.
			if (this->rtcpMux && !this->tuple->Compare(tuple))
			{
				MS_DEBUG_TAG(rtcp, "ignoring RTCP packet from unknown IP:port");

				return;
			}
			// If no RTCP-mux verify that the packet's tuple matches our RTCP tuple.
			else if (!this->rtcpMux && !this->rtcpTuple->Compare(tuple))
			{
				MS_DEBUG_TAG(rtcp, "ignoring RTCP packet from unknown IP:port");

				return;
			}
		}

		RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

			return;
		}

		// Handle each RTCP packet.
		while (packet != nullptr)
		{
			ReceiveRtcpPacket(packet);

			RTC::RTCP::Packet* previousPacket = packet;

			packet = packet->GetNext();
			delete previousPacket;
		}
	}

	void PlainRtpTransport::UserOnNewProducer(RTC::Producer* /*producer*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PlainRtpTransport::UserOnNewConsumer(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();
	}

	void PlainRtpTransport::UserOnRembFeedback(RTC::RTCP::FeedbackPsRembPacket* /*remb*/)
	{
		MS_TRACE();

		// TODO
	}

	inline void PlainRtpTransport::OnConsumerNeedBitrateChange(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();
	}

	inline void PlainRtpTransport::OnPacketRecv(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketRecv(&tuple, data, len);
	}
} // namespace RTC
