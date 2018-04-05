#define MS_CLASS "RTC::PlainRtpTransport"
// #define MS_LOG_DEV

#include "RTC/PlainRtpTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Producer.hpp"

namespace RTC
{
	/* Instance methods. */

	PlainRtpTransport::PlainRtpTransport(
	  RTC::Transport::Listener* listener,
	  Channel::Notifier* notifier,
	  uint32_t transportId,
	  Options& options)
	  : RTC::Transport::Transport(listener, notifier, transportId)
	{
		MS_TRACE();

		int err;

		switch (Utils::IP::GetFamily(options.remoteIP))
		{
			case AF_INET:
			{
				if (!Settings::configuration.hasIPv4 && options.localIP.empty())
					MS_THROW_ERROR("IPv4 disabled");

				err = uv_ip4_addr(
				  options.remoteIP.c_str(),
				  static_cast<int>(options.remotePort),
				  reinterpret_cast<struct sockaddr_in*>(&this->remoteAddrStorage));
				if (err != 0)
					MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));

				if (options.localIP.empty())
					this->udpSocket = new RTC::UdpSocket(this, AF_INET);
				else
					this->udpSocket = new RTC::UdpSocket(this, options.localIP);

				break;
			}

			case AF_INET6:
			{
				if (!Settings::configuration.hasIPv6 && options.localIP.empty())
					MS_THROW_ERROR("IPv6 disabled");

				err = uv_ip6_addr(
				  options.remoteIP.c_str(),
				  static_cast<int>(options.remotePort),
				  reinterpret_cast<struct sockaddr_in6*>(&this->remoteAddrStorage));
				if (err != 0)
					MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));

				if (options.localIP.empty())
					this->udpSocket = new RTC::UdpSocket(this, AF_INET6);
				else
					this->udpSocket = new RTC::UdpSocket(this, options.localIP);

				break;
			}

			default:
			{
				MS_THROW_ERROR("invalid destination IP '%s'", options.remoteIP.c_str());

				break;
			}
		}

		// Create a single tuple.
		this->tuple = new RTC::TransportTuple(
		  this->udpSocket, reinterpret_cast<struct sockaddr*>(&this->remoteAddrStorage));

		// Start the RTCP timer.
		this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));
	}

	PlainRtpTransport::~PlainRtpTransport()
	{
		MS_TRACE();

		delete this->tuple;

		if (this->udpSocket != nullptr)
			this->udpSocket->Destroy();
	}

	Json::Value PlainRtpTransport::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTransportId{ "transportId" };
		static const Json::StaticString JsonStringTuple{ "tuple" };
		static const Json::StaticString JsonStringRtpListener{ "rtpListener" };

		Json::Value json(Json::objectValue);

		// Add transportId.
		json[JsonStringTransportId] = Json::UInt{ this->transportId };

		// Add tuple.
		if (this->tuple != nullptr)
			json[JsonStringTuple] = this->tuple->ToJson();

		// Add rtpListener.
		json[JsonStringRtpListener] = this->rtpListener.ToJson();

		return json;
	}

	Json::Value PlainRtpTransport::GetStats() const
	{
		MS_TRACE();

		Json::Value json(Json::objectValue);

		return json;
	}

	void PlainRtpTransport::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// Mirror RTP if needed.
		if (this->mirrorTuple != nullptr && this->mirroringOptions.sendRtp)
			this->mirrorTuple->Send(data, len);

		this->tuple->Send(data, len);
	}

	void PlainRtpTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// Mirror RTCP if needed.
		if (this->mirrorTuple != nullptr && this->mirroringOptions.sendRtcp)
			this->mirrorTuple->Send(data, len);

		this->tuple->Send(data, len);
	}

	bool PlainRtpTransport::IsConnected() const
	{
		return true;
	}

	void PlainRtpTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// Mirror RTCP if needed.
		if (this->mirrorTuple != nullptr && this->mirroringOptions.sendRtcp)
			this->mirrorTuple->Send(data, len);

		this->tuple->Send(data, len);
	}

	inline void PlainRtpTransport::OnPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Check if it's RTCP.
		if (RTCP::Packet::IsRtcp(data, len))
		{
			OnRtcpDataRecv(tuple, data, len);
		}
		// Check if it's RTP.
		else if (RtpPacket::IsRtp(data, len))
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

		// Mirror RTP if needed.
		if (this->mirrorTuple != nullptr && this->mirroringOptions.recvRtp)
			this->mirrorTuple->Send(data, len);

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Apply the Transport RTP header extension ids so the RTP listener can use them.

		if (this->headerExtensionIds.rid != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::RTP_STREAM_ID, this->headerExtensionIds.rid);
		}
		if (this->headerExtensionIds.absSendTime != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::ABS_SEND_TIME, this->headerExtensionIds.absSendTime);
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

		// Mirror RTCP if needed.
		if (this->mirrorTuple != nullptr && this->mirroringOptions.recvRtcp)
			this->mirrorTuple->Send(data, len);

		RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

			return;
		}

		// Handle each RTCP packet.
		while (packet != nullptr)
		{
			HandleRtcpPacket(packet);

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
