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
		    uint32_t sourceProducerId,
		    RTC::Transport* transport);

	private:
		virtual ~Consumer();

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		void AddListener(RTC::ConsumerListener* listener);
		void RemoveListener(RTC::ConsumerListener* listener);
		bool IsEnabled() const;
		RTC::RtpParameters* GetParameters() const;
		void SendRtpPacket(RTC::RtpPacket* packet);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		uint32_t GetTransmissionRate(uint64_t now);
		void RequestFullFrame();

	private:
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding);
		void RetransmitRtpPacket(RTC::RtpPacket* packet);

	public:
		// Passed by argument.
		uint32_t consumerId{ 0 };
		RTC::Media::Kind kind;
		uint32_t sourceProducerId{ 0 };

	private:
		// Passed by argument.
		std::unordered_set<RTC::ConsumerListener*> listeners;
		Channel::Notifier* notifier{ nullptr };
		RTC::Transport* transport{ nullptr };
		// Allocated by this.
		RTC::RtpParameters* rtpParameters{ nullptr };
		RTC::RtpStreamSend* rtpStream{ nullptr };
		// Others.
		std::unordered_set<uint8_t> supportedCodecPayloadTypes;
		bool paused{ false };
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
		return this->transport && this->rtpParameters && this->rtpStream;
	}

	inline RTC::RtpParameters* Consumer::GetParameters() const
	{
		return this->rtpParameters;
	}

	inline uint32_t Consumer::GetTransmissionRate(uint64_t now)
	{
		return this->transmittedCounter.GetRate(now) + this->retransmittedCounter.GetRate(now);
	}
} // namespace RTC

#endif
