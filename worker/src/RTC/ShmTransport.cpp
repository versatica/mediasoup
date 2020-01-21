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
		std::string shm;

		// Read shm.name
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

		auto jsonLogNameIt = jsonLogIt->find("fileName");
		if (jsonLogNameIt == jsonLogIt->end())
		  MS_THROW_TYPE_ERROR("missing log.fileName");
		else if (!jsonLogNameIt->is_string())
			MS_THROW_TYPE_ERROR("wrong log.fileName (not a string)");
		
		std::string logname;
		logname.assign(jsonLogNameIt->get<std::string>());

		auto jsonLogLevelIt = jsonLogIt->find("level");
		if (jsonLogLevelIt == jsonLogIt->end())
		  MS_THROW_TYPE_ERROR("missing log.level");
		else if (!jsonLogLevelIt->is_number())
			MS_THROW_TYPE_ERROR("wrong log.level (not a number");

		int loglevel = jsonLogLevelIt->get<int>();

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

		MS_DEBUG_TAG(rtp, "ShmTransport ctor will call DepLibSfuShm::InitializeShmWriterCtx()");
 	  DepLibSfuShm::InitializeShmWriterCtx(shm, logname, loglevel, &this->shmCtx);

		MS_DEBUG_TAG(
			  rtp,
			  "ShmTransport object created: [shm name:%s, logname:%s status: %u", shm.c_str(), logname.c_str(), this->shmCtx.Status());
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

				this->isTransportConnectedCalled = true;
				//TODO: shm smth?
				MS_DEBUG_TAG(rtp, "-=-=-=-=-=-=-=-=-=-=-=-=-=- SHM TRANSPORT isTransportConnectedCalled = true!");

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


	inline bool ShmTransport::IsConnected() const
	{
		return true; //isTransportConnectedCalled; // Problem is, consumer won't ever switch into connected state if transport's IsConnected() is false,
		// However, for the shm transport to be "fully connected" we need both audio and video ssrc to init, this is too late.
		// Consider shmTransport always "connected" i.e. "consumers can become active right after calling transport.consume()"
		// The actual writing into shm will be possible only if "fully connected" i.e. status is SHM_WRT_READY
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

		//TODO: packet->ReadVideoOrientation() will return some data which we could pass to shm...

		// TODO: since we can have > 1 producer and consumer on the same transport, how do I tell if packet is audio or video?!
// removed switch() to shmconsumer!!!

		// Increase send transmission.
		RTC::Transport::DataSent(packet->GetSize());
	}

	void ShmTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsFullyConnected())
			return;

		uint8_t const* data = packet->GetData();
		size_t len          = packet->GetSize();

		// write it out into shm, separate channel
		this->chunk.data = const_cast<uint8_t*> (data);
		this->chunk.len = len;
		this->chunk.rtp_time = 0; // packet->GetTimestamp(); TODO: RTCP packets don't seem to have timestamps or seqIds
		this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = 0; // TODO: what instead of packet->GetSequenceNumber()?
		this->chunk.begin = this->chunk.end = 1;

		DepLibSfuShm::WriteChunk(&this->shmCtx, &chunk, DepLibSfuShm::ShmChunkType::RTCP);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

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
		this->chunk.rtp_time = 0; //packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
		this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = 0; //packet->GetSequenceNumber(); // TODO: are there several seqIds in a compound packet??
		this->chunk.begin = this->chunk.end = 1;

		DepLibSfuShm::WriteChunk(&this->shmCtx, &chunk, DepLibSfuShm::ShmChunkType::RTCP);

		// How to check for validity looping thru all packets in a compound packet: https://tools.ietf.org/html/rfc3550#appendix-A.2

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

  bool ShmTransport::RecvStreamMeta(json& data) const
	{
		MS_TRACE();
/*
		TBD: Assuming that metadata contains shm stream name, need to match with the one used by this transport
		// Read shm name
		auto jsonShmIt = data.find("shm");
		if (jsonShmIt == data.end())
			MS_THROW_TYPE_ERROR("missing shm");
		else if (!jsonShmIt->is_string())
			MS_THROW_TYPE_ERROR("wrong shm (not a string)");

		std::string shmStr;
		shmStr.assign(jsonShmIt->get<std::string>());
		if (shmStr.compare(this->shm) != 0) {
			MS_WARN_DEV("metadata shm %s does not match transport shm %s", shmStr.c_str(), this->shm.c_str() );
			return false;
		}
*/
		// TODO: write stream metadata into shm somehow

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
