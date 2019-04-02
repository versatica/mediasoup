#define MS_CLASS "RTC::PipeConsumer"
// #define MS_LOG_DEV

#include "RTC/PipeConsumer.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	/* Instance methods. */

	PipeConsumer::PipeConsumer(const std::string& id, RTC::Consumer::Listener* listener, json& data)
	  : RTC::Consumer::Consumer(id, listener, data, RTC::RtpParameters::Type::PIPE)
	{
		MS_TRACE();

		// Ensure there are as many encodings as consumable encodings.
		if (this->rtpParameters.encodings.size() != this->consumableRtpEncodings.size())
			MS_THROW_TYPE_ERROR("number of rtpParameters.encodings and consumableRtpEncodings do not match");

		// Create RtpStreamSend instances.
		CreateRtpStreams();
	}

	PipeConsumer::~PipeConsumer()
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto* rtpStream = kv.second;

			delete rtpStream;
		}
		this->mapMappedSsrcRtpStream.clear();
	}

	void PipeConsumer::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Consumer::FillJson(jsonObject);
	}

	void PipeConsumer::FillJsonStats(json& jsonArray) const
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::FillJsonScore(json& jsonObject) const
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
			{
				RequestKeyFrame();

				request->Accept();

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Consumer::HandleRequest(request);
			}
		}
	}

	void PipeConsumer::UseBandwidth(uint32_t availableBandwidth)
	{
		MS_TRACE();

		RequestKeyFrame();
	}

	void PipeConsumer::ProducerRtpStream(RTC::RtpStream* /*rtpStream*/, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::ProducerNewRtpStream(RTC::RtpStream* /*rtpStream*/, uint32_t /*mappedSsrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::ProducerRtpStreamScore(RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void PipeConsumer::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		auto* rtpStream = this->mapMappedSsrcRtpStream.at(packet->GetSsrc());

		// Process the packet.
		if (rtpStream->ReceivePacket(packet))
		{
			// Send the packet.
			this->listener->OnConsumerSendRtpPacket(this, packet);
		}
		else
		{
			MS_WARN_TAG(
			  rtp,
			  "failed to send packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());
		}
	}

	void PipeConsumer::GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)
	{
		MS_TRACE();

		if (static_cast<float>((now - this->lastRtcpSentTime) * 1.15) < this->maxRtcpInterval)
			return;

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream = kv.second;
			auto* report    = rtpStream->GetRtcpSenderReport(now);

			if (!report)
				continue;

			packet->AddSenderReport(report);

			// Build SDES chunk for this sender.
			auto* sdesChunk = rtpStream->GetRtcpSdesChunk();

			packet->AddSdesChunk(sdesChunk);

			this->lastRtcpSentTime = now;
		}
	}

	void PipeConsumer::NeedWorstRemoteFractionLost(uint32_t /*mappedSsrc*/, uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();

		if (!IsActive())
			return;

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream   = kv.second;
			auto fractionLost = rtpStream->GetFractionLost();

			// If our fraction lost is worse than the given one, update it.
			if (fractionLost > worstRemoteFractionLost)
				worstRemoteFractionLost = fractionLost;
		}
	}

	void PipeConsumer::ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* /*nackPacket*/)
	{
		MS_TRACE();

		// Do nothing since we do not enable NACK.
	}

	void PipeConsumer::ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType)
	{
		MS_TRACE();

		// TODO: Must call the corresponding rtpStream->ReceiveKeyFrameRequest(messageType);

		RequestKeyFrame();
	}

	void PipeConsumer::ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto ssrc       = kv.first;
			auto& rtpStream = kv.second;

			if (report->GetSsrc() == ssrc)
			{
				rtpStream->ReceiveRtcpReceiverReport(report);

				break;
			}
		}
	}

	uint32_t PipeConsumer::GetTransmissionRate(uint64_t now)
	{
		MS_TRACE();

		uint32_t rate{ 0 };

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream = kv.second;

			rate += rtpStream->GetRate(now);
		}

		return rate;
	}

	float PipeConsumer::GetLossPercentage() const
	{
		MS_TRACE();

		// Do nothing.
		return 0u;
	}

	void PipeConsumer::Paused(bool /*wasProducer*/)
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream = kv.second;

			rtpStream->Pause();
		}
	}

	void PipeConsumer::Resumed(bool wasProducer)
	{
		MS_TRACE();

		for (auto& kv : this->mapMappedSsrcRtpStream)
		{
			auto& rtpStream = kv.second;

			rtpStream->Resume();
		}

		// If we have been resumed due to the Producer becoming resumed, we don't
		// need to request a key frame since the Producer already requested it.
		if (!wasProducer)
			RequestKeyFrame();
	}

	void PipeConsumer::CreateRtpStreams()
	{
		MS_TRACE();

		// NOTE: Here we know that SSRCs in Consumer's rtpParameters must be the same
		// as in the given consumableRtpEncodings.
		for (auto& encoding : this->rtpParameters.encodings)
		{
			auto* mediaCodec = this->rtpParameters.GetCodecForEncoding(encoding);

			// Set stream params.
			RTC::RtpStream::Params params;

			params.ssrc        = encoding.ssrc;
			params.payloadType = mediaCodec->payloadType;
			params.mimeType    = mediaCodec->mimeType;
			params.clockRate   = mediaCodec->clockRate;
			params.cname       = this->rtpParameters.rtcp.cname;

			// Check in band FEC in codec parameters.
			if (mediaCodec->parameters.HasInteger("useinbandfec") && mediaCodec->parameters.GetInteger("useinbandfec") == 1)
			{
				MS_DEBUG_TAG(rtcp, "in band FEC enabled");

				params.useInBandFec = true;
			}

			// Check DTX in codec parameters.
			if (mediaCodec->parameters.HasInteger("usedtx") && mediaCodec->parameters.GetInteger("usedtx") == 1)
			{
				MS_DEBUG_TAG(rtcp, "DTX enabled");

				params.useDtx = true;
			}

			// Check DTX in the encoding.
			if (encoding.dtx)
			{
				MS_DEBUG_TAG(rtcp, "DTX enabled");

				params.useDtx = true;
			}

			for (auto& fb : mediaCodec->rtcpFeedback)
			{
				// NOTE: Do not consider NACK in PipeConsumer.

				if (!params.usePli && fb.type == "nack" && fb.parameter == "pli")
				{
					MS_DEBUG_TAG(rtcp, "PLI supported");

					params.usePli = true;
				}
				else if (!params.useFir && fb.type == "ccm" && fb.parameter == "fir")
				{
					MS_DEBUG_TAG(rtcp, "FIR supported");

					params.useFir = true;
				}
			}

			// Create a RtpStreamSend for sending a single media stream.
			// NOTE: PipeConsumer does not support NACK.
			size_t bufferSize{ 0 };
			auto* rtpStream = new RTC::RtpStreamSend(this, params, bufferSize);

			// If the Consumer is paused, tell the RtpStreamSend.
			if (IsPaused() || IsProducerPaused())
				rtpStream->Pause();

			auto* rtxCodec = this->rtpParameters.GetRtxCodecForEncoding(encoding);

			if (rtxCodec && encoding.hasRtx)
				rtpStream->SetRtx(rtxCodec->payloadType, encoding.rtx.ssrc);

			this->mapMappedSsrcRtpStream[encoding.ssrc] = rtpStream;
		}
	}

	void PipeConsumer::RequestKeyFrame()
	{
		MS_TRACE();

		if (!IsActive() || this->kind != RTC::Media::Kind::VIDEO)
			return;

		for (auto& encoding : this->rtpParameters.encodings)
		{
			auto mappedSsrc = encoding.ssrc;

			this->listener->OnConsumerKeyFrameRequested(this, mappedSsrc);
		}
	}

	inline void PipeConsumer::OnRtpStreamScore(RTC::RtpStream* /*rtpStream*/, uint8_t /*score*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	inline void PipeConsumer::OnRtpStreamRetransmitRtpPacket(
	  RTC::RtpStreamSend* /*rtpStream*/, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->listener->OnConsumerSendRtpPacket(this, packet);
	}
} // namespace RTC
