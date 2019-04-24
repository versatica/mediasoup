#define MS_CLASS "RTC::PipeTransport"
// #define MS_LOG_DEV

#include "RTC/PipeTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	PipeTransport::PipeTransport(const std::string& id, RTC::Transport::Listener* listener, json& data)
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

		try
		{
			// This may throw.
			this->udpSocket = new RTC::UdpSocket(this, this->listenIp.ip);
		}
		catch (const MediaSoupError& error)
		{
			// Must delete everything since the destructor won't be called.

			delete this->udpSocket;
			this->udpSocket = nullptr;

			throw;
		}
	}

	PipeTransport::~PipeTransport()
	{
		MS_TRACE();

		delete this->udpSocket;

		delete this->tuple;
	}

	void PipeTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);

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

		// Add rtpListener.
		this->rtpListener.FillJson(jsonObject["rtpListener"]);
	}

	void PipeTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		jsonArray.emplace_back(json::value_t::object);
		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "pipe-transport";

		// Add transportId.
		jsonObject["transportId"] = this->id;

		// Add timestamp.
		jsonObject["timestamp"] = DepLibUV::GetTime();

		if (this->tuple != nullptr)
		{
			this->tuple->FillJson(jsonObject["tuple"]);
		}
		else
		{
			// Add tuple.
			jsonObject["tuple"] = json::object();
			auto jsonTupleIt    = jsonObject.find("tuple");

			if (this->listenIp.announcedIp.empty())
				(*jsonTupleIt)["localIp"] = this->udpSocket->GetLocalIp();
			else
				(*jsonTupleIt)["localIp"] = this->listenIp.announcedIp;

			(*jsonTupleIt)["localPort"] = this->udpSocket->GetLocalPort();
			(*jsonTupleIt)["protocol"]  = "udp";
		}

		// Add bytesReceived.
		jsonObject["bytesReceived"] = RTC::Transport::GetReceivedBytes();

		// Add bytesSent.
		jsonObject["bytesSent"] = RTC::Transport::GetSentBytes();

		// Add recvBitrate.
		jsonObject["recvBitrate"] = RTC::Transport::GetRecvBitrate();

		// Add sendBitrate.
		jsonObject["sendBitrate"] = RTC::Transport::GetSendBitrate();
	}

	void PipeTransport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_CONNECT:
			{
				// Ensure this method is not called twice.
				if (this->tuple != nullptr)
					MS_THROW_ERROR("connect() already called");

				try
				{
					std::string ip;
					uint16_t port{ 0u };

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
				}
				catch (const MediaSoupError& error)
				{
					if (this->tuple != nullptr)
					{
						delete this->tuple;
						this->tuple = nullptr;
					}

					throw;
				}

				// Tell the caller about the selected local DTLS role.
				json data = json::object();

				this->tuple->FillJson(data["tuple"]);

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

	inline bool PipeTransport::IsConnected() const
	{
		return this->tuple != nullptr;
	}

	void PipeTransport::SendRtpPacket(
	  RTC::RtpPacket* packet, RTC::Consumer* /*consumer*/, bool /*retransmitted*/)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		this->tuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PipeTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		this->tuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void PipeTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		this->tuple->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	inline void PipeTransport::OnPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Increase receive transmission.
		RTC::Transport::DataReceived(len);

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

	inline void PipeTransport::OnRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Verify that the packet's tuple matches our tuple.
		if (!this->tuple->Compare(tuple))
		{
			MS_DEBUG_TAG(rtp, "ignoring RTP packet from unknown IP:port");

			return;
		}

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

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

		// MS_DEBUG_DEV(
		//   "RTP packet received [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", producerId:%s]",
		//   packet->GetSsrc(),
		//   packet->GetPayloadType(),
		//   producer->id.c_str());

		// Pass the RTP packet to the corresponding Producer.
		producer->ReceiveRtpPacket(packet);

		delete packet;
	}

	inline void PipeTransport::OnRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Verify that the packet's tuple matches our tuple.
		if (!this->tuple->Compare(tuple))
		{
			MS_DEBUG_TAG(rtcp, "ignoring RTCP packet from unknown IP:port");

			return;
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

	void PipeTransport::UserOnNewProducer(RTC::Producer* /*producer*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeTransport::UserOnNewConsumer(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();
	}

	void PipeTransport::UserOnRembFeedback(RTC::RTCP::FeedbackPsRembPacket* /*remb*/)
	{
		MS_TRACE();

		// TODO
	}

	inline void PipeTransport::OnConsumerNeedBitrateChange(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();

		// TODO
	}

	inline void PipeTransport::OnPacketRecv(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketRecv(&tuple, data, len);
	}
} // namespace RTC
