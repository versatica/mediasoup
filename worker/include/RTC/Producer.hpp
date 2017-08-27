#ifndef MS_RTC_PRODUCER_HPP
#define MS_RTC_PRODUCER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include <json/json.h>
#include <map>
#include <string>

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class Transport;

	class Producer : public RtpStreamRecv::Listener
	{
	public:
		/**
		 * RTC::Peer is the Listener.
		 */
		class Listener
		{
		public:
			virtual void OnProducerRtpParameters(RTC::Producer* producer)             = 0;
			virtual void OnRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet) = 0;
			virtual void OnProducerClosed(const RTC::Producer* producer)              = 0;
		};

	private:
		struct RtpMapping
		{
			std::map<uint8_t, uint8_t> codecPayloadTypes;
			std::map<uint8_t, uint8_t> headerExtensionIds;
		};

	private:
		struct KnownHeaderExtensions
		{
			uint8_t ssrcAudioLevelId{ 0 }; // 0 means no ssrc-audio-level id.
			uint8_t absSendTimeId{ 0 };    // 0 means no abs-send-time id.
		};

	public:
		Producer(Listener* listener, Channel::Notifier* notifier, uint32_t producerId, RTC::Media::Kind kind);

	private:
		virtual ~Producer();

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		void SetTransport(RTC::Transport* transport);
		RTC::Transport* GetTransport() const;
		void RemoveTransport(RTC::Transport* transport);
		RTC::RtpParameters* GetParameters() const;
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void ReceiveRtcpFeedback(RTC::RTCP::FeedbackPsPacket* packet) const;
		void ReceiveRtcpFeedback(RTC::RTCP::FeedbackRtpPacket* packet) const;
		void RequestFullFrame() const;

	private:
		void CreateRtpMapping(Json::Value& rtpMapping);
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding);
		void ClearRtpStreams();
		void ApplyRtpMapping(RTC::RtpPacket* packet);

		/* Pure virtual methods inherited from RTC::RtpStreamRecv::Listener. */
	public:
		void OnNackRequired(RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers) override;
		void OnPliRequired(RTC::RtpStreamRecv* rtpStream) override;

	public:
		// Passed by argument.
		uint32_t producerId{ 0 };
		RTC::Media::Kind kind;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		Channel::Notifier* notifier{ nullptr };
		RTC::Transport* transport{ nullptr };
		// Allocated by this.
		RTC::RtpParameters* rtpParameters{ nullptr };
		std::map<uint32_t, RTC::RtpStreamRecv*> rtpStreams;
		std::map<uint32_t, RTC::RtpStreamRecv*> rtxStreamMap;
		// Others.
		struct RtpMapping rtpMapping;
		struct KnownHeaderExtensions knownHeaderExtensions;
		bool rtpRawEventEnabled{ false };
		bool rtpObjectEventEnabled{ false };
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
	};

	/* Inline methods. */

	inline void Producer::SetTransport(RTC::Transport* transport)
	{
		this->transport = transport;
	}

	inline RTC::Transport* Producer::GetTransport() const
	{
		return this->transport;
	}

	inline void Producer::RemoveTransport(RTC::Transport* transport)
	{
		if (this->transport == transport)
			this->transport = nullptr;
	}

	inline RTC::RtpParameters* Producer::GetParameters() const
	{
		return this->rtpParameters;
	}

	inline void Producer::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		auto it = this->rtpStreams.find(report->GetSsrc());

		if (it != this->rtpStreams.end())
		{
			auto rtpStream = it->second;

			rtpStream->ReceiveRtcpSenderReport(report);
		}
	}
} // namespace RTC

#endif
