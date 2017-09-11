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
#include "handles/Timer.hpp"
#include <json/json.h>
#include <map>
#include <string>
#include <unordered_set>
#include <set>
#include <vector>

namespace RTC
{
	class Producer : public RtpStreamRecv::Listener, public Timer::Listener
	{
	public:
		struct RtpMapping
		{
			std::map<uint8_t, uint8_t> codecPayloadTypes;
			std::map<uint8_t, uint8_t> headerExtensionIds;
		};

	public:
		// TODO: YES?
		// RTP header extension ids that must be shared by all the Producers using
		// the same Transport.
		// NOTE: These ids are the original ids in the RTP packet (before the Producer
		// maps them to the corresponding ids in the room).
		struct HeaderExtensionIdsForTransport
		{
			uint8_t absSendTime{ 0 }; // 0 means no abs-send-time id.
			uint8_t rid{ 0 };         // 0 means no RID id.
		};

	private:
		struct HeaderExtensionIds
		{
			uint8_t ssrcAudioLevel{ 0 }; // 0 means no ssrc-audio-level id.
			uint8_t absSendTime{ 0 };    // 0 means no abs-send-time id.
			uint8_t rid{ 0 };            // 0 means no RID id.
		};

	public:
		Producer(
		  Channel::Notifier* notifier,
		  uint32_t producerId,
		  RTC::Media::Kind kind,
		  RTC::Transport* transport,
		  RTC::RtpParameters& rtpParameters,
		  struct RtpMapping& rtpMapping,
		  bool paused);

	public:
		// Must be public because Router needs to call it.
		virtual ~Producer();

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		void AddListener(RTC::ProducerListener* listener);
		void RemoveListener(RTC::ProducerListener* listener);
		void UpdateRtpParameters(RTC::RtpParameters& rtpParameters);
		void Pause();
		void Resume();
		void SetRtpRawEvent(bool enabled);
		void SetRtpObjectEvent(bool enabled);
		const RTC::RtpParameters& GetParameters() const;
		bool IsPaused() const;
		void ReceiveRtpPacket(RTC::RtpPacket* packet);
		void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);
		void GetRtcp(RTC::RTCP::CompoundPacket* packet, uint64_t now);
		void ReceiveRtcpFeedback(RTC::RTCP::FeedbackPsPacket* packet) const;
		void ReceiveRtcpFeedback(RTC::RTCP::FeedbackRtpPacket* packet) const;
		void RequestFullFrame(bool force = false);
		const std::set<RTC::RtpEncodingParameters::Profile> GetProfiles() const;

	private:
		void FillHeaderExtensionIds();
		void CreateRtpStream(RTC::RtpEncodingParameters& encoding);
		void ClearRtpStream(RTC::RtpStreamRecv* rtpStream);
		void ClearRtpStreams();
		void ApplyRtpMapping(RTC::RtpPacket* packet) const;
		RTC::RtpEncodingParameters::Profile GetProfile(RTC::RtpStreamRecv* rtpStream, RTC::RtpPacket* packet);

		/* Pure virtual methods inherited from RTC::RtpStreamRecv::Listener. */
	public:
		void OnRtpStreamRecvNackRequired(
		  RTC::RtpStreamRecv* rtpStream, const std::vector<uint16_t>& seqNumbers) override;
		void OnRtpStreamRecvPliRequired(RTC::RtpStreamRecv* rtpStream) override;
		void OnRtpStreamHealthReport(RTC::RtpStream* rtpStream, bool healthy) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Passed by argument.
		uint32_t producerId{ 0 };
		RTC::Media::Kind kind;

	private:
		// Passed by argument.
		Channel::Notifier* notifier{ nullptr };
		RTC::Transport* transport{ nullptr };
		RTC::RtpParameters rtpParameters;
		struct RtpMapping rtpMapping;
		std::unordered_set<RTC::ProducerListener*> listeners;
		// Allocated by this.
		std::map<uint32_t, RTC::RtpStreamRecv*> rtpStreams;
		std::map<uint32_t, RTC::RtpStreamRecv*> mapRtxStreams;
		std::map<RTC::RtpStreamRecv*, std::set<RTC::RtpEncodingParameters::Profile>> profiles;
		Timer* fullFrameRequestBlockTimer{ nullptr };
		// Others.
		std::vector<RtpEncodingParameters> outputEncodings;
		struct HeaderExtensionIds headerExtensionIds;
		bool paused{ false };
		bool rtpRawEventEnabled{ false };
		bool rtpObjectEventEnabled{ false };
		bool isFullFrameRequested{ false };
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

	inline const RTC::RtpParameters& Producer::GetParameters() const
	{
		return this->rtpParameters;
	}

	inline bool Producer::IsPaused() const
	{
		return this->paused;
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

	inline const std::set<RTC::RtpEncodingParameters::Profile> Producer::GetProfiles() const
	{
		std::set<RTC::RtpEncodingParameters::Profile> profiles;

		for (const auto& it : this->profiles)
		{
			for (const auto& it2 : it.second)
			{
				auto profile = it2;

				if (profile == RTC::RtpEncodingParameters::Profile::DEFAULT)
					break;

				profiles.insert(profile);
			}
		}

		return profiles;
	}
} // namespace RTC

#endif
