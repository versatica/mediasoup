#define MS_CLASS "RTC::RtpSender"
// #define MS_LOG_DEV

#include "RTC/RtpSender.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "MediaSoupError.hpp"
#include "Logger.hpp"
#include <unordered_set>

namespace RTC
{
	/* Class variables. */

	// Can retransmit up to 17 RTP packets.
	std::vector<RTC::RtpPacket*> RtpSender::rtpRetransmissionContainer(18);

	/* Instance methods. */

	RtpSender::RtpSender(Listener* listener, Channel::Notifier* notifier, uint32_t rtpSenderId, RTC::Media::Kind kind) :
		rtpSenderId(rtpSenderId),
		kind(kind),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MAX_AUDIO_INTERVAL_MS;
		else
			this->maxRtcpInterval = RTC::RTCP::MAX_VIDEO_INTERVAL_MS;
	}

	RtpSender::~RtpSender()
	{
		MS_TRACE();

		if (this->rtpParameters)
			delete this->rtpParameters;

		if (this->rtpStream)
			delete this->rtpStream;
	}

	void RtpSender::Close()
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");

		Json::Value event_data(Json::objectValue);

		event_data[k_class] = "RtpSender";
		this->notifier->Emit(this->rtpSenderId, "close", event_data);

		// Notify the listener.
		this->listener->onRtpSenderClosed(this);

		delete this;
	}

	Json::Value RtpSender::toJson()
	{
		MS_TRACE();

		static Json::Value null_data(Json::nullValue);
		static const Json::StaticString k_rtpSenderId("rtpSenderId");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_hasTransport("hasTransport");
		static const Json::StaticString k_available("available");
		static const Json::StaticString k_supportedPayloadTypes("supportedPayloadTypes");

		Json::Value json(Json::objectValue);

		json[k_rtpSenderId] = (Json::UInt)this->rtpSenderId;

		json[k_kind] = RTC::Media::GetJsonString(this->kind);

		if (this->rtpParameters)
			json[k_rtpParameters] = this->rtpParameters->toJson();
		else
			json[k_rtpParameters] = null_data;

		json[k_hasTransport] = this->transport ? true : false;

		json[k_available] = this->available;

		json[k_supportedPayloadTypes] = Json::arrayValue;

		for (auto payloadType : this->supportedPayloadTypes)
		{
			json[k_supportedPayloadTypes].append((Json::UInt)payloadType);
		}

		return json;
	}

	void RtpSender::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::rtpSender_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void RtpSender::SetPeerCapabilities(RTC::RtpCapabilities* peerCapabilities)
	{
		MS_TRACE();

		MS_ASSERT(peerCapabilities, "given peerCapabilities are null");

		this->peerCapabilities = peerCapabilities;
	}

	void RtpSender::Send(RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_available("available");

		MS_ASSERT(rtpParameters, "no RTP parameters given");

		bool hadParameters = this->rtpParameters ? true : false;

		// Free the previous rtpParameters.
		if (hadParameters)
			delete this->rtpParameters;

		// Delete previous RtpStreamSend (if any).
		if (this->rtpStream)
			delete this->rtpStream;

		// Clone given RTP parameters so we manage our own sender parameters.
		this->rtpParameters = new RTC::RtpParameters(rtpParameters);

		// Remove RTP parameters not supported by this Peer.

		// Remove unsupported codecs.
		for (auto it = this->rtpParameters->codecs.begin(); it != this->rtpParameters->codecs.end();)
		{
			auto& codec = *it;
			auto it2 = this->peerCapabilities->codecs.begin();

			for (; it2 != this->peerCapabilities->codecs.end(); ++it2)
			{
				auto& codecCapability = *it2;

				if (codecCapability.Matches(codec))
					break;
			}

			if (it2 != this->peerCapabilities->codecs.end())
			{
				this->supportedPayloadTypes.insert(codec.payloadType);
				++it;
			}
			else
			{
				it = this->rtpParameters->codecs.erase(it);
			}
		}

		// Remove unsupported encodings.
		for (auto it = this->rtpParameters->encodings.begin(); it != this->rtpParameters->encodings.end();)
		{
			auto& encoding = *it;

			if (supportedPayloadTypes.find(encoding.codecPayloadType) != supportedPayloadTypes.end())
			{
				++it;
			}
			else
			{
				it = this->rtpParameters->encodings.erase(it);
			}
		}

		// Remove unsupported header extensions.
		this->rtpParameters->ReduceHeaderExtensions(this->peerCapabilities->headerExtensions);

		// If there are no encodings set not available.
		if (this->rtpParameters->encodings.size() > 0)
		{
			this->available = true;

			// Set the RtpStreamSend.
			// NOTE: We assume a single stream when sending to remote peers.
			uint32_t streamClockRate = this->rtpParameters->GetEncodingClockRate(0);

			// Create a RtpStreamSend for sending a single media stream.
			switch (this->kind)
			{
				case RTC::Media::Kind::VIDEO:
				case RTC::Media::Kind::DEPTH:
				{
					// Buffer up to 200 packets.
					this->rtpStream = new RTC::RtpStreamSend(streamClockRate, 200);
					break;
				}

				case RTC::Media::Kind::AUDIO:
				{
					// No buffer for audio streams.
					this->rtpStream = new RTC::RtpStreamSend(streamClockRate, 0);
					break;
				}

				default:
					;
			}
		}
		else
		{
			this->available = false;
		}

		// Emit "parameterschange" if these are updated parameters.
		if (hadParameters)
		{
			Json::Value event_data(Json::objectValue);

			event_data[k_class] = "RtpSender";
			event_data[k_rtpParameters] = this->rtpParameters->toJson();
			event_data[k_available] = this->available;

			this->notifier->Emit(this->rtpSenderId, "parameterschange", event_data);
		}
	}

	void RtpSender::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!this->available || !this->transport)
			return;

		MS_ASSERT(this->rtpStream, "no RtpStream set");

		// Process the packet.
		// TODO: Must check what kind of packet we are checking. For example, RTX
		// packets (once implemented) should have a different handling.
		if (!this->rtpStream->ReceivePacket(packet))
			return;

		// Map the payload type.
		uint8_t payloadType = packet->GetPayloadType();
		auto it = this->supportedPayloadTypes.find(payloadType);

		// NOTE: This may happen if this peer supports just some codecs from the
		// given RtpParameters.
		if (it == this->supportedPayloadTypes.end())
		{
			MS_DEBUG_TAG(rtp, "payload type not supported [payloadType:%" PRIu8 "]", payloadType);

			return;
		}

		// Send the packet.
		this->transport->SendRtpPacket(packet);

		// Save RTP data
		transmitted.Update(packet);
	}

	void RtpSender::GetRtcp(RTC::RTCP::CompoundPacket *packet, uint64_t now)
	{
		if (this->rtpStream)
		{
			if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) >= this->maxRtcpInterval)
			{
				RTC::RTCP::SenderReport* report = this->rtpStream->GetRtcpSenderReport(now);

				if (report)
				{
					// TODO: This assumes a single stream for now.
					uint32_t ssrc = this->rtpParameters->GetEncodingMediaSsrc(0);
					std::string cname = this->rtpParameters->rtcp.cname;

					report->SetSsrc(ssrc);
					packet->AddSenderReport(report);

					// Build sdes chunk for this sender.
					RTC::RTCP::SdesChunk *sdesChunk = new RTC::RTCP::SdesChunk(ssrc);
					RTC::RTCP::SdesItem *sdesItem = new RTC::RTCP::SdesItem(RTC::RTCP::SdesItem::Type::CNAME, cname.size(), cname.c_str());

					sdesChunk->AddItem(sdesItem);
					packet->AddSdesChunk(sdesChunk);

					this->lastRtcpSentTime = now;
				}
			}
		}
	}

	void RtpSender::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		if (!this->rtpStream)
		{
			MS_WARN_TAG(rtp, "no RtpStreamSend");

			return;
		}

		for (auto it = nackPacket->Begin(); it != nackPacket->End(); ++it)
		{
			RTC::RTCP::NackItem* item = *it;

			this->rtpStream->RequestRtpRetransmission(item->GetPacketId(), item->GetLostPacketBitmask(), RtpSender::rtpRetransmissionContainer);

			for (auto it = RtpSender::rtpRetransmissionContainer.begin(); it != RtpSender::rtpRetransmissionContainer.end(); ++it)
			{
				RTC::RtpPacket* packet = *it;

				if (packet == nullptr)
					break;

				RetransmitRtpPacket(packet);
			}
		}
	}

	void RtpSender::RetransmitRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!this->available || !this->transport)
			return;

		// If the peer supports RTX create a RTX packet and insert the given media
		// packet as payload. Otherwise just send the packet as usual.
		// TODO: No RTX for now so just send as usual.
		SendRtpPacket(packet);
	}
}
