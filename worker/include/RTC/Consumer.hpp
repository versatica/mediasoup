#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include <string>
#include <unordered_set>
#include <vector>

namespace RTC
{
	class Consumer
	{
	public:
		class Listener
		{
		public:
			virtual void OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet)  = 0;
			virtual void OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc) = 0;
			virtual void onConsumerProducerClosed(RTC::Consumer* consumer)                         = 0;
		};

	public:
		Consumer(
		  const std::string& id,
		  RTC::Consumer::Listener* listener,
		  json& data,
		  RTC::RtpParameters::Type type);
		virtual ~Consumer();

	public:
		virtual void FillJson(json& jsonObject) const;
		virtual void FillJsonStats(json& jsonArray) const  = 0;
		virtual void FillJsonScore(json& jsonObject) const = 0;
		virtual void HandleRequest(Channel::Request* request);
		RTC::Media::Kind GetKind() const;
		const RTC::RtpParameters& GetRtpParameters() const;
		const struct RTC::RtpHeaderExtensionIds& GetRtpHeaderExtensionIds() const;
		RTC::RtpParameters::Type GetType() const;
		const std::vector<uint32_t>& GetMediaSsrcs() const;
		bool IsActive() const;
		bool IsPaused() const;
		bool IsProducerPaused() const;
		virtual void UseBandwidth(uint32_t availableBandwidth) = 0;
		void ProducerPaused();
		void ProducerResumed();
		virtual void ProducerRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc)    = 0;
		virtual void ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) = 0;
		virtual void ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score)     = 0;
		void ProducerClosed();
		virtual void SendRtpPacket(RTC::RtpPacket* packet)                    = 0;
		virtual void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now) = 0;
		virtual void NeedWorstRemoteFractionLost(uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost) = 0;
		virtual void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)              = 0;
		virtual void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType) = 0;
		virtual void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)           = 0;
		virtual uint32_t GetTransmissionRate(uint64_t now)                                  = 0;
		virtual float GetLossPercentage() const                                             = 0;

	protected:
		virtual void Paused(bool wasProducer)  = 0;
		virtual void Resumed(bool wasProducer) = 0;

	public:
		// Passed by argument.
		const std::string id;

	protected:
		// Passed by argument.
		RTC::Consumer::Listener* listener{ nullptr };
		RTC::Media::Kind kind;
		RTC::RtpParameters rtpParameters;
		RTC::RtpParameters::Type type{ RTC::RtpParameters::Type::NONE };
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;
		struct RTC::RtpHeaderExtensionIds rtpHeaderExtensionIds;
		// Others.
		std::unordered_set<uint8_t> supportedCodecPayloadTypes;
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };

	private:
		// Others.
		std::vector<uint32_t> mediaSsrcs;
		bool paused{ false };
		bool producerPaused{ false };
		bool producerClosed{ false };
	};

	/* Inline methods. */

	inline RTC::Media::Kind Consumer::GetKind() const
	{
		return this->kind;
	}

	inline const RTC::RtpParameters& Consumer::GetRtpParameters() const
	{
		return this->rtpParameters;
	}

	inline const struct RTC::RtpHeaderExtensionIds& Consumer::GetRtpHeaderExtensionIds() const
	{
		return this->rtpHeaderExtensionIds;
	}

	inline RTC::RtpParameters::Type Consumer::GetType() const
	{
		return this->type;
	}

	inline const std::vector<uint32_t>& Consumer::GetMediaSsrcs() const
	{
		return this->mediaSsrcs;
	}

	inline bool Consumer::IsActive() const
	{
		return !this->paused && !this->producerPaused && !this->producerClosed;
	}

	inline bool Consumer::IsPaused() const
	{
		return this->paused;
	}

	inline bool Consumer::IsProducerPaused() const
	{
		return this->producerPaused;
	}
} // namespace RTC

#endif
