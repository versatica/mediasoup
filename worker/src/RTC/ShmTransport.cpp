#define MS_CLASS "RTC::ShmTransport"
// #define MS_LOG_DEV

#include "RTC/ShmTransport.hpp"
#include "DepLibUV.hpp"
//#include "DepLibSfuShm.hpp"
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

		// Read shm.name
		auto jsonShmIt = data.find("shm");
		if (jsonShmIt == data.end())
			MS_THROW_TYPE_ERROR("missing shm");
		else if (!jsonShmIt->is_object())
			MS_THROW_TYPE_ERROR("wrong shm (not an object)");

		auto jsonShmNameIt = jsonShmIt->find("name");
		if (jsonShmNameIt == data.end())
			MS_THROW_TYPE_ERROR("missing shm.name");
		else if (!jsonShmNameIt->is_string())
			MS_THROW_TYPE_ERROR("wrong shm.name (not a string)");

		this->shm.assign(jsonShmNameIt->get<std::string>());

		// media kind, matching producer media kind
		auto jsonKindIt = jsonShmIt->find("kind");
		if (jsonKindIt == data.end())
			MS_THROW_TYPE_ERROR("missing kind");
		else if (!jsonKindIt->is_string())
			MS_THROW_TYPE_ERROR("wrong kind (not a string)");

		std::string kindStr;
		kindStr.assign(jsonKindIt->get<std::string>());
		if (kindStr.compare("audio") == 0) {
			this->producerKind = RTC::Media::Kind::AUDIO;
		}
		else if (kindStr.compare("video") == 0) {
			this->producerKind = RTC::Media::Kind::VIDEO;
		}
		else {
			MS_THROW_TYPE_ERROR("wrong kind, must be audio or video");
		}

		// ngxshm log name and level
		auto jsonLogIt = data.find("log");
		if (jsonLogIt == data.end())
			MS_THROW_TYPE_ERROR("missing log");
		else if(!jsonLogIt->is_object())
			MS_THROW_TYPE_ERROR("wrong log (not an object)");

		auto jsonLogNameIt = jsonLogIt->find("fileName");
		if (jsonLogNameIt == data.end())
		  MS_THROW_TYPE_ERROR("missing log.fileName");
		else if (!jsonLogNameIt->is_string())
			MS_THROW_TYPE_ERROR("wrong log.fileName (not a string)");
		
		this->logname.assign(jsonLogNameIt->get<std::string>());

		auto jsonLogLevelIt = jsonLogIt->find("level");
		if (jsonLogLevelIt == data.end())
		  MS_THROW_TYPE_ERROR("missing log.level");
		else if (!jsonLogLevelIt->is_number())
			MS_THROW_TYPE_ERROR("wrong log.level (not a number");

		this->loglevel = jsonLogLevelIt->get<int>();

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
	}

	ShmTransport::~ShmTransport()
	{
		MS_TRACE();

		; //delete this->xcodeshm;
	}

	void ShmTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);

		// Add shm info
		jsonObject["shm"] = json::object();
		auto jsonShmIt    = jsonObject.find("shm");
		(*jsonShmIt)["name"] = this->shm;
		if (this->shmCtx) {
			switch(this->shmCtx->wrt_status)
			{
				case DepLibSfuShm::SHM_WRT_INITIALIZED:
					(*jsonShmIt)["writer"] = "initialized";
					break;

				case DepLibSfuShm::SHM_WRT_CLOSED:
					(*jsonShmIt)["writer"] = "closed";
					break;

				case DepLibSfuShm::SHM_WRT_UNDEFINED:
				default:
					(*jsonShmIt)["writer"] = "undefined";
					break;
			}
		}
		else {
			(*jsonShmIt)["writer"] = "null";
		}
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

		//TODO: Add some stats on shm writing
	}

	void ShmTransport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		// TODO: confirm that this class does not have to do anything in case of TRANSPORT_CONNECT
		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_CONNECT:
			{

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
		return true; //TODO: or always false? or what? What is it used for?
	}

	void ShmTransport::SendRtpPacket(RTC::RtpPacket* packet, onSendCallback* /* cb */)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		auto* data = packet->GetPayload();
		auto len   = packet->GetPayloadLength(); // length without RTP padding

		//TODO: packet->ReadVideoOrientation() will return some data which we could pass to shm...

		switch (this->producerKind)
		{
			case RTC::Media::Kind::AUDIO:
			{
				/* Just write out the whole opus packet without parsing into separate opus frames.
					+----------+--------------+
					|RTP Header| Opus Payload |
					+----------+--------------+	*/
				this->chunk.data = data;
				this->chunk.len = len;
				this->chunk.rtp_time = packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
				this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = packet->GetSequenceNumber();
				this->chunk.begin = this->chunk.end = 1;
				DepLibSfuShm::WriteChunk(this->shm, &chunk, DepLibSfuShm::ShmChunkType::AUDIO);

				break;
			} // audio

			case RTC::Media::Kind::VIDEO: //video
			{
				uint8_t nal = *data & 0x1F;

				switch (nal)
				{
					// Single NAL unit packet.
					case 7:
					{
						size_t offset{ 1 };

						len -= 1;
						if (len < 3) {
							MS_WARN_TAG(rtp, "NALU data len <3: %lu", len);
							break;
						}

						auto naluSize  = Utils::Byte::Get2Bytes(data, offset);

						//TODO: Do I worry about padding?
						/*
						  The RTP timestamp is set to the sampling timestamp of the content.
      				A 90 kHz clock rate MUST be used.
						*/

						this->chunk.data = data+offset;
						this->chunk.len = naluSize;
						this->chunk.rtp_time = packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
						this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = packet->GetSequenceNumber();
						this->chunk.begin = this->chunk.end = 1;

						DepLibSfuShm::WriteChunk(this->shm, &chunk, DepLibSfuShm::ShmChunkType::VIDEO);
						break;
					}

					// Aggregation packet. Contains several NAL units in a single RTP packet
					// STAP-A.
					/*
						0                   1                   2                   3
						0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|                          RTP Header                           |
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|                         NALU 1 Data                           |
						:                                                               :
						+               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|               | NALU 2 Size                   | NALU 2 HDR    |
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|                         NALU 2 Data                           |
						:                                                               :
						|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						|                               :...OPTIONAL RTP padding        |
						+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						Figure 7.  An example of an RTP packet including an STAP-A
						containing two single-time aggregation units
					*/
					case 24:
					{
						size_t offset{ 1 };
						len -= 1;
						// Iterate NAL units.
						while (len >= 3)
						{
							auto naluSize  = Utils::Byte::Get2Bytes(data, offset);

							// Check if there is room for the indicated NAL unit size.
							if (len < (naluSize + sizeof(naluSize))) {
								break;
							}
							else {
								/* The RTP timestamp MUST be set to the earliest of the NALU-times of
      					all the NAL units to be aggregated. */
								this->chunk.data = data+offset;
								this->chunk.len = len;
								this->chunk.rtp_time = packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
								this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = packet->GetSequenceNumber(); // TODO: does NALU have its own seqId?
								this->chunk.begin = this->chunk.end = 1;

								DepLibSfuShm::WriteChunk(this->shm, &chunk, DepLibSfuShm::ShmChunkType::VIDEO);
							}

							offset += naluSize + sizeof(naluSize);
							len -= naluSize + sizeof(naluSize);
						}
						break;
					}

					// Fragmentation unit. Single NAL unit is spreaded accross several RTP packets.
					// FU-A
					case 29:
					{
						size_t offset{ 1 };
						auto naluSize  = Utils::Byte::Get2Bytes(data, offset);

						uint8_t subnal   = *(data + 1) & 0x1F; // Last 5 bits in FU header, subtype of FU unit, we don't use it
						uint8_t startBit = *(data + 1) & 0x80; // 1st bit indicates start of fragmented NALU
						uint8_t endBit = *(data + 1) & 0x40; ; // 2nd bit indicates end

						this->chunk.data = data+offset;
						this->chunk.len = naluSize;
						this->chunk.rtp_time = packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
						this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = packet->GetSequenceNumber();
						this->chunk.begin = (startBit == 128)? 1 : 0;
						this->chunk.end = (endBit) ? 1 : 0;

						DepLibSfuShm::WriteChunk(this->shm, &chunk, DepLibSfuShm::ShmChunkType::VIDEO);
						break;
					}
					case 25: // STAB-B
					case 26: // MTAP-16
					case 27: // MTAP-24
					case 28: // FU-B
					{
						MS_WARN_TAG(rtp, "Unsupported NAL unit type %d", nal);
						break;
					}
					default: // ignore the rest
						break;
				}
				break;
			} // video

			default:
			  // RTC::Media::Kind::ALL
			  break;
		}
		
		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void ShmTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		uint8_t const* data = packet->GetData();
		size_t len          = packet->GetSize();

		// write it out into shm, separate channel
		this->chunk.data = const_cast<uint8_t*> (data);
		this->chunk.len = len;
		this->chunk.rtp_time = 0; // packet->GetTimestamp(); TODO: RTCP packets don't seem to have timestamps or seqIds
		this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = 0; // TODO: what instead of packet->GetSequenceNumber()?
		this->chunk.begin = this->chunk.end = 1;

		DepLibSfuShm::WriteChunk(this->shm, &chunk, DepLibSfuShm::ShmChunkType::RTCP);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void ShmTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		uint8_t const* data = packet->GetData();
		size_t len          = packet->GetSize();

		// write it out into shm, separate channel
		this->chunk.data = const_cast<uint8_t*> (data);
		this->chunk.len = len;
		this->chunk.rtp_time = 0; //packet->GetTimestamp(); // TODO: recalculate into actual one including cycles info from RTPStream
		this->chunk.first_rtp_seq = this->chunk.last_rtp_seq = 0; //packet->GetSequenceNumber(); // TODO: are there several seqIds in a compound packet??
		this->chunk.begin = this->chunk.end = 1;

		DepLibSfuShm::WriteChunk(this->shm, &chunk, DepLibSfuShm::ShmChunkType::RTCP);

		// How to check for validity looping thru all packets in a compound packet: https://tools.ietf.org/html/rfc3550#appendix-A.2

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

  bool ShmTransport::RecvStreamMeta(json& data) const
	{
		MS_TRACE();

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

		// Do nothing.
	}
} // namespace RTC
