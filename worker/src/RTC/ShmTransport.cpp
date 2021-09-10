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
					"name": "...",
					"queueAge": 100,
					"testNack": 0,
					"reverseIt": 0,
					"shmAppData": "..."
				},
				"log": {
					"name": /var/log/sg/nginx/test_sfu_shm.log",
					"level": 9,
					"stdio": 1
				}
			}
		*/

		MS_DEBUG_TAG(xcode, "ShmTransport ctor[transportId:%s] [%s]", this->id.c_str(), data.dump().c_str());

		// Read shm.name
		std::string shm;
		auto jsonShmIt = data.find("shm");
		if (jsonShmIt == data.end())
			MS_THROW_TYPE_ERROR("missing shm in [%s]", data.dump().c_str());
		else if (!jsonShmIt->is_object())
			MS_THROW_TYPE_ERROR("wrong shm (not an object) in [%s]", data.dump().c_str());

		auto jsonShmNameIt = jsonShmIt->find("name");
		if (jsonShmNameIt == jsonShmIt->end())
			MS_THROW_TYPE_ERROR("missing shm.name in [%s]", data.dump().c_str());
		else if (!jsonShmNameIt->is_string())
			MS_THROW_TYPE_ERROR("wrong shm.name (not a string) in [%s]", data.dump().c_str());

		shm.assign(jsonShmNameIt->get<std::string>());

		// Read shm.queueAge in ms
		auto queueAge = 100;
		auto jsonQueueAgeIt = jsonShmIt->find("queueAge");
		if (jsonQueueAgeIt != jsonShmIt->end())
		{
			if (!jsonQueueAgeIt->is_number())
				MS_THROW_TYPE_ERROR("wrong shm.queueAge (not a number) in [%s]", data.dump().c_str());
			else
				queueAge = jsonQueueAgeIt->get<int>();
		}
		
		// Read shm.testNack in ms, default is 0 which means disabled NACK testing
		auto testNack = 0;
		auto jsonTestNackIt = jsonShmIt->find("testNack");
		if (jsonTestNackIt != jsonShmIt->end())
		{
			if (!jsonTestNackIt->is_number())
				MS_THROW_TYPE_ERROR("wrong shm.testNack (not a number) in [%s]", data.dump().c_str());
			else
				testNack = jsonTestNackIt->get<int>();
		}

		// Perf testing: use forward or reverse iterator to place incoming chunks into video buffer
		bool useReverse = false;
		auto jsonReverseIt = jsonShmIt->find("reverseIt");
		if (jsonReverseIt != jsonShmIt->end() && jsonReverseIt->is_number())
		{
			useReverse = (jsonReverseIt->get<int>() != 0) ? true : false;
		}
		
        // Read shmAppData
        std::string shmAppData;
        auto shmAppDataIt = jsonShmIt->find("shmAppData");
        if (shmAppDataIt != jsonShmIt->end() && shmAppDataIt->is_string()) {
            shmAppData.assign(shmAppDataIt->get<std::string>());
        }

		// ngxshm log name and level
		auto jsonLogIt = data.find("log");
		if (jsonLogIt == data.end())
			MS_THROW_TYPE_ERROR("missing log in [%s]", data.dump().c_str());
		else if(!jsonLogIt->is_object())
			MS_THROW_TYPE_ERROR("wrong log (not an object) in [%s]", data.dump().c_str());

		auto jsonLogNameIt = jsonLogIt->find("name");
		if (jsonLogNameIt == jsonLogIt->end())
		  MS_THROW_TYPE_ERROR("missing log.name in [%s]", data.dump().c_str());
		else if (!jsonLogNameIt->is_string())
			MS_THROW_TYPE_ERROR("wrong log.name (not a string) in [%s]", data.dump().c_str());
		
		std::string logname;
		logname.assign(jsonLogNameIt->get<std::string>());

		auto loglevel = 9; // default log level. TODO: add log level mapping into DepLibSfuShm
		auto jsonLogLevelIt = jsonLogIt->find("level");
		if (jsonLogLevelIt != jsonLogIt->end())
		{
			if (!jsonLogLevelIt->is_number())
				MS_THROW_TYPE_ERROR("wrong log.level (not a number) in [%s]", data.dump().c_str());
			else
				loglevel = jsonLogLevelIt->get<int>();
		}

		// data contains listenIp: {ip: ..., announcedIp: ...}
		auto jsonListenIpIt = data.find("listenIp");
		if (jsonListenIpIt == data.end())
			MS_THROW_TYPE_ERROR("missing listenIp in [%s]", data.dump().c_str());
		else if (!jsonListenIpIt->is_object())
			MS_THROW_TYPE_ERROR("wrong listenIp (not an object) in [%s]", data.dump().c_str());

		auto jsonIpIt = jsonListenIpIt->find("ip");

		if (jsonIpIt == jsonListenIpIt->end())
			MS_THROW_TYPE_ERROR("missing listenIp.ip in [%s]", data.dump().c_str());
		else if (!jsonIpIt->is_string())
			MS_THROW_TYPE_ERROR("wrong listenIp.ip (not a string) in [%s]", data.dump().c_str());

		this->listenIp.ip.assign(jsonIpIt->get<std::string>());

		// This may throw.
		Utils::IP::NormalizeIp(this->listenIp.ip);

		auto jsonAnnouncedIpIt = jsonListenIpIt->find("announcedIp");

		if (jsonAnnouncedIpIt != jsonListenIpIt->end())
		{
			if (!jsonAnnouncedIpIt->is_string())
				MS_THROW_TYPE_ERROR("wrong listenIp.announcedIp (not an string) in [%s]", data.dump().c_str());

			this->listenIp.announcedIp.assign(jsonAnnouncedIpIt->get<std::string>());
		}

 	  this->shmCtx.InitializeShmWriterCtx(shm, queueAge, useReverse, testNack, logname /* + "." + shm + "." + this->id */, loglevel, shmAppData);
	}

	ShmTransport::~ShmTransport()
	{
		MS_TRACE();
		MS_DEBUG_TAG(xcode, "shm[%s] ShmTransport dtor[transportId:%s]", this->shmCtx.StreamName().c_str(), this->id.c_str());
		this->shmCtx.CloseShmWriterCtx();
	}

	void ShmTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);

		jsonObject["shm"] = json::object();
		auto jsonIt = jsonObject.find("shm");

		(*jsonIt)["name"] = this->shmCtx.StreamName().c_str();
		(*jsonIt)["log"] = this->shmCtx.LogName().c_str();
		(*jsonIt)["maxqueueage"] = this->shmCtx.MaxQueuePktDelayMs();
		(*jsonIt)["testnack"] = this->shmCtx.TestNackMs();

		switch (this->shmCtx.Status())
		{
			case DepLibSfuShm::SHM_WRT_READY:
				(*jsonIt)["status"] = "ready";
				break;

			case DepLibSfuShm::SHM_WRT_CLOSED:
				(*jsonIt)["status"] = "closed";
				break;

			case DepLibSfuShm::SHM_WRT_UNDEFINED:
			default:
				(*jsonIt)["status"] = "undefined";
				break;
		}
	}

	void ShmTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		Transport::FillJsonStats(jsonArray);

		auto& jsonObject = jsonArray[0];
		// Add type.
		jsonObject["type"] = "shm-transport";
	}

	void ShmTransport::SendStreamClosed(uint32_t /*ssrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void ShmTransport::RecvStreamClosed(uint32_t /*ssrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void ShmTransport::SendMessage(
	  RTC::DataConsumer* /*dataConsumer*/, uint32_t /*ppid*/, const uint8_t* /*msg*/, size_t /*len*/, onQueuedCallback* /*cb*/)
	{
		MS_TRACE();

		// Do nothing.
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

		if (!IsConnected())
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

		if (!IsConnected())
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

		if (!IsConnected())
			return;

		// Pass it to the parent transport.
		RTC::Transport::ReceiveSctpData(data, len);
	}


	void ShmTransport::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::ChannelRequest::MethodId::TRANSPORT_CONNECT:
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

			case Channel::ChannelRequest::MethodId::TRANSPORT_CONSUME_STREAM_META:
			{
				if (RecvStreamMeta(request->data))
					request->Accept();
				else
					request->Error("ShmTransport::RecvStreamMeta returned false");
				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Transport::HandleRequest(request);
			}
		}
	}


	void ShmTransport::HandleNotification(PayloadChannel::Notification* notification)
	{
		MS_TRACE();

		// Pass it to the parent class.
		RTC::Transport::HandleNotification(notification);
	}


	inline bool ShmTransport::IsConnected() const
	{
		return true;
	}


	void ShmTransport::SendRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet, onSendCallback* /* cb */)
	{
		MS_TRACE();

		if (!IsConnected())
			return;
	
		// Increase send transmission. Consumer writes RTP packets to shm, nothing else to do here.
		RTC::Transport::DataSent(packet->GetSize());
	}


	void ShmTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Increase send transmission.
		RTC::Transport::DataSent(packet->GetSize());
	}


	void ShmTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Increase send transmission.
		RTC::Transport::DataSent(packet->GetSize());
	}


  bool ShmTransport::RecvStreamMeta(json& data)
	{
		MS_TRACE();
/*
					const reqdata = {
						meta: ...,
						shm: ...
					};
*/
		MS_DEBUG_TAG(xcode, "shm[%s] received stream metadata [%s]", this->shmCtx.StreamName().c_str(), data.dump().c_str());

		std::string metadata;
		auto jsonMetaIt = data.find("meta");
		if (jsonMetaIt == data.end())
			MS_THROW_TYPE_ERROR("missing metadata in [%s]", data.dump().c_str());
		else if (!jsonMetaIt->is_string())
			MS_THROW_TYPE_ERROR("wrong metadata (not a string) in [%s]", data.dump().c_str());

		metadata.assign(jsonMetaIt->get<std::string>());

		std::string shm;
		auto jsonShmIt = data.find("shm");
		if (jsonShmIt == data.end())
			MS_THROW_TYPE_ERROR("missing shm name in [%s]", data.dump().c_str());
		else if (!jsonShmIt->is_string())
			MS_THROW_TYPE_ERROR("wrong shm name (not a string) in [%s]", data.dump().c_str());

		shm.assign(jsonShmIt->get<std::string>());

		return (0 == this->shmCtx.WriteStreamMeta(metadata, shm));
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
