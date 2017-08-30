#define MS_CLASS "RTC::Consumer"
// #define MS_LOG_DEV

#include "RTC/Consumer.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include <vector>

namespace RTC
{
	/* Static. */

	static std::vector<RTC::RtpPacket*> RtpRetransmissionContainer(18);

	/* Instance methods. */

	Consumer::Consumer(
	    Channel::Notifier* notifier,
	    uint32_t consumerId,
	    RTC::Media::Kind kind,
	    uint32_t sourceProducerId,
	    RTC::Transport* transport)
	    : consumerId(consumerId), kind(kind), sourceProducerId(sourceProducerId), notifier(notifier),
	      transport(transport)
	{
		MS_TRACE();

		if (this->kind == RTC::Media::Kind::AUDIO)
			this->maxRtcpInterval = RTC::RTCP::MaxAudioIntervalMs;
		else
			this->maxRtcpInterval = RTC::RTCP::MaxVideoIntervalMs;
	}

	Consumer::~Consumer()
	{
		MS_TRACE();

		delete this->rtpParameters;
		delete this->rtpStream;
	}

	void Consumer::Destroy()
	{
		MS_TRACE();

		// Notify the listeners.
		for (auto& listener : this->listeners)
		{
			listener->OnConsumerClosed(this);
		}

		delete this;
	}

	Json::Value Consumer::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringConsumerId{ "consumerId" };
		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringSourceProducerId{ "sourceProducerId" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringRtpStream{ "rtpStream" };
		static const Json::StaticString JsonStringSupportedCodecPayloadTypes{
			"supportedCodecPayloadTypes"
		};
		static const Json::StaticString JsonStringPaused{ "paused" };

		Json::Value json(Json::objectValue);

		json[JsonStringConsumerId] = Json::UInt{ this->consumerId };

		json[JsonStringKind] = RTC::Media::GetJsonString(this->kind);

		json[JsonStringSourceProducerId] = Json::UInt{ this->sourceProducerId };

		if (this->rtpParameters != nullptr)
			json[JsonStringRtpParameters] = this->rtpParameters->ToJson();
		else
			json[JsonStringRtpParameters] = Json::nullValue;

		if (this->rtpStream != nullptr)
			json[JsonStringRtpStream] = this->rtpStream->ToJson();

		json[JsonStringSupportedCodecPayloadTypes] = Json::arrayValue;

		for (auto payloadType : this->supportedCodecPayloadTypes)
		{
			json[JsonStringSupportedCodecPayloadTypes].append(Json::UInt{ payloadType });
		}

		json[JsonStringPaused] = this->paused;

		return json;
	}

	void Consumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::CONSUMER_SEND:
			{
				static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };

				if (!request->data[JsonStringRtpParameters].isObject())
				{
					request->Reject("missing data.rtpParameters");

					return;
				}

				if (this->rtpParameters)
				{
					request->Reject("Consumer already has RTP parameters");

					return;
				}

				try
				{
					// NOTE: This may throw.
					this->rtpParameters = new RTC::RtpParameters(request->data[JsonStringRtpParameters]);

					for (auto& listener : this->listeners)
					{
						listener->OnConsumerRtpParameters(this);
					}
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// Create RtpStreamSend instance.
				CreateRtpStream(this->rtpParameters->encodings[0]);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_PAUSE:
			{
				this->paused = true;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_RESUME:
			{
				bool wasPaused = this->paused;

				this->paused = false;

				request->Accept();

				if (wasPaused)
				{
					for (auto& listener : this->listeners)
					{
						listener->OnConsumerFullFrameRequired(this);
					}
				}

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void Consumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(IsEnabled(), "Consumer not enabled");

		// TODO: Must refactor for simulcast.
		// Ignore the packet if the SSRC is not the single one in the sender
		// RTP parameters.
		if (packet->GetSsrc() != this->rtpParameters->encodings[0].ssrc)
		{
			MS_WARN_TAG(rtp, "ignoring packet with unknown SSRC [ssrc:%" PRIu32 "]", packet->GetSsrc());

			return;
		}

		// Map the payload type.
		uint8_t payloadType = packet->GetPayloadType();
		auto it             = this->supportedCodecPayloadTypes.find(payloadType);

		// NOTE: This may happen if this peer supports just some codecs from the
		// given RtpParameters.
		// TODO: We should just ignore those packets as they have non been negotiated.
		if (it == this->supportedCodecPayloadTypes.end())
		{
			MS_WARN_DEV("payload type not supported [payloadType:%" PRIu8 "]", payloadType);

			return;
		}

		// Process the packet.
		if (!this->rtpStream->ReceivePacket(packet))
			return;

		// Send the packet.
		this->transport->SendRtpPacket(packet);

		// Update transmitted RTP data counter.
		this->transmittedCounter.Update(packet);
	}

	void Consumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		MS_ASSERT(IsEnabled(), "Consumer not enabled");

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		auto* report = this->rtpStream->GetRtcpSenderReport(now);
		if (report == nullptr)
			return;

		// NOTE: This assumes a single stream.
		uint32_t ssrc     = this->rtpParameters->encodings[0].ssrc;
		std::string cname = this->rtpParameters->rtcp.cname;

		report->SetSsrc(ssrc);
		packet->AddSenderReport(report);

		// Build SDES chunk for this sender.
		auto sdesChunk = new RTC::RTCP::SdesChunk(ssrc);
		auto sdesItem =
		    new RTC::RTCP::SdesItem(RTC::RTCP::SdesItem::Type::CNAME, cname.size(), cname.c_str());

		sdesChunk->AddItem(sdesItem);
		packet->AddSdesChunk(sdesChunk);

		this->lastRtcpSentTime = now;
	}

	void Consumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)
	{
		MS_TRACE();

		MS_ASSERT(IsEnabled(), "Consumer not enabled");

		for (auto it = nackPacket->Begin(); it != nackPacket->End(); ++it)
		{
			RTC::RTCP::FeedbackRtpNackItem* item = *it;

			this->rtpStream->RequestRtpRetransmission(
			    item->GetPacketId(), item->GetLostPacketBitmask(), RtpRetransmissionContainer);

			auto it2 = RtpRetransmissionContainer.begin();
			for (; it2 != RtpRetransmissionContainer.end(); ++it2)
			{
				RTC::RtpPacket* packet = *it2;

				if (packet == nullptr)
					break;

				RetransmitRtpPacket(packet);
			}
		}
	}

