#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/ConsumerListener.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RtpDataCounter.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStreamSend.hpp"
#include "RTC/Transport.hpp"
#include <json/json.h>
#include <unordered_set>

namespace RTC
{
	class Consumer
	{
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
		void HandleRequest(Channel::Request* request);
		void AddListener(RTC::ConsumerListener* listener);
		void RemoveListener(RTC::ConsumerListener* listener);
		void Enable(RTC::Transport* transport, RTC::RtpParameters& rtpParameters);
		void Disable();
		bool IsEnabled() const;
		const RTC::RtpParameters& GetParameters() const;
		bool IsPaused() const;
		void SetSourcePaused();
		void SetSourceResumed();
		void SendRtpPacket(RTC::RtpPacket* packet);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		uint32_t GetTransmissionRate(uint64_t now);
		void RequestFullFrame();

	private:
		void FillSupportedCodecPayloadTypes();
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding);
		void RetransmitRtpPacket(RTC::RtpPacket* packet);
		void Pause();
		void Resume();

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
		// Others.
		std::unordered_set<uint8_t> supportedCodecPayloadTypes;
		bool paused{ false };
		bool sourcePaused{ false };
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
		// RTP counters.
		RTC::RtpDataCounter transmittedCounter;
		RTC::RtpDataCounter retransmittedCounter;
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

	inline void Consumer::SetSourcePaused()
	{
		if (this->sourcePaused)
			return;

		bool wasPaused = IsPaused();

		this->sourcePaused = true;
		this->notifier->Emit(this->consumerId, "sourcepaused");

		if (!wasPaused)
			Pause();
	}

	inline void Consumer::SetSourceResumed()
	{
		if (!this->sourcePaused)
			return;

		bool wasPaused = IsPaused();

		this->sourcePaused = false;
		this->notifier->Emit(this->consumerId, "sourceresumed");

		if (wasPaused)
			Resume();
	}

	inline uint32_t Consumer::GetTransmissionRate(uint64_t now)
	{
		return this->transmittedCounter.GetRate(now) + this->retransmittedCounter.GetRate(now);
	}

	inline void Consumer::Pause()
	{
		if (!IsEnabled())
			return;

		this->rtpStream->Reset();
	}

	inline void Consumer::Resume()
	{
		if (!IsEnabled())
			return;

		for (auto& listener : this->listeners)
		{
			listener->OnConsumerFullFrameRequired(this);
		}
	}
} // namespace RTC

#endif
