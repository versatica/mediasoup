#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpMonitor.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStreamSend.hpp"
#include "RTC/SeqManager.hpp"
#include <string>
#include <unordered_set>
#include <vector>

namespace RTC
{
	class Consumer : public RTC::RtpMonitor::Listener
	{
		static constexpr uint16_t RtpPacketsBeforeProbation{ 2000 };
		// Must be a power of 2.
		static constexpr uint16_t ProbationPacketNumber{ 256 };

	public:
		class Listener
		{
		public:
			virtual void OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet) = 0;
			virtual void OnConsumerKeyFrameRequired(RTC::Consumer* consumer, uint32_t ssrc)       = 0;
			virtual void onConsumerProducerClosed(RTC::Consumer* consumer)                        = 0;
		};

	public:
		Consumer(
		  const std::string& id,
		  Listener* listener,
		  RTC::Media::Kind kind,
		  RTC::RtpParameters& rtpParameters,
		  std::vector<RTC::RtpEncodingParameters>& consumableRtpEncodings);
		virtual ~Consumer();

	public:
		void FillJson(json& jsonObject) const;
		void FillJsonStats(json& jsonArray) const;
		void HandleRequest(Channel::Request* request);
		bool IsStarted() const;
		bool IsPaused() const;
		bool IsProducerPaused() const;
		std::vector<uint32_t>& GetMediaSsrcs();
		void TransportConnected();
		void ProducerPaused();
		void ProducerResumed();
		void ProducerRtpStreamHealthy(RTC::RtpStream* rtpStream, uint32_t mappedSsrc);
		void ProducerRtpStreamUnhealthy(RTC::RtpStream* rtpStream, uint32_t mappedSsrc);
		void ProducerClosed();
		void SendRtpPacket(RTC::RtpPacket* packet);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveKeyFrameRequest(RTC::RTCP::FeedbackPs::MessageType messageType);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);

		uint32_t GetTransmissionRate(uint64_t now);
		float GetLossPercentage() const;
		void RequestKeyFrame(); // TODO

	private:
		void CreateRtpStream();
		void RetransmitRtpPacket(RTC::RtpPacket* packet);
		// void RecalculateTargetProfile(bool force = false);
		// void SetEffectiveProfile(RTC::RtpEncodingParameters::Profile profile);
		void MayRunProbation();
		bool IsProbing() const;
		// void StartProbation(RTC::RtpEncodingParameters::Profile profile);
		// void StopProbation();
		// void SendProbation(RTC::RtpPacket* packet);

		/* Pure virtual methods inherited from RTC::RtpMonitor::Listener. */
	public:
		void OnRtpMonitorScore(uint8_t score) override;

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		RTC::Media::Kind kind;
		RTC::RtpParameters rtpParameters;
		std::vector<RTC::RtpEncodingParameters> consumableRtpEncodings;
		// Allocated by this.
		RTC::RtpStreamSend* rtpStream{ nullptr };
		RtpMonitor* rtpMonitor{ nullptr };
		// Others.
		std::unordered_set<uint8_t> supportedCodecPayloadTypes;
		std::vector<uint32_t> mediaSsrcs; // TODO
		bool started{ false };
		bool paused{ false };
		bool producerPaused{ false };
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
		// RTP counters.
		RTC::RtpDataCounter retransmittedCounter;
		// RTP sequence number and timestamp.
		RTC::SeqManager<uint16_t> rtpSeqManager;
		RTC::SeqManager<uint32_t> rtpTimestampManager;
		bool syncRequired{ true };
		// RTP payload descriptor encoding.
		std::unique_ptr<RTC::Codecs::EncodingContext> encodingContext;
		uint16_t rtpPacketsBeforeProbation{ RtpPacketsBeforeProbation };
		uint16_t probationPackets{ 0 };
	};

	/* Inline methods. */

	inline bool Consumer::IsStarted() const
	{
		return this->started;
	}

	inline bool Consumer::IsPaused() const
	{
		return this->paused || this->producerPaused;
	}

	inline bool Consumer::IsProducerPaused() const
	{
		return this->paused || this->producerPaused;
	}

	inline std::vector<uint32_t>& Consumer::GetMediaSsrcs()
	{
		return this->mediaSsrcs;
	}

	inline uint32_t Consumer::GetTransmissionRate(uint64_t now)
	{
		return this->rtpStream->GetRate(now) + this->retransmittedCounter.GetRate(now);
	}

	inline bool Consumer::IsProbing() const
	{
		return this->probationPackets != 0;
	}

	// inline void Consumer::StartProbation(RTC::RtpEncodingParameters::Profile profile)
	// {
	// 	this->probationPackets = ProbationPacketNumber;
	// 	this->probingProfile   = profile;
	// }

	// inline void Consumer::StopProbation()
	// {
	// 	this->probationPackets = 0;
	// 	this->probingProfile   = RTC::RtpEncodingParameters::Profile::NONE;
	// }
} // namespace RTC

#endif
