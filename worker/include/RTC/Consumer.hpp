#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/RtpStreamSend.hpp"
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
			virtual void OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet) = 0;
			virtual void OnConsumerKeyFrameRequired(RTC::Consumer* consumer, uint32_t mappedSsrc) = 0;
			virtual void onConsumerProducerClosed(RTC::Consumer* consumer)                        = 0;
		};

	public:
		Consumer(const std::string& id, Listener* listener, json& data);
		virtual ~Consumer();

	public:
		virtual void FillJson(json& jsonObject) const;
		virtual void FillJsonStats(json& jsonArray) const = 0;
		virtual void HandleRequest(Channel::Request* request);
		const std::vector<uint32_t>& GetMediaSsrcs() const;
		bool IsActive() const;
		bool IsProducerPaused() const; // This is needed by the Transport.
		virtual void TransportConnected() = 0;
		void ProducerPaused();
		void ProducerResumed();
		virtual void ProducerNewRtpStream(RTC::RtpStream* rtpStream, uint32_t mappedSsrc) = 0;
		virtual void ProducerRtpStreamScore(RTC::RtpStream* rtpStream, uint8_t score)     = 0;
		void ProducerClosed();
		virtual void SendRtpPacket(RTC::RtpPacket* packet)                                  = 0;
		virtual void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now)               = 0;
		virtual void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket)              = 0;
		virtual void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType) = 0;
		virtual void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report)           = 0;
		virtual uint32_t GetTransmissionRate(uint64_t now)                                  = 0;
		virtual float GetLossPercentage() const                                             = 0;

	protected:
		virtual void Started()                         = 0;
		virtual void Paused(bool wasProducer = false)  = 0;
		virtual void Resumed(bool wasProducer = false) = 0;

	public:
		// Passed by argument.
		const std::string id;

	protected:
		// Passed by argument.
		Listener* listener{ nullptr };
		RTC::Media::Kind kind;
		RTC::RtpParameters rtpParameters;
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;
		// Others.
		std::unordered_set<uint8_t> supportedCodecPayloadTypes;

	private:
		// Others.
		std::vector<uint32_t> mediaSsrcs;
		bool started{ false };
		bool paused{ false };
		bool producerPaused{ false };
	};

	/* Inline methods. */

	inline const std::vector<uint32_t>& Consumer::GetMediaSsrcs() const
	{
		return this->mediaSsrcs;
	}

	inline bool Consumer::IsActive() const
	{
		return this->started && !this->paused && !this->producerPaused;
	}

	inline bool Consumer::IsProducerPaused() const
	{
		return this->producerPaused;
	}
} // namespace RTC

#endif
