#define MS_CLASS "RTC::PlainRtpTransport"
// #define MS_LOG_DEV

#include "RTC/PlainRtpTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Producer.hpp"

namespace RTC
{
	/* Instance methods. */

	PlainRtpTransport::PlainRtpTransport(
	  std::string& id, RTC::Transport::Listener* listener, Options& options)
	  : RTC::Transport::Transport(id, listener), rtcpMux(options.rtcpMux)
	{
		MS_TRACE();

		try
		{
			// This may throw.
			this->udpSocket = new RTC::UdpSocket(this, options.listenIp.ip);

			if (!this->rtcpMux)
			{
				// This may throw.
				this->rtcpUdpSocket = new RTC::UdpSocket(this, options.listenIp.ip);
			}
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR("constructor failed: %s", error.what());

			// Must delete everything since the destructor won't be called.

			if (this->udpSocket)
			{
				delete this->udpSocket;
				this->udpSocket = nullptr;
			}

			if (this->rtcpUdpSocket)
			{
				delete this->rtcpUdpSocket;
				this->rtcpUdpSocket = nullptr;
			}

			throw error;
		}
	}

	PlainRtpTransport::~PlainRtpTransport()
	{
		MS_TRACE();

		if (this->udpSocket != nullptr)
			delete this->udpSocket;

		if (this->rtcpUdpSocket != nullptr)
			delete this->rtcpUdpSocket;

		if (this->tuple != nullptr)
			delete this->tuple;

		if (this->rtcpTuple != nullptr)
			delete this->rtcpTuple;
	}

	void PlainRtpTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add tuple.
		if (this->tuple != nullptr)
		{
			this->tuple->FillJson(jsonObject["tuple"]);
		}
		else
		{
			jsonObject["tuple"] = json::object();

			auto jsonTupleIt = jsonObject.find("tuple");

			(*jsonTupleIt)["localIp"]   = this->udpSocket->GetLocalIp();
			(*jsonTupleIt)["localPort"] = this->udpSocket->GetLocalPort();
			(*jsonTupleIt)["transport"] = "udp";
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

				auto jsonRtcpTupleIt = jsonObject.find("rtcpTuple");

				(*jsonRtcpTupleIt)["localIp"]   = this->rtcpUdpSocket->GetLocalIp();
				(*jsonRtcpTupleIt)["localPort"] = this->rtcpUdpSocket->GetLocalPort();
				(*jsonRtcpTupleIt)["transport"] = "udp";
			}
		}

		// Add headerExtensionIds.
		jsonObject["headerExtensions"] = json::object();
		auto jsonHeaderExtensionsIt    = jsonObject.find("headerExtensions");

		if (this->rtpHeaderExtensionIds.absSendTime != 0u)
			(*jsonHeaderExtensionsIt)["absSendTime"] = this->rtpHeaderExtensionIds.absSendTime;

		if (this->rtpHeaderExtensionIds.mid != 0u)
			(*jsonHeaderExtensionsIt)["mid"] = this->rtpHeaderExtensionIds.mid;

		if (this->rtpHeaderExtensionIds.rid != 0u)
			(*jsonHeaderExtensionsIt)["rid"] = this->rtpHeaderExtensionIds.rid;

		// Add rtpListener.
		this->rtpListener.FillJson(jsonObject["rtpListener"]);
	}

	void PlainRtpTransport::FillJsonStats(json& jsonObject) const
	{
		MS_TRACE();

		// Add type.
		jsonObject["type"] = "transport";

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

		// Add rtcpTuple.
		if (!this->rtcpMux && this->rtcpTuple != nullptr)
			this->rtcpTuple->FillJson(jsonObject["rtcpTuple"]);
	}

	void Router::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_DUMP:
			{
				json data{ json::object() };

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_GET_STATS:
			{
				json data{ json::object() };

				FillJsonStats(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_CONNECT:
			{
				// Ensure this method is not called twice.
				if (this->tuple != nullptr)
				{
					request->Reject("connect() already called");

					return;
				}

				std::string ip;
				uint16_t port{ 0u };
				uint16_t rtcpPort{ 0u };

				try
				{
					auto jsonIpIt = request->data.find("ip");

					if (jsonIpIt == request->data.end() || !jsonIpIt->is_string())
						MS_THROW_ERROR("missing ip");

					// This may throw.
					ip = Utils::IP::NormalizeIp(jsonIpIt->get<std::string>());

					auto jsonPortIt = request->data.find("port");

					if (jsonPortIt == request->data.end() || !jsonPortIt->is_number_unsigned())
						MS_THROW_ERROR("missing port");

					port = jsonPortIt->get<uint16_t>();

					auto jsonRtcpPortIt = request->data.find("rtcpPort");

					if (jsonRtcpPortIt != request->data.end() && jsonRtcpPortIt->is_number_unsigned())
					{
						if (this->rtcpMux)
							MS_THROW_ERROR("cannot set rtcpPort with rtcpMux enabled");

						rtcpPort = jsonRtcpPortIt->get<uint16_t>();
					}
					else if (jsonRtcpPortIt == request->data.end() || !jsonRtcpPortIt->is_number_unsigned())
					{
						if (!this->rtcpMux)
							MS_THROW_ERROR("missing rtcpPort (required with rtcpMux disabled)");
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
							MS_THROW_ERROR("invalid destination IP '%s'", ip.c_str());
						}
					}

					// Create the tuple.
					this->tuple = new RTC::TransportTuple(
					  this->udpSocket, reinterpret_cast<struct sockaddr*>(&this->remoteAddrStorage));

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
								MS_THROW_ERROR("invalid destination IP '%s'", ip.c_str());
							}
						}

						// Create the tuple.
						this->rtcpTuple = new RTC::TransportTuple(
						  this->rtcpUdpSocket, reinterpret_cast<struct sockaddr*>(&this->rtcpRemoteAddrStorage));
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

					request->Reject(error.what());

					return;
				}

				// Start the RTCP timer.
				this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));

				// Tell the caller about the selected local DTLS role.
				json data{ json::object() };

				this->tuple->FillJson(data["tuple"]);

				if (!this->rtcpMux)
					this->rtcpTuple->FillJson(data["rtcpTuple"]);

				request->Accept(data);

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

	void PlainRtpTransport::SendRtpPacket(RTC::RtpPacket* packet)
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
		else
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
		else
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

	inline void PlainRtpTransport::OnRtpDataRecv(
	  RTC::TransportTuple* /*tuple*/, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Apply the Transport RTP header extension ids so the RTP listener can use them.
		packet->SetAbsSendTimeExtensionId(this->rtpHeaderExtensionIds.absSendTime);
		packet->SetMidExtensionId(this->rtpHeaderExtensionIds.mid);
		packet->SetRidExtensionId(this->rtpHeaderExtensionIds.rid);

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
	  RTC::TransportTuple* /*tuple*/, const uint8_t* data, size_t len)
	{
		MS_TRACE();

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

	void PlainRtpTransport::OnPacketRecv(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketRecv(&tuple, data, len);
	}
} // namespace RTC
