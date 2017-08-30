#ifndef MS_RTC_PRODUCER_HPP
#define MS_RTC_PRODUCER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/ProducerListener.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStreamRecv.hpp"
#include "RTC/Transport.hpp"
#include <json/json.h>
#include <map>
#include <string>
#include <unordered_set>

namespace RTC
{
	class Producer : public RtpStreamRecv::Listener
	{
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
		Producer(
		    Channel::Notifier* notifier,
		    uint32_t producerId,
		    RTC::Media::Kind kind,
		    RTC::Transport* transport);

	public:
		// Must be public because Router needs to call it.
		virtual ~Producer();

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		void AddListener(RTC::ProducerListener* listener);
		void RemoveListener(RTC::ProducerListener* listener);
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
		std::unordered_set<RTC::ProducerListener*> listeners;
		Channel::Notifier* notifier{ nullptr };
		RTC::Transport* transport{ nullptr };
		// Allocated by this.
		RTC::RtpParameters* rtpParameters{ nullptr };
		std::map<uint32_t, RTC::RtpStreamRecv*> rtpStreams;
		std::map<uint32_t, RTC::RtpStreamRecv*> mapRtxStreams;
		// Others.
		struct RtpMapping rtpMapping;
		struct KnownHeaderExtensions knownHeaderExtensions;
		bool paused{ false };
		bool rtpRawEventEnabled{ false };
		bool rtpObjectEventEnabled{ false };
		// Timestamp when last RTCP was sent.
		uint64_t lastRtcpSentTime{ 0 };
		uint16_t maxRtcpInterval{ 0 };
	};

	/* Inline methods. */

	inline void Producer::AddListener(RTC::ProducerListener* listener)
	{
		this->listeners.insert(listener);
	}

	inline void Producer::RemoveListener(RTC::ProducerListener* listener)
	{
		this->listeners.erase(listener);
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