	void Consumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		MS_ASSERT(IsEnabled(), "Consumer not enabled");

		this->rtpStream->ReceiveRtcpReceiverReport(report);
	}

	void Consumer::RequestFullFrame()
	{
		MS_TRACE();

		MS_ASSERT(IsEnabled(), "Consumer not enabled");

		for (auto& listener : this->listeners)
		{
			listener->OnConsumerFullFrameRequired(this);
		}
	}

	void Consumer::CreateRtpStream(RTC::RtpEncodingParameters& encoding)
	{
		MS_TRACE();

		uint32_t ssrc = encoding.ssrc;
		// Get the codec of the stream/encoding.
		auto& codec = this->rtpParameters->GetCodecForEncoding(encoding);
		bool useNack{ false };
		bool usePli{ false };

		for (auto& fb : codec.rtcpFeedback)
		{
			if (!useNack && fb.type == "nack")
			{
				MS_DEBUG_TAG(rtcp, "enabling NACK reception");

				useNack = true;
			}
			if (!usePli && fb.type == "nack" && fb.parameter == "pli")
			{
				MS_DEBUG_TAG(rtcp, "enabling PLI reception");

				usePli = true;
			}
		}

		// Create stream params.
		RTC::RtpStream::Params params;

		params.ssrc        = ssrc;
		params.payloadType = codec.payloadType;
		params.mime        = codec.mime;
		params.clockRate   = codec.clockRate;
		params.useNack     = useNack;
		params.usePli      = usePli;

		// Create a RtpStreamSend for sending a single media stream.
		if (useNack)
			this->rtpStream = new RTC::RtpStreamSend(params, 750);
		else
			this->rtpStream = new RTC::RtpStreamSend(params, 0);

		if (encoding.hasRtx && encoding.rtx.ssrc != 0u)
		{
			auto& codec = this->rtpParameters->GetRtxCodecForEncoding(encoding);

			this->rtpStream->SetRtx(codec.payloadType, encoding.rtx.ssrc);
		}
	}

	void Consumer::RetransmitRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(IsEnabled(), "Consumer not enabled");

		RTC::RtpPacket* rtxPacket{ nullptr };

		if (this->rtpStream->HasRtx())
		{
			static uint8_t RtxBuffer[MtuSize];

			rtxPacket = packet->Clone(RtxBuffer);
			this->rtpStream->RtxEncode(rtxPacket);

			MS_DEBUG_TAG(
			    rtx,
			    "sending rtx packet [ssrc: %" PRIu32 " seqnr: %" PRIu16
			    "] recovering original [ssrc: %" PRIu32 " seqnr: %" PRIu16 "]",
			    rtxPacket->GetSsrc(),
			    rtxPacket->GetSequenceNumber(),
			    packet->GetSsrc(),
			    packet->GetSequenceNumber());
		}

		else
		{
			rtxPacket = packet;
			MS_DEBUG_TAG(
			    rtx,
			    "retransmitting packet [ssrc: %" PRIu32 " seqnr: %" PRIu16 "]",
			    rtxPacket->GetSsrc(),
			    rtxPacket->GetSequenceNumber());
		}

		// Update retransmitted RTP data counter.
		this->retransmittedCounter.Update(rtxPacket);

		// Send the packet.
		this->transport->SendRtpPacket(rtxPacket);

		// Delete the RTX RtpPacket if it was created.
		if (rtxPacket != packet)
			delete rtxPacket;
	}
} // namespace RTC
