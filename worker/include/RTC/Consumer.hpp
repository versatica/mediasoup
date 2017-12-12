#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include "RTC/ConsumerListener.hpp"
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
#include "RTC/Transport.hpp"
#include <json/json.h>
#include <set>
#include <unordered_set>

namespace RTC
{
	class Consumer : public RTC::RtpMonitor::Listener
	{
		static constexpr uint16_t RtpPacketsBeforeProbation{ 2000 };
		// Must be a power of 2.
		static constexpr uint16_t ProbationPacketNumber{ 256 };

	public:
		Consumer(
		  Channel::Notifier* notifier,
		  uint32_t consumerId,
		  RTC::Media::Kind kind,
		  uint32_t sourceProducerId);

	private:
		virtual ~Consumer();

	public:
		void Destroy();
		Json::Value ToJson() const;
		Json::Value GetStats() const;
		void AddListener(RTC::ConsumerListener* listener);
		void RemoveListener(RTC::ConsumerListener* listener);
		void Enable(RTC::Transport* transport, RTC::RtpParameters& rtpParameters);
		void Pause();
		void Resume();
		void SourcePause();
		void SourceResume();
		void AddProfile(const RTC::RtpEncodingParameters::Profile profile, const RTC::RtpStream* rtpStream);
		void RemoveProfile(const RTC::RtpEncodingParameters::Profile profile);
		void SourceRtpParametersUpdated();
		void SetPreferredProfile(const RTC::RtpEncodingParameters::Profile profile);
		void SetSourcePreferredProfile(const RTC::RtpEncodingParameters::Profile profile);
		void SetEncodingPreferences(const RTC::Codecs::EncodingContext::Preferences preferences);
		void Disable();
		bool IsEnabled() const;
		const RTC::RtpParameters& GetParameters() const;
		bool IsPaused() const;
		RTC::RtpEncodingParameters::Profile GetPreferredProfile() const;
		void SendRtpPacket(RTC::RtpPacket* packet, RTC::RtpEncodingParameters::Profile profile);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveKeyFrameRequest(RTCP::FeedbackPs::MessageType messageType);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		uint32_t GetTransmissionRate(uint64_t now);
		float GetLossPercentage() const;
		void RequestKeyFrame();

	private:
		void FillSupportedCodecPayloadTypes();
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding);
		void RetransmitRtpPacket(RTC::RtpPacket* packet);
		void RecalculateTargetProfile(bool force = false);
		void SetEffectiveProfile(RTC::RtpEncodingParameters::Profile profile);
		void MayRunProbation();
		bool IsProbing() const;
		void StartProbation(RTC::RtpEncodingParameters::Profile profile);
		void StopProbation();
		void SendProbation(RTC::RtpPacket* packet);

		/* Pure virtual methods inherited from RTC::RtpMonitor::Listener. */
	public:
		void OnRtpMonitorHealthy() override;
		void OnRtpMonitorUnhealthy() override;

	public:
		// Passed by argument.
		uint32_t consumerId{ 0 };
		RTC::Media::Kind kind;
		uint32_t sourceProducerId{ 0 };

	private:
		// Passed by argument.
		Channel::Notifier* notifier{ nullptr };
		RTC::Transport* transport{ nullptr };
		RTC::RtpParameters rtpParameters;
		std::unordered_set<RTC::ConsumerListener*> listeners;
		// Allocated by this.
		RTC::RtpStreamSend* rtpStream{ nullptr };
		RtpMonitor* rtpMonitor{ nullptr };
		// Others.
		std::unordered_set<uint8_t> supportedCodecPayloadTypes;
		bool paused{ false };
		bool sourcePaused{ false };
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
		// RTP profiles.
		std::map<RTC::RtpEncodingParameters::Profile, const RTC::RtpStream*> mapProfileRtpStream;
		RTC::RtpEncodingParameters::Profile preferredProfile{ RTC::RtpEncodingParameters::Profile::DEFAULT };
		RTC::RtpEncodingParameters::Profile sourcePreferredProfile{
			RTC::RtpEncodingParameters::Profile::DEFAULT
		};
		RTC::RtpEncodingParameters::Profile targetProfile{ RTC::RtpEncodingParameters::Profile::DEFAULT };
		RTC::RtpEncodingParameters::Profile effectiveProfile{ RTC::RtpEncodingParameters::Profile::DEFAULT };
		RTC::RtpEncodingParameters::Profile probingProfile{ RTC::RtpEncodingParameters::Profile::NONE };
		// RTP probation.
		uint16_t rtpPacketsBeforeProbation{ RtpPacketsBeforeProbation };
		uint16_t probationPackets{ 0 };
	};

	/* Inline methods. */

	inline void Consumer::AddListener(RTC::ConsumerListener* listener)
	{
		this->listeners.insert(listener);
	}

	inline void Consumer::RemoveListener(RTC::ConsumerListener* listener)
	{
		this->listeners.erase(listener);
	}

	inline bool Consumer::IsEnabled() const
	{
		return this->transport != nullptr;
	}

	inline const RTC::RtpParameters& Consumer::GetParameters() const
	{
		return this->rtpParameters;
	}

	inline bool Consumer::IsPaused() const
	{
		return this->paused || this->sourcePaused;
	}

	inline RTC::RtpEncodingParameters::Profile Consumer::GetPreferredProfile() const
	{
		// If Consumer preferred profile and source (Producer) preferred profile
		// are the same, that's.
		if (this->preferredProfile == this->sourcePreferredProfile)
			return this->preferredProfile;

		// If Consumer preferred profile is 'default', use whichever the source
		// preferred profile is.
		if (this->preferredProfile == RTC::RtpEncodingParameters::Profile::DEFAULT)
			return this->sourcePreferredProfile;

		// If source preferred profile is 'default', use whichever the Consumer
		// preferred profile is.
		if (this->sourcePreferredProfile == RTC::RtpEncodingParameters::Profile::DEFAULT)
			return this->preferredProfile;

		// Otherwise the Consumer preferred profile is chosen.
		return this->preferredProfile;
	}

	inline uint32_t Consumer::GetTransmissionRate(uint64_t now)
	{
		return this->rtpStream->GetRate(now) + this->retransmittedCounter.GetRate(now);
	}

	inline bool Consumer::IsProbing() const
	{
		return this->probationPackets != 0;
	}

	inline void Consumer::StartProbation(RTC::RtpEncodingParameters::Profile profile)
	{
		this->probationPackets = ProbationPacketNumber;
		this->probingProfile   = profile;
	}

	inline void Consumer::StopProbation()
	{
		this->probationPackets = 0;
		this->probingProfile   = RTC::RtpEncodingParameters::Profile::NONE;
	}
} // namespace RTC

#endif
