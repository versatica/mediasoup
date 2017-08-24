#ifndef MS_RTC_CONSUMER_HPP
#define MS_RTC_CONSUMER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
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
		/**
		 * RTC::Peer is the Listener.
		 */
		class Listener
		{
		public:
			virtual void OnConsumerClosed(Consumer* consumer)            = 0;
			virtual void OnConsumerFullFrameRequired(Consumer* consumer) = 0;
		};

	public:
		Consumer(Listener* listener, Channel::Notifier* notifier, uint32_t consumerId, RTC::Media::Kind kind);

	private:
		virtual ~Consumer();

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		void Send(RTC::RtpParameters* rtpParameters);
		void SetTransport(RTC::Transport* transport);
		RTC::Transport* GetTransport() const;
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetParameters() const;
		bool GetEnabled() const;
		void SendRtpPacket(RTC::RtpPacket* packet);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void ReceiveNack(RTC::RTCP::FeedbackRtpNackPacket* nackPacket);
		void ReceiveRtcpReceiverReport(RTC::RTCP::ReceiverReport* report);
		uint32_t GetTransmissionRate(uint64_t now);

	private:
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding);
		void RetransmitRtpPacket(RTC::RtpPacket* packet);
		void EmitEnabledChange() const;

	public:
		// Passed by argument.
		uint32_t consumerId{ 0 };
		RTC::Media::Kind kind;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		Channel::Notifier* notifier{ nullptr };
		RTC::Transport* transport{ nullptr };
		// Allocated by this.
		RTC::RtpParameters* rtpParameters{ nullptr };
		RTC::RtpStreamSend* rtpStream{ nullptr };
		// Others.
		std::unordered_set<uint8_t> supportedPayloadTypes;
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
		// RTP counters.
		RTC::RtpDataCounter transmittedCounter;
		RTC::RtpDataCounter retransmittedCounter;
	};

	/* Inline methods. */

	inline void Consumer::SetTransport(RTC::Transport* transport)
	{
		bool wasEnabled = this->GetEnabled();

		this->transport = transport;

		if (wasEnabled != this->GetEnabled())
			EmitEnabledChange();
	}

	inline RTC::Transport* Consumer::GetTransport() const
	{
		return this->transport;
	}

	inline void Consumer::RemoveTransport(RTC::Transport* transport)
	{
		bool wasEnabled = this->GetEnabled();

		if (this->transport == transport)
			this->transport = nullptr;

		if (wasEnabled != this->GetEnabled())
			EmitEnabledChange();
	}

	inline RTC::RtpParameters* Consumer::GetParameters() const
	{
		return this->rtpParameters;
	}

	inline bool Consumer::GetEnabled() const
	{
		return this->transport;
	}

	inline uint32_t Consumer::GetTransmissionRate(uint64_t now)
	{
		return this->transmittedCounter.GetRate(now) + this->retransmittedCounter.GetRate(now);
	}
} // namespace RTC

#endif
