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
	  RTC::Transport::Listener* listener, uint32_t transportId, Options& options)
	  : RTC::Transport::Transport(listener, transportId)
	{
		MS_TRACE();

		// Remote IP address is provided.
		if (!options.remoteIp.empty())
		{
			int addressFamily = Utils::IP::GetFamily(options.remoteIp);

			if (addressFamily == AF_INET)
				CreateSocket(AF_INET, options.localIp);
			else if (addressFamily == AF_INET6)
				CreateSocket(AF_INET6, options.localIp);
			else
				MS_THROW_ERROR("invalid destination IP '%s'", options.remoteIp.c_str());

			// This may throw.
			try
			{
				SetRemoteParameters(options.remoteIp, options.remotePort);
			}
			catch (const MediaSoupError& error)
			{
				// Close UdpSocket since ~PlainRtpTransport() will not be called.
				delete this->udpSocket;

				throw;
			}
		}
		// No remote IP address is provided.
		else
		{
			// IPv4 is preferred.
			if (options.preferIPv4)
			{
				if (!Settings::configuration.hasIPv4)
					MS_THROW_ERROR("IPv4 disabled");

				CreateSocket(AF_INET, options.localIp);
			}
			// IPv6 is preferred.
			else if (options.preferIPv6)
			{
				if (!Settings::configuration.hasIPv6)
					MS_THROW_ERROR("IPv6 disabled");

				CreateSocket(AF_INET6, options.localIp);
			}
			// No IP family preference, try with IPv4 and then IPv6.
			else if (Settings::configuration.hasIPv4)
			{
				CreateSocket(AF_INET, options.localIp);
			}
			else
			{
				CreateSocket(AF_INET6, options.localIp);
			}
		}
	}

	PlainRtpTransport::~PlainRtpTransport()
	{
		MS_TRACE();

		delete this->tuple;
		delete this->udpSocket;
	}

	Json::Value PlainRtpTransport::ToJson() const
	{
		MS_TRACE();

		// TODO: Redo to have:
		// - .tuple
		//   - .localIp
		//   - .remoteIp
		//   - .localPort
		//   - .remotePort
		//   - .protocol

		static const Json::StaticString JsonStringTransportId{ "transportId" };
		static const Json::StaticString JsonStringTuple{ "tuple" };
		static const Json::StaticString JsonStringRtpListener{ "rtpListener" };
		static const Json::StaticString JsonStringLocalIp{ "localIp" };
		static const Json::StaticString JsonStringLocalPort{ "localPort" };

		Json::Value json(Json::objectValue);

		// Add transportId.
		json[JsonStringTransportId] = Json::UInt{ this->transportId };

		// Add tuple.
		if (this->tuple != nullptr)
			json[JsonStringTuple] = this->tuple->ToJson();

		// TODO: This does not make any sense. This info is supposed to be in the tuple.
		//
		// Local IP address and port.
		json[JsonStringLocalIp]   = this->udpSocket->GetLocalIp();
		json[JsonStringLocalPort] = Json::UInt{ this->udpSocket->GetLocalPort() };

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

	void PlainRtpTransport::SetRemoteParameters(const std::string& ip, const uint16_t port)
	{
		int err;

		switch (Utils::IP::GetFamily(ip))
		{
			case AF_INET:
			{
				if (!Settings::configuration.hasIPv4)
					MS_THROW_ERROR("IPv4 disabled");

				err = uv_ip4_addr(
				  ip.c_str(),
				  static_cast<int>(port),
				  reinterpret_cast<struct sockaddr_in*>(&this->remoteAddrStorage));
				if (err != 0)
					MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));

				break;
			}

			case AF_INET6:
			{
				if (!Settings::configuration.hasIPv6)
					MS_THROW_ERROR("IPv6 disabled");

				err = uv_ip6_addr(
				  ip.c_str(),
				  static_cast<int>(port),
				  reinterpret_cast<struct sockaddr_in6*>(&this->remoteAddrStorage));
				if (err != 0)
					MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));

				break;
			}

			default:
			{
				MS_THROW_ERROR("invalid destination IP '%s'", ip.c_str());
			}
		}

		// Create a single tuple.
		this->tuple = new RTC::TransportTuple(
		  this->udpSocket, reinterpret_cast<struct sockaddr*>(&this->remoteAddrStorage));

		// Start the RTCP timer.
		this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));
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

		this->tuple->Send(data, len);
	}

	void PlainRtpTransport::CreateSocket(int addressFamily, const std::string& localIp)
	{
		if (localIp.empty())
			this->udpSocket = new RTC::UdpSocket(this, addressFamily);
		else
			this->udpSocket = new RTC::UdpSocket(this, localIp);
	}

	bool PlainRtpTransport::IsConnected() const
	{
		return this->tuple != nullptr;
	}

	void PlainRtpTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

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

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Apply the Transport RTP header extension ids so the RTP listener can use them.
		if (this->headerExtensionIds.absSendTime != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::ABS_SEND_TIME, this->headerExtensionIds.absSendTime);
		}
		if (this->headerExtensionIds.mid != 0u)
		{
			packet->AddExtensionMapping(RtpHeaderExtensionUri::Type::MID, this->headerExtensionIds.mid);
		}
		if (this->headerExtensionIds.rid != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::RTP_STREAM_ID, this->headerExtensionIds.rid);
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
