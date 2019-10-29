#define MS_CLASS "RTC::ShmTransport"
// #define MS_LOG_DEV

#include "RTC/ShmTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	ShmTransport::ShmTransport(const std::string& id, RTC::Transport::Listener* listener, json& data)
	  : RTC::Transport::Transport(id, listener)
	{
		MS_TRACE();

		// Setup sfushm context
		; // this->xcodeshm = new xcodeshm();
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

		// TODO: Add shm info

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
		jsonObject["timestamp"] = DepLibUV::GetTime();

		// Add some stats on shm writing

		// Add bytesReceived.
		jsonObject["bytesReceived"] = RTC::Transport::GetReceivedBytes();

		// Add bytesSent.
		jsonObject["bytesSent"] = RTC::Transport::GetSentBytes();

		// Add recvBitrate.
		jsonObject["recvBitrate"] = RTC::Transport::GetRecvBitrate();

		// Add sendBitrate.
		jsonObject["sendBitrate"] = RTC::Transport::GetSendBitrate();
	}

	void ShmTransport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_CONNECT:
			{
				// Just init shm context
				std::string shmName;
				auto jsonShmName = request->data.find("shm");

				if (jsonShmName == request->data.end() || !jsonShmName->is_string())
					MS_THROW_TYPE_ERROR("missing SHM name");
				
				// TODO: what else do I need to init shm? number of channels? assign media types to chn #?

				shmName = jsonShmName->get<std::string>();

				; //this->xcodeshm->initWriterContext(shmName);

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
		return true; // this->xcodeshm->isWriterCtxInitialized();
	}

	void ShmTransport::SendRtpPacket(
	  RTC::RtpPacket* packet, RTC::Consumer* consumer, bool /*retransmitted*/, bool /*probation*/)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		auto* data = packet->GetPayload();
		auto len   = packet->GetPayloadLength(); // length without RTP padding

		switch (consumer->GetKind())
		{
			case RTC::Media::Kind::AUDIO:
			{
				/* Just write out the whole opus packet without parsing into separate opus frames.
					+----------+--------------+
					|RTP Header| Opus Payload |
					+----------+--------------+
				*/
				; //this->xcodeshm->writeChunk(packet->GetTimestamp(), data, len, SHM::MediaType:AUDIO);

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
						; //this->xcodeshm->writeChunk(packet->GetTimestamp(), data, naluSize, SHM::MediaType:VIDEO);
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
								/*
								The RTP timestamp MUST be set to the earliest of the NALU-times of
      					all the NAL units to be aggregated.
								*/
								; //this->xcodeshm->writeChunk(packet->GetTimestamp(), data + offset, naluSize, SHM::MediaType:VIDEO);
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

						uint8_t subnal   = *(data + 1) & 0x1F; // TODO: review
						uint8_t startBit = *(data + 1) & 0x80;
						uint8_t endBit = 0; // TODO:

						if (startBit == 128) {
						// Open chunk for writing in shm: set its seq number to invalid
							; //startChunkSeqId = this->xcodeshm->openChunkForWrite(packet->GetTimestamp(), SHM::MediaType:VIDEO);

						}
						
						; //this->xcodeshm->addChunkFragment(data + offset, naluSize, SHM::MediaType::VIDEO);

						if (endBit) { // TODO: endBit is set) {
							// complete publishing data chunk: set seqid
							; //this->xcodeshm->PublishChunk(startChunkSeqId, SHM::MediaType::VIDEO);
						}
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

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// write it out into shm, separate channel
		; //this->xcodeshm->writeChunk(packet->GetTimestamp(), data, len, SHM::MediaType:RTCP);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void ShmTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// write it out into shm, separate channel
		; //this->xcodeshm->writeChunk(packet->GetTimestamp(), data, len, SHM::MediaType:RTCP);

		// How to check for validity looping thru all packets in a compound packet: https://tools.ietf.org/html/rfc3550#appendix-A.2

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	inline void ShmTransport::OnPacketReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
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
			MS_DEBUG_TAG(rtp, "ignoring SCTP packet");
		}
		else
		{
			MS_WARN_DEV("ignoring received packet of unknown type");
		}
	}

	inline void ShmTransport::OnRtpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// If multiSource is set allow it without any checking.
		// TODO: see if this is applicable at all
		if (this->multiSource)
		{
			// Do nothing.
		}
		else
		{
			// If not yet connected do it now.
			if (!IsConnected())
				RTC::Transport::Connected();
		}

			// Verify that the packet's tuple matches our RTP tuple.
// TODO: if (!this->tuple->Compare(tuple)) // instead, compare shm names...
//			{
//				MS_DEBUG_TAG(rtp, "ignoring RTP packet from unknown IP:port");
//
//				return;
//			}
//		}

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

		// MS_DEBUG_DEV(
		//   "RTP packet received [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", producerId:%s]",
		//   packet->GetSsrc(),
		//   packet->GetPayloadType(),
		//   producer->id.c_str());

		// Pass the RTP packet to the corresponding Producer
		producer->ReceiveRtpPacket(packet);

		delete packet;
	}

  bool ShmTransport::RecvStreamMeta(json& data) const
	{
		MS_TRACE();
		//return true if shm name passed matches current one, and data is valid (TBD what that means)

		// TODO: write stream metadata into special channel
		return true;
	}

	inline void ShmTransport::OnRtcpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Do nothing.

		// TODO: see if we want to write RTCP messages into shm
	}

	void ShmTransport::UserOnNewProducer(RTC::Producer* /*producer*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void ShmTransport::UserOnNewConsumer(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();
	}

	void ShmTransport::UserOnRembFeedback(RTC::RTCP::FeedbackPsRembPacket* /*remb*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void ShmTransport::UserOnSendSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();
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
