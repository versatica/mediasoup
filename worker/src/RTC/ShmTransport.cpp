#define MS_CLASS "RTC::ShmTransport"

#include "RTC/ShmTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Instance methods. */

	ShmTransport::ShmTransport(const std::string& id, RTC::Transport::Listener* listener, json& data)
	  : RTC::Transport::Transport(id, listener, data)
	{
		MS_TRACE();
		/*
			{ 
				"listenIp": '127.0.0.1',
				"shm": {
					"name": "shmtest"
				},
				"log": {
					"name": /var/log/sg/nginx/test_sfu_shm.log",
					"level": 9,
					"stdio": 1
				}
			}
		*/
		// MS_DEBUG_TAG(rtp, "===============JSON APP DATA: %s", (data.dump()).c_str());
		// Read shm.name
		std::string shm;
		auto jsonShmIt = data.find("shm");
		if (jsonShmIt == data.end())
			MS_THROW_TYPE_ERROR("missing shm");
		else if (!jsonShmIt->is_object())
			MS_THROW_TYPE_ERROR("wrong shm (not an object)");

		auto jsonShmNameIt = jsonShmIt->find("name");
		if (jsonShmNameIt == jsonShmIt->end())
			MS_THROW_TYPE_ERROR("missing shm.name");
		else if (!jsonShmNameIt->is_string())
			MS_THROW_TYPE_ERROR("wrong shm.name (not a string)");

		shm.assign(jsonShmNameIt->get<std::string>());

		// ngxshm log name and level
		auto jsonLogIt = data.find("log");
		if (jsonLogIt == data.end())
			MS_THROW_TYPE_ERROR("missing log");
		else if(!jsonLogIt->is_object())
			MS_THROW_TYPE_ERROR("wrong log (not an object)");

		auto jsonLogNameIt = jsonLogIt->find("name");
		if (jsonLogNameIt == jsonLogIt->end())
		  MS_THROW_TYPE_ERROR("missing log.name");
		else if (!jsonLogNameIt->is_string())
			MS_THROW_TYPE_ERROR("wrong log.name (not a string)");
		
		std::string logname;
		logname.assign(jsonLogNameIt->get<std::string>());

		auto loglevel = 9; // default log level. TODO: add log level mapping into DepLibSfuShm
		auto jsonLogLevelIt = jsonLogIt->find("level");
		if (jsonLogLevelIt != jsonLogIt->end())
		{
			if (!jsonLogLevelIt->is_number())
				MS_THROW_TYPE_ERROR("wrong log.level (not a number");
			else
				loglevel = jsonLogLevelIt->get<int>();
		}

		auto redirect_stdio = 0; // default, otherwise anything but 0 means redirect
		auto jsonLogRedirectIt = jsonLogIt->find("stdio");
		if (jsonLogRedirectIt != jsonLogIt->end())
		{
			if (!jsonLogRedirectIt->is_number())
				MS_THROW_TYPE_ERROR("wrong log.stdio (not a number");
			else
				redirect_stdio = (jsonLogRedirectIt->get<int>() != 0) ? 1 : 0;
		}


		// data contains listenIp: {ip: ..., announcedIp: ...}
		auto jsonListenIpIt = data.find("listenIp");
		if (jsonListenIpIt == data.end())
			MS_THROW_TYPE_ERROR("missing listenIp");
		else if (!jsonListenIpIt->is_object())
			MS_THROW_TYPE_ERROR("wrong listenIp (not an object)");

		auto jsonIpIt = jsonListenIpIt->find("ip");

		if (jsonIpIt == jsonListenIpIt->end())
			MS_THROW_TYPE_ERROR("missing listenIp.ip");
		else if (!jsonIpIt->is_string())
			MS_THROW_TYPE_ERROR("wrong listenIp.ip (not a string)");

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

 	  this->shmCtx.InitializeShmWriterCtx(shm, logname, loglevel, redirect_stdio);
	}

	ShmTransport::~ShmTransport()
	{
		MS_TRACE();
	}

	void ShmTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);

		// TODO: Add some shm info
		jsonObject["shm"] = json::object();
		auto jsonShmIt    = jsonObject.find("shm");
		//(*jsonShmIt)["name"] = this->shm;

		switch (this->shmCtx.Status())
		{
			case DepLibSfuShm::SHM_WRT_READY:
				(*jsonShmIt)["shm_writer"] = "ready";
				break;

			case DepLibSfuShm::SHM_WRT_CLOSED:
				(*jsonShmIt)["shm_writer"] = "closed";
				break;

			case DepLibSfuShm::SHM_WRT_VIDEO_CHNL_CONF_MISSING:
				(*jsonShmIt)["shm_writer"] = "video conf missing";
				break;

			case DepLibSfuShm::SHM_WRT_AUDIO_CHNL_CONF_MISSING:
				(*jsonShmIt)["shm_writer"] = "audio conf missing";
				break;

			case DepLibSfuShm::SHM_WRT_UNDEFINED:
			default:
				(*jsonShmIt)["shm_writer"] = "undefined";
				break;
		}
		//TODO: Add shm log info?
	}

	void ShmTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		jsonArray.emplace_back(json::value_t::object);
		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "shm-transport";

		// Add transportId.
		jsonObject["transportId"] = this->id;

		// Add timestamp.
		jsonObject["timestamp"] = DepLibUV::GetTimeMs();

		//shm
		jsonObject["shm-fully-connected"] = IsFullyConnected();
	}

	inline void ShmTransport::OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Increase receive transmission.
		RTC::Transport::DataReceived(len);

		// Check if it's RTCP.
		if (RTC::RTCP::Packet::IsRtcp(data, len))
		{
			OnRtcpDataReceived(tuple, data, len);
		}
		// Check if it's RTP.
		else if (RTC::RtpPacket::IsRtp(data, len))
		{
			OnRtpDataReceived(tuple, data, len);
		}
		// Check if it's SCTP.
		else if (RTC::SctpAssociation::IsSctp(data, len))
		{
			OnSctpDataReceived(tuple, data, len);
		}
		else
		{
			MS_WARN_DEV("ignoring received packet of unknown type");
		}
	}

	inline void ShmTransport::OnRtpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsFullyConnected())
		{
			return;
		}

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Pass the packet to the parent transport.
		RTC::Transport::ReceiveRtpPacket(packet);
	}

	inline void ShmTransport::OnRtcpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsFullyConnected())
			return;

		RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

			return;
		}

		// Pass the packet to the parent transport.
		RTC::Transport::ReceiveRtcpPacket(packet);
	}

	inline void ShmTransport::OnSctpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsFullyConnected())
			return;

		// Pass it to the parent transport.
		RTC::Transport::ReceiveSctpData(data, len);
	}


	void ShmTransport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_CONNECT:
			{
				if (this->IsConnected())
				{
					MS_THROW_ERROR("transport_connect() already called");
				}
				request->Accept();

				// Tell the parent class.
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


	/*
	Transport's consumer expects 'true' from IsConnected() in order to activate.
	ShmTransport::IsConnected() rather means that transport is initialized but it may not be "fully connected" and ready to write into shm yet.
	To write into shm we wait until both audio and video consumers receive their first RTP packets with ssrc values.
	Call ShmTransport::IsFullyConnected() to detect if ShmTransport is ready to write into shm.
	*/
	inline bool ShmTransport::IsConnected() const
	{
		return true;
	}


	inline bool ShmTransport::IsFullyConnected() const
	{
		return this->shmCtx.Status() == DepLibSfuShm::SHM_WRT_READY;
	}


	void ShmTransport::SendRtpPacket(RTC::RtpPacket* packet, onSendCallback* /* cb */)
	{
		MS_TRACE();

		if (!IsFullyConnected())
			return;
	
		// Increase send transmission. Consumer writes RTP packets to shm, nothing else to do here.
		RTC::Transport::DataSent(packet->GetSize());
	}


	void ShmTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsFullyConnected())
			return;

		uint8_t const* data = packet->GetData();
		size_t len          = packet->GetSize();

		//TODO: write it out into shm, separate channel
		this->chunk.data = const_cast<uint8_t*> (data);
		this->chunk.len = len;
		this->chunk.rtp_time = 0; // packet->GetTimestamp(); TODO: RTCP packets don't seem to have timestamps or seqIds
		this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = 0; // TODO: what instead of packet->GetSequenceNumber()?
		this->chunk.begin = this->chunk.end = 1;

		this->shmCtx.WriteChunk(&chunk, DepLibSfuShm::ShmChunkType::RTCP);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	// Note: according to https://tools.ietf.org/html/rfc3550#section-6.1 all RTCP packets are compound
	void ShmTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsFullyConnected())
			return;

		uint8_t const* data = packet->GetData();
		size_t len          = packet->GetSize();

		// write it out into shm, separate channel
		this->chunk.data = const_cast<uint8_t*> (data);
		this->chunk.len = len;
		this->chunk.rtp_time = 0;
		this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = 0;
		this->chunk.begin = this->chunk.end = 1;

		// How to check for validity looping thru all packets in a compound packet: https://tools.ietf.org/html/rfc3550#appendix-A.2
		this->shmCtx.WriteChunk(&chunk, DepLibSfuShm::ShmChunkType::RTCP);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}


  bool ShmTransport::RecvStreamMeta(json& data) const
	{
		MS_TRACE();
		// TODO: write stream metadata into shm somehow, no API yet

		return true;
	}


	void ShmTransport::SendSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}


	inline void ShmTransport::OnConsumerNeedBitrateChange(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();

		// Do nothing.
	}


	inline void ShmTransport::OnUdpSocketPacketReceived(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketReceived(&tuple, data, len);
	}
} // namespace RTC
